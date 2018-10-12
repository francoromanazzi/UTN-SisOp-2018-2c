#include "FM9.h"

int main(void) {
	if(fm9_initialize() == -1){
		fm9_exit(); return EXIT_FAILURE;
	}

	while(1){
		int nuevo_cliente = socket_aceptar_conexion(listening_socket);
		if( !fm9_crear_nuevo_hilo(nuevo_cliente)){
			log_error(logger, "No pude crear un nuevo hilo para atender una nueva conexion");
			fm9_exit();
			return EXIT_FAILURE;
		}
		log_info(logger, "Creo un nuevo hilo para atender los pedidos de un nuevo cliente");
	}

	fm9_exit();
	return EXIT_SUCCESS;
}

int _fm9_dir_logica_a_fisica_seg_pura(unsigned int pid, int nro_seg, int offset, int* ok){

	bool _mismo_pid(void* proceso){
		return ((t_proceso*) proceso)->pid == pid;
	}

	bool _mismo_nro_seg(void* fila_tabla){
		return ((t_fila_tabla_segmento*) fila_tabla)->nro_seg == nro_seg;
	}

	pthread_mutex_lock(&sem_mutex_lista_procesos);
	t_proceso* proceso = list_find(lista_procesos, _mismo_pid);
	pthread_mutex_unlock(&sem_mutex_lista_procesos);

	if(proceso == NULL) {
		*ok = FM9_ERROR_SEG_FAULT;
		return -1;
	}

	t_fila_tabla_segmento* fila_tabla = list_find(proceso->lista_tabla_segmentos, _mismo_nro_seg);
	if(fila_tabla == NULL) {
		*ok = FM9_ERROR_SEG_FAULT;
		return -1;
	}

	if(offset > fila_tabla->limite) {
		*ok = FM9_ERROR_SEG_FAULT;
		return -1;
	}

	*ok = OK;
	return (int) (fila_tabla->base + offset);
}

int fm9_initialize(){

	void _modo_y_estr_administrativas_init(){
		char* bitarray;
		switch(modo){
			case SEG:
				lista_procesos = list_create();
				pthread_mutex_init(&sem_mutex_lista_procesos, NULL);

				bitarray = calloc(storage_cant_lineas / 8, sizeof(char));
				bitarray_lineas = bitarray_create_with_mode(bitarray, storage_cant_lineas / 8, MSB_FIRST);
				pthread_mutex_init(&sem_mutex_bitarray_lineas, NULL);

				fm9_dir_logica_a_fisica = &_fm9_dir_logica_a_fisica_seg_pura;
				fm9_dump_pid = &_fm9_dump_pid_seg_pura;
			break;

			case TPI:
			break;

			case SPA:
			break;
		}
	}


	config = config_create(CONFIG_PATH);
	mkdir(LOG_DIRECTORY_PATH,0777);
	logger = log_create(LOG_PATH, "FM9", false, LOG_LEVEL_TRACE);

	modo = !strcmp("SEG", config_get_string_value(config, "MODO")) ? SEG : !strcmp("TPI", config_get_string_value(config, "MODO")) ? TPI : SPA;
	tamanio = config_get_int_value(config, "TAMANIO");
	max_linea = config_get_int_value(config, "MAX_LINEA");
	storage_cant_lineas = tamanio / max_linea;
	tam_pagina = config_get_int_value(config, "TAM_PAGINA");
	log_info(logger,"Se realiza la inicializacion del storage y de las estructuras administrativas");
	storage = calloc(1, tamanio);
	_modo_y_estr_administrativas_init();

	pthread_mutex_init(&sem_mutex_realocacion_en_progreso, NULL);

	pthread_t thread_consola;
	if(pthread_create(&thread_consola,NULL,(void*) fm9_consola_init,NULL)){
		log_error(logger,"No se pudo crear el hilo para la consola");
		return -1;
	}
	log_info(logger,"Se creo el hilo para la consola");

	if((listening_socket = socket_create_listener(IP, config_get_string_value(config, "PUERTO"))) == -1){
		log_error(logger,"No se pudo crear socket de escucha");
		return -1;
	}
	log_info(logger, "Comienzo a escuchar  por el socket %d", listening_socket);
	return 1;
}

int fm9_send(int socket, e_tipo_msg tipo_msg, ...){
	t_msg* mensaje_a_enviar;
	int ret, ok;
	char* str;
	bool str_free = false;

 	va_list arguments;
	va_start(arguments, tipo_msg);

	switch(tipo_msg){
		case RESULTADO_CREAR_FM9:
			ok = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_int(ok);
		break;

		case RESULTADO_GET_FM9:
			ok = va_arg(arguments, int);
			str = va_arg(arguments, char*);
			if(str == NULL){
				str = strdup("");
				str_free = true;
			}
			mensaje_a_enviar = empaquetar_resultado_get_fm9(ok, str);
			if(str_free) free(str);
		break;

		case RESULTADO_ESCRIBIR_FM9:
			ok = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_int(ok);
		break;

	}
	mensaje_a_enviar->header->emisor = FM9;
	mensaje_a_enviar->header->tipo_mensaje = tipo_msg;
	ret = msg_send(socket, *mensaje_a_enviar);
	msg_free(&mensaje_a_enviar);
	va_end(arguments);
	return ret;
}

bool fm9_crear_nuevo_hilo(int socket_nuevo_cliente){
	pthread_t nuevo_thread;
	if(pthread_create( &nuevo_thread, NULL, (void*) fm9_nuevo_cliente_iniciar, (void*) socket_nuevo_cliente) ){
		return false;
	}
	pthread_detach(nuevo_thread);
	return true;
}

void fm9_nuevo_cliente_iniciar(int socket){
	while(1){
		t_msg* nuevo_mensaje = malloc(sizeof(t_msg));
		if(msg_await(socket, nuevo_mensaje) == -1 || fm9_manejar_nuevo_mensaje(socket, nuevo_mensaje) == -1){
			log_info(logger, "Cierro el hilo que atendia a este cliente");
			free(nuevo_mensaje);
			close(socket);
			pthread_exit(NULL);
			return;
		}
		msg_free(&nuevo_mensaje);
	} // Fin while(1)
}

int fm9_manejar_nuevo_mensaje(int socket, t_msg* msg){
	log_info(logger, "EVENTO: Emisor: %d, Tipo: %d, Tamanio: %d",msg->header->emisor,msg->header->tipo_mensaje,msg->header->payload_size);

	unsigned int id;
	int base, offset, operacion_ok = OK;
	char* datos;

	if(msg->header->emisor == DAM){
		switch(msg->header->tipo_mensaje){
			case CONEXION:
				log_info(logger,"Se me conecto DAM");
			break;

			case DESCONEXION:
				log_info(logger,"Se me conecto DAM");
				return -1;
			break;

			case CREAR_FM9:
				id = desempaquetar_int(msg);
				log_info(logger,"DAM me pidio reservar memoria para un nuevo archivo del ID: %d", id);

				base = fm9_storage_nuevo_archivo(id, &operacion_ok);
				if(operacion_ok != OK){
					if(operacion_ok == FM9_ERROR_INSUFICIENTE_ESPACIO)
						operacion_ok = ERROR_ABRIR_ESPACIO_INSUFICIENTE_FM9;
					fm9_send(socket, RESULTADO_CREAR_FM9, operacion_ok);
				}
				else
					fm9_send(socket, RESULTADO_CREAR_FM9, base);
			break;

			case ESCRIBIR_FM9:
				desempaquetar_escribir_fm9(msg, &id, &base, &offset, &datos);
				log_info(logger,"DAM me pidio la operacion ESCRIBIR %s del ID: %d con base: %d y offset: %d", datos, id, base, offset);

				fm9_storage_escribir(id, base, offset, datos, &operacion_ok);

				if(operacion_ok != OK){ // Intento realocar el segmento a otro lugar mas grande
					fm9_storage_realocar(id, base, offset, &operacion_ok);
					if(operacion_ok){
						fm9_storage_escribir(id, base, offset, datos, &operacion_ok);
					}
				}
				free(datos);
				fm9_send(socket, RESULTADO_ESCRIBIR_FM9, operacion_ok);
			break;

			case GET_FM9:
				desempaquetar_get_fm9(msg, &id, &base, &offset);
				log_info(logger,"DAM me pidio la operacion GET del ID: %d con base: %d y offset: %d", id, base, offset);

				datos = fm9_storage_leer(id, base, offset, &operacion_ok);

				fm9_send(socket, RESULTADO_GET_FM9, (void*) operacion_ok, (void*) datos);
				if(datos != NULL) free(datos);
			break;

			default:
				log_info(logger,"No entendi el mensaje de DAM");
		}

	}
	else if(msg->header->emisor == CPU){
		switch(msg->header->tipo_mensaje){
			case CONEXION:
				log_info(logger,"Se me conecto CPU");
			break;

			case DESCONEXION:
				log_info(logger,"Se me desconecto CPU");
				return -1;
			break;

			case ESCRIBIR_FM9:
				desempaquetar_escribir_fm9(msg, &id, &base, &offset, &datos);
				log_info(logger,"CPU me pidio la operacion ESCRIBIR (asignar) %s del ID: %d con base: %d y offset: %d", datos, id, base, offset);

				fm9_storage_escribir(id, base, offset, datos, &operacion_ok);
				free(datos);
				if(operacion_ok == FM9_ERROR_SEG_FAULT){
					operacion_ok = ERROR_ASIGNAR_FALLO_SEGMENTO;
				}
				fm9_send(socket, RESULTADO_ESCRIBIR_FM9, operacion_ok);
			break;

			case GET_FM9:
				desempaquetar_get_fm9(msg, &id, &base, &offset);
				log_info(logger,"CPU me pidio la operacion GET del ID: %d con base: %d y offset: %d", id, base, offset);

				datos = fm9_storage_leer(id, base, offset, &operacion_ok);
				fm9_send(socket, RESULTADO_GET_FM9, (void*) operacion_ok, (void*) datos);
				if(datos != NULL) free(datos);
			break;

			default:
				log_info(logger,"No entendi el mensaje de CPU");
		}
	}
	else if(msg->header->emisor == DESCONOCIDO){
		log_info(logger, "Me hablo alguien desconocido");
	}
	return 1;
}

int fm9_storage_nuevo_archivo(unsigned int id, int* ok){

	bool _mismo_pid(void* proceso){
		return ((t_proceso*) proceso)->pid == id;
	}


	int base, nro_linea;
	bool linea_disponible = false;
	*ok = OK;

	pthread_mutex_lock(&sem_mutex_bitarray_lineas);
	for(nro_linea = 0; nro_linea < storage_cant_lineas && !(linea_disponible = !(bitarray_test_bit(bitarray_lineas, nro_linea))); nro_linea++);

	if(!linea_disponible){
		pthread_mutex_unlock(&sem_mutex_bitarray_lineas);
		*ok = FM9_ERROR_INSUFICIENTE_ESPACIO;
		return 0;
	}
	bitarray_set_bit(bitarray_lineas, nro_linea);
	pthread_mutex_unlock(&sem_mutex_bitarray_lineas);

	t_proceso* proceso;
	t_fila_tabla_segmento* nueva_fila_tabla;

	switch(modo){
		case SEG:
			pthread_mutex_lock(&sem_mutex_lista_procesos);
			if((proceso = list_find(lista_procesos, _mismo_pid)) == NULL){ // Este proceso no existia en la lista
				proceso = malloc(sizeof(t_proceso));
				proceso->pid = id;
				proceso->lista_tabla_segmentos = list_create();
				list_add(lista_procesos, proceso);
			}
			pthread_mutex_unlock(&sem_mutex_lista_procesos);

			nueva_fila_tabla = malloc(sizeof(t_fila_tabla_segmento));

			if(proceso->lista_tabla_segmentos->elements_count >= 1){
				nueva_fila_tabla->nro_seg = ((t_fila_tabla_segmento*) list_get(proceso->lista_tabla_segmentos, proceso->lista_tabla_segmentos->elements_count - 1))->nro_seg + 1;
			}else{
				nueva_fila_tabla->nro_seg = 0;
			}

			nueva_fila_tabla->base = nro_linea;
			nueva_fila_tabla->limite = 0;
			list_add(proceso->lista_tabla_segmentos, nueva_fila_tabla);

			base = nueva_fila_tabla->nro_seg;

			log_info(logger, "Agrego el segmento %d del PID: %d, Base: %d, Limite: %d",
					nueva_fila_tabla->nro_seg, proceso->pid, nueva_fila_tabla->base, nueva_fila_tabla->limite);
		break;

		case TPI:
		break;

		case SPA:
		break;
	}


	*ok = OK;
	return base;
}

void fm9_storage_realocar(unsigned int id, int base, int offset, int* ok){

	bool _mismo_id(void* proceso){
		return ((t_proceso*) proceso)->pid == id;
	}

	bool _mismo_nro_seg(void* fila_tabla){
		return ((t_fila_tabla_segmento*) fila_tabla)->nro_seg == base;
	}

	*ok = OK;
	t_proceso* proceso;
	t_fila_tabla_segmento* fila_tabla;
	int i, j, lineas_contiguas_disponibles = 0, base_nuevo_segmento = 0;
	t_list* backup_lineas;

	pthread_mutex_lock(&sem_mutex_realocacion_en_progreso);
	switch(modo){
		case SEG:
			pthread_mutex_lock(&sem_mutex_lista_procesos);
			proceso = list_find(lista_procesos, _mismo_id);
			pthread_mutex_unlock(&sem_mutex_lista_procesos);

			if(proceso == NULL){
				log_error(logger, "No se encontro el proceso al realocar");
				*ok = FM9_ERROR_NO_ENCONTRADO_EN_ESTR_ADM;
				pthread_mutex_unlock(&sem_mutex_realocacion_en_progreso);
				return;
			}
			fila_tabla = list_find(proceso->lista_tabla_segmentos, _mismo_nro_seg);
			if(fila_tabla == NULL){
				log_error(logger, "No se encontro el segmento al realocar");
				*ok = FM9_ERROR_NO_ENCONTRADO_EN_ESTR_ADM;
				pthread_mutex_unlock(&sem_mutex_realocacion_en_progreso);
				return;
			}
			log_info(logger, "Intento realocar el segmento %d del PID: %d con base %d, de %d a %d lineas", fila_tabla->nro_seg, proceso->pid, fila_tabla->base, fila_tabla->limite + 1, offset + 1);

			pthread_mutex_lock(&sem_mutex_bitarray_lineas);
			for(j = 0; j <= fila_tabla->limite; j++){
				bitarray_clean_bit(bitarray_lineas, fila_tabla->base + j);
			}

			for(i = 0; i < storage_cant_lineas; i++){ // Tengo que encontrar una cantidad "offset" de lineas contiguas disponibles

				if(bitarray_test_bit(bitarray_lineas, i) == true){ // Linea no disponible
					lineas_contiguas_disponibles = 0;
					base_nuevo_segmento = i + 1; // Reset del comienzo del nuevo segmento destino
				}
				else { // Linea disponible
					if(++lineas_contiguas_disponibles == offset + 1){ // Pude realocar!
						*ok = OK;

						backup_lineas = list_create();

						/* Actualizo storage y bitarray */
						for(j = 0; j <= fila_tabla->limite; j++){
							list_add(backup_lineas, (void*) fm9_storage_leer(id, (int) fila_tabla->nro_seg, j , ok));
						}

						fila_tabla->base = base_nuevo_segmento;
						fila_tabla->limite = offset;

						for(j = 0; j<backup_lineas->elements_count; j++){
							fm9_storage_escribir(id, (int) fila_tabla->nro_seg, j, (char*) list_get(backup_lineas, j), ok);
						}
						list_destroy_and_destroy_elements(backup_lineas, free);

						for(j = fila_tabla->base; j <= (fila_tabla->base + fila_tabla->limite); j++){
							bitarray_set_bit(bitarray_lineas, j);
						}
						pthread_mutex_unlock(&sem_mutex_bitarray_lineas);

						log_info(logger, "Pude realocar el segmento %d del PID: %d. Nueva base: %d, nuevo limite: %d", fila_tabla->nro_seg, proceso->pid, fila_tabla->base, fila_tabla->limite);
						pthread_mutex_unlock(&sem_mutex_realocacion_en_progreso);
						return;
					}
				}
			} // Fin for bitarray

			/* No pude realocar */
			log_error(logger, "No hay suficiente espacio contiguo para realocar");
			/* Restauro el bitarray */
			for(j = 0; j <= fila_tabla->limite; j++){
				bitarray_set_bit(bitarray_lineas, fila_tabla->base + j);
			}
			pthread_mutex_unlock(&sem_mutex_bitarray_lineas);

			*ok = FM9_ERROR_INSUFICIENTE_ESPACIO;
		break;
	} // Fin switch(modo)
	pthread_mutex_unlock(&sem_mutex_realocacion_en_progreso);
}

void fm9_storage_escribir(unsigned int id, int base, int offset, char* str, int* ok){
	int dir_fisica_lineas = fm9_dir_logica_a_fisica(id, base, offset, ok);
	if(*ok != OK) {
		log_error(logger, "Seg fault al traducir una direccion (si fue DAM, todo bien)");
		return;
	}

	void* dir_fisica_bytes = (void*) (storage + (dir_fisica_lineas * max_linea));

	log_info(logger, "Escribo %s en %d", str, dir_fisica_lineas);

	memcpy(dir_fisica_bytes, (void*) str, strlen(str) + 1);
}

char* fm9_storage_leer(unsigned int id, int base, int offset, int* ok){
	int dir_fisica_lineas = fm9_dir_logica_a_fisica(id, base, offset, ok);
	if(*ok != OK) {
		log_error(logger, "Seg fault al traducir una direccion (si fue DAM, todo bien)");
		return NULL;
	}

	void* dir_fisica_bytes = (void*) (storage + (dir_fisica_lineas * max_linea));

	char* ret = malloc(max_linea);
	memcpy((void*) ret, dir_fisica_bytes, max_linea);
	return ret;
}

void fm9_exit(){
	free(storage);
	close(listening_socket);
	log_destroy(logger);
	config_destroy(config);
}
