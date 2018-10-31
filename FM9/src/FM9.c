#include "FM9.h"

				void __huecos_print_y_log(){ // TODO Sacarlo despues, esto sirve para debuggear
					char* huecos_str = string_new(), *aux;
					int i;

					log_info(logger, "Lista de huecos final: ");

					for(i = 0; i < list_size(lista_huecos_storage); i++){
						t_vector2* hueco = (t_vector2*) list_get(lista_huecos_storage, i);
						string_append(&huecos_str, "(");
						aux = string_itoa(hueco->x);
						string_append(&huecos_str, aux);
						free(aux);
						string_append(&huecos_str, ",");
						aux = string_itoa(hueco->y);
						string_append(&huecos_str, aux);
						free(aux);
						string_append(&huecos_str, ")");
						string_append(&huecos_str, ",");
					}
					if(i > 0) huecos_str[strlen(huecos_str) - 1] = '\0';

					log_info(logger, "\t[%s]", huecos_str);
					free(huecos_str);
				}

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

int fm9_initialize(){

	void _modo_y_estr_administrativas_init(){
		switch(modo){
			case SEG:
				lista_procesos = list_create();
				pthread_mutex_init(&sem_mutex_lista_procesos, NULL);

				lista_huecos_storage = list_create();
				t_vector2* hueco_inicial = malloc(sizeof(t_vector2));
				hueco_inicial->x = 0;
				hueco_inicial->y = storage_cant_lineas - 1;
				list_add(lista_huecos_storage, hueco_inicial);
				__huecos_print_y_log();
				pthread_mutex_init(&sem_mutex_lista_huecos_storage, NULL);

				fm9_dir_logica_a_fisica = &_fm9_dir_logica_a_fisica_seg_pura;
				fm9_dump_pid = &_fm9_dump_pid_seg_pura;
				fm9_close = &_fm9_close_seg_pura;
				fm9_liberar_memoria_proceso = &_fm9_liberar_memoria_proceso_seg_pura;
			break;

			case TPI:
				bitMap1=calloc(cant_marcos,1);

				bitmapPaginacion= bitarray_create_with_mode(bitMap1,cant_marcos,MSB_FIRST);

				tablaPaginasInvertida=calloc(cant_marcos,sizeof(t_fila_tabla_paginas_invertida));
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
	int ret, ok, base;
	char* str;
	bool str_free = false;

 	va_list arguments;
	va_start(arguments, tipo_msg);

	switch(tipo_msg){
		case RESULTADO_CREAR_FM9:
			ok = va_arg(arguments, int);
			base = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_resultado_crear_fm9(ok, base);
		break;

		case RESULTADO_GET_FM9:
			ok = va_arg(arguments, int);
			str = va_arg(arguments, char*);
			if(str == NULL){
				log_debug(logger, "fm9_send - resultado_get_fm9 str==NULL");
				str = strdup("");
				str_free = true;
			}
			log_debug(logger, "%d | %s", ok, str); //
			mensaje_a_enviar = empaquetar_resultado_get_fm9(ok, str);

			if(str_free) free(str);
		break;

		case RESULTADO_ESCRIBIR_FM9:
			ok = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_int(ok);
		break;

		case RESULTADO_RESERVAR_LINEA_FM9:
			ok = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_int(ok);
		break;

		case RESULTADO_CLOSE:
			ok = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_int(ok);
		break;

		case RESULTADO_LIBERAR_MEMORIA_FM9:
			mensaje_a_enviar = empaquetar_int(OK);
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
				int tamArchivo=msg->header->payload_size;
				base = fm9_storage_nuevo_archivo(id,tamArchivo ,&operacion_ok);

				if(operacion_ok == FM9_ERROR_INSUFICIENTE_ESPACIO)
					operacion_ok = ERROR_ABRIR_ESPACIO_INSUFICIENTE_FM9;

				fm9_send(socket, RESULTADO_CREAR_FM9, operacion_ok, base);

			break;

			case ESCRIBIR_FM9:
				desempaquetar_escribir_fm9(msg, &id, &base, &offset, &datos);
				log_info(logger,"DAM me pidio la operacion ESCRIBIR %s del ID: %d con base: %d y offset: %d", datos, id, base, offset);

				fm9_storage_escribir(id, base, offset, datos, &operacion_ok, true);

				if(operacion_ok != OK){ // Intento realocar lo que tengo del archivo a otro lugar mas grande
					fm9_storage_realocar(id, base, offset, &operacion_ok);
					if(operacion_ok == OK){
						fm9_storage_escribir(id, base, offset, datos, &operacion_ok, true);
					}
				}
				free(datos);
				if(operacion_ok != OK) operacion_ok = ERROR_ABRIR_ESPACIO_INSUFICIENTE_FM9;
				fm9_send(socket, RESULTADO_ESCRIBIR_FM9, operacion_ok);
			break;

			case GET_FM9:
				desempaquetar_get_fm9(msg, &id, &base, &offset);
				log_info(logger,"DAM me pidio la operacion GET del ID: %d con base: %d y offset: %d", id, base, offset);

				datos = fm9_storage_leer(id, base, offset, &operacion_ok, true);

				fm9_send(socket, RESULTADO_GET_FM9, operacion_ok, datos);
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

				fm9_storage_escribir(id, base, offset, datos, &operacion_ok, false);
				free(datos);
				if(operacion_ok == FM9_ERROR_SEG_FAULT){
					operacion_ok = ERROR_ASIGNAR_FALLO_SEGMENTO;
				}
				fm9_send(socket, RESULTADO_ESCRIBIR_FM9, operacion_ok);
			break;

			case GET_FM9:
				desempaquetar_get_fm9(msg, &id, &base, &offset);
				log_info(logger,"CPU me pidio la operacion GET del ID: %d con base: %d y offset: %d", id, base, offset);

				datos = fm9_storage_leer(id, base, offset, &operacion_ok, false);
				if(datos != NULL)
					log_info(logger, "Le envio a CPU: %s", datos);

				fm9_send(socket, RESULTADO_GET_FM9, operacion_ok, datos);
				if(datos != NULL) free(datos);
			break;

			case CLOSE:
				desempaquetar_close(msg, &id, &base);
				log_info(logger,"CPU me pidio liberar el archivo de ID: %d con base: %d", id, base);

				fm9_close(id, base, &operacion_ok);
				if(operacion_ok == FM9_ERROR_SEG_FAULT){
					operacion_ok = ERROR_CLOSE_FALLO_SEGMENTO;
				}
				fm9_send(socket, RESULTADO_CLOSE, operacion_ok);
			break;

			case LIBERAR_MEMORIA_FM9:
				id = (unsigned int) desempaquetar_int(msg);
				log_info(logger, "CPU me pidio liberar la memoria usada por el proceso %d", id);
				fm9_liberar_memoria_proceso(id);
				fm9_send(socket, RESULTADO_LIBERAR_MEMORIA_FM9);
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

int fm9_storage_nuevo_archivo(unsigned int id,int tamArchivo ,int* ok){

	bool _mismo_pid(void* proceso){
		return ((t_proceso*) proceso)->pid == id;
	}

	*ok = OK;

	int nro_linea, ret_base;

	t_proceso* proceso;
	t_fila_tabla_segmento* nueva_fila_tabla;

	t_fila_tabla_paginas_invertida* nueva_fila_tablaDePaginas;

	switch(modo){
		case SEG:
			pthread_mutex_lock(&sem_mutex_lista_huecos_storage);
			nro_linea = _fm9_best_fit_seg_pura(1);
			__huecos_print_y_log();
			pthread_mutex_unlock(&sem_mutex_lista_huecos_storage);

			if(nro_linea == -1){
				*ok = FM9_ERROR_INSUFICIENTE_ESPACIO;
				return 0;
			}

			pthread_mutex_lock(&sem_mutex_lista_procesos);
			if((proceso = list_find(lista_procesos, _mismo_pid)) == NULL){ // Este proceso no existia en la lista
				proceso = malloc(sizeof(t_proceso));
				proceso->pid = id;
				proceso->lista_tabla_segmentos = list_create();
				list_add(lista_procesos, proceso);
			}


			nueva_fila_tabla = malloc(sizeof(t_fila_tabla_segmento));

			if(proceso->lista_tabla_segmentos->elements_count >= 1){
				nueva_fila_tabla->nro_seg = ((t_fila_tabla_segmento*) list_get(proceso->lista_tabla_segmentos, proceso->lista_tabla_segmentos->elements_count - 1))->nro_seg + 1;
			}else{
				nueva_fila_tabla->nro_seg = 0;
			}

			nueva_fila_tabla->base = nro_linea;
			nueva_fila_tabla->limite = 0;
			list_add(proceso->lista_tabla_segmentos, nueva_fila_tabla);

			ret_base = nueva_fila_tabla->nro_seg;

			log_info(logger, "Agrego el segmento %d del PID: %d, Base: %d, Limite: %d",
					nueva_fila_tabla->nro_seg, proceso->pid, nueva_fila_tabla->base, nueva_fila_tabla->limite);
			pthread_mutex_unlock(&sem_mutex_lista_procesos);
		break;

		case TPI:

			nueva_fila_tablaDePaginas = malloc(sizeof(t_fila_tabla_paginas_invertida));
			int cantPaginas=cantidad_paginas_del_archivo(tamArchivo);
			int marco_libre;
			int j=0;
			if(cantPaginas<marcos_disponibles()){
				while(j<cantPaginas){
					//mutex lock
					marco_libre=obtener_Marco_Libre();
					nueva_fila_tablaDePaginas->nro_marco=marco_libre;
					nueva_fila_tablaDePaginas->pid=id;
					nueva_fila_tablaDePaginas->nro_pagina=j;
					list_add(tablaPaginasInvertida,nueva_fila_tablaDePaginas);

					bitarray_set_bit(bitmapPaginacion,marco_libre);
					//mutex unlock
					log_info(logger, "Agrego la pagina %d del PID: %d, al Marco: %d",
										nueva_fila_tablaDePaginas->nro_pagina, nueva_fila_tablaDePaginas->pid, nueva_fila_tablaDePaginas->nro_marco);
					j++;
				}

			}
			else{
				*ok = FM9_ERROR_INSUFICIENTE_ESPACIO;
			}

		break;

		case SPA:
		break;
	}


	*ok = OK;
	return ret_base;
}

void fm9_storage_realocar(unsigned int id, int base, int offset, int* ok){

	bool _mismo_nro_seg(void* fila_tabla){
		return ((t_fila_tabla_segmento*) fila_tabla)->nro_seg == base;
	}

	bool _mismo_id(void* proceso){
		return ((t_proceso*) proceso)->pid == id;
	}

	*ok = OK;
	t_proceso* proceso;
	t_fila_tabla_segmento* fila_tabla;
	t_list* backup_lineas, *backup_dir_logicas;
	int i, lineas_contiguas_disponibles = 0, base_nuevo_segmento = 0, ret_dir_logica, dir_logica_aux;
	char* linea;

	switch(modo){
		case SEG:
			pthread_mutex_lock(&sem_mutex_lista_procesos);
			proceso = list_find(lista_procesos, _mismo_id);

			if(proceso == NULL){
				pthread_mutex_unlock(&sem_mutex_lista_procesos);
				log_error(logger, "No se encontro el proceso al realocar");
				*ok = FM9_ERROR_NO_ENCONTRADO_EN_ESTR_ADM;
				return;
			}
			fila_tabla = list_find(proceso->lista_tabla_segmentos, _mismo_nro_seg);
			if(fila_tabla == NULL){
				pthread_mutex_unlock(&sem_mutex_lista_procesos);
				log_error(logger, "No se encontro el segmento al realocar");
				*ok = FM9_ERROR_NO_ENCONTRADO_EN_ESTR_ADM;
				return;
			}
			log_info(logger, "Intento realocar el segmento %d del PID: %d con base %d, de %d a %d lineas", fila_tabla->nro_seg, proceso->pid, fila_tabla->base, fila_tabla->limite + 1, offset + 1);


			pthread_mutex_lock(&sem_mutex_lista_huecos_storage);
			log_info(logger, "Antes de liberar y best fit:"); // TODO Sacar
			__huecos_print_y_log();// TODO Sacar
			_fm9_nuevo_hueco_disponible_seg_pura(fila_tabla->base, fila_tabla->base + fila_tabla->limite);
			base_nuevo_segmento = _fm9_best_fit_seg_pura(offset + 1);
			__huecos_print_y_log();
			pthread_mutex_unlock(&sem_mutex_lista_huecos_storage);

			if(base_nuevo_segmento == -1){
				log_error(logger, "No hay suficiente espacio contiguo para realocar");

				/* Elimino las estr administrativas del segmento (el storage ya habia sido liberado) */
				list_remove_and_destroy_by_condition(proceso->lista_tabla_segmentos, _mismo_nro_seg, free);

				pthread_mutex_unlock(&sem_mutex_lista_procesos);
				*ok = FM9_ERROR_INSUFICIENTE_ESPACIO;
				return;

			}

			/* Ok, pude realocar */
			/* Me guardo las lineas que ya estaban desde antes */
			backup_lineas = list_create();
			pthread_mutex_unlock(&sem_mutex_lista_procesos);
			for(i = 0; i <= fila_tabla->limite; i++){
				list_add(backup_lineas, (void*) fm9_storage_leer(id, (int) fila_tabla->nro_seg, i , ok, true));
			}

			/* Actualizo tabla de segmentos */
			fila_tabla->base = base_nuevo_segmento;
			fila_tabla->limite = offset;

			/* Escribo las lineas guardadas */
			for(i = 0; i<backup_lineas->elements_count; i++){
				fm9_storage_escribir(id, (int) fila_tabla->nro_seg, i, (char*) list_get(backup_lineas, i), ok, true);
			}
			list_destroy_and_destroy_elements(backup_lineas, free);

			log_info(logger, "Pude realocar el segmento %d del PID: %d. Nueva base: %d, nuevo limite: %d", fila_tabla->nro_seg, proceso->pid, fila_tabla->base, fila_tabla->limite);
		break;

		case SPA:
		break;

		case TPI:
		break;

	} // Fin switch(modo)
}

void fm9_storage_escribir(unsigned int id, int base,  int offset, char* str, int* ok, bool permisos_totales){
	int dir_fisica = fm9_dir_logica_a_fisica(id, base, offset, ok);
	if(*ok != OK) {
		if(!permisos_totales)
			log_error(logger, "Seg fault al traducir una direccion");
		return;
	}

	void* dir_fisica_bytes = (void*) (storage + (dir_fisica * max_linea));

	log_info(logger, "Escribo %s en %d", str, dir_fisica);

	memcpy(dir_fisica_bytes, (void*) str, strlen(str) + 1);
}

char* fm9_storage_leer(unsigned int id, int base, int offset, int* ok, bool permisos_totales){
	int dir_fisica = fm9_dir_logica_a_fisica(id, base, offset, ok);
	if(*ok != OK) {
		if(!permisos_totales)
			log_error(logger, "Seg fault al traducir una direccion");
		return NULL;
	}

	void* dir_fisica_bytes = (void*) (storage + (dir_fisica * max_linea));

	char* ret = malloc(max_linea);
	memcpy((void*) ret, dir_fisica_bytes, max_linea);
	return ret;
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

	if(proceso == NULL) {
		*ok = FM9_ERROR_SEG_FAULT;
		pthread_mutex_unlock(&sem_mutex_lista_procesos);
		return -1;
	}

	t_fila_tabla_segmento* fila_tabla = list_find(proceso->lista_tabla_segmentos, _mismo_nro_seg);
	if(fila_tabla == NULL) {
		*ok = FM9_ERROR_SEG_FAULT;
		pthread_mutex_unlock(&sem_mutex_lista_procesos);
		return -1;
	}

	if(offset > fila_tabla->limite) {
		*ok = FM9_ERROR_SEG_FAULT;
		pthread_mutex_unlock(&sem_mutex_lista_procesos);
		return -1;
	}

	*ok = OK;
	int ret = fila_tabla->base + offset;
	pthread_mutex_unlock(&sem_mutex_lista_procesos);
	return ret;
}

int _fm9_best_fit_seg_pura(int cant_lineas){
	int tam_mejor_hueco, i, ret = -1, indice_ret;
	bool algun_hueco_encontrado = false;

	if(list_is_empty(lista_huecos_storage)) return -1;

	for(i = 0; i < list_size(lista_huecos_storage); i++){
		t_vector2* hueco_actual = (t_vector2*) list_get(lista_huecos_storage, i);
		int tam_hueco_actual = hueco_actual->y - hueco_actual->x + 1;

		if((!algun_hueco_encontrado || tam_hueco_actual < tam_mejor_hueco) && tam_hueco_actual >= cant_lineas){
			algun_hueco_encontrado = true;
			tam_mejor_hueco = tam_hueco_actual;
			ret = hueco_actual->x;
			indice_ret = i;
		}
	}

	if(ret == -1){
		return -1;
	}
	t_vector2* hueco_a_actualizar = (t_vector2*) list_get(lista_huecos_storage, indice_ret);
	hueco_a_actualizar->x += cant_lineas;
	if(hueco_a_actualizar->x > hueco_a_actualizar->y){ // El hueco se lleno por completo
		list_remove_and_destroy_element(lista_huecos_storage, indice_ret, free);
	}

	return ret;
}

void _fm9_nuevo_hueco_disponible_seg_pura(int linea_inicio, int linea_fin){

	bool _criterio_orden_huecos(void* hueco1, void* hueco2){
		return ((t_vector2*) hueco1)->x < ((t_vector2*) hueco2)->x;
	}

	int i;
	t_list* lista_indices_huecos_a_eliminar = list_create();

	/* Me expropio los huecos contiguos, si es que hay */
	for(i = 0; i < list_size(lista_huecos_storage); i++){
		t_vector2* hueco = (t_vector2*) list_get(lista_huecos_storage, i);
		if(linea_inicio == hueco->y + 1){
			linea_inicio = hueco->x;
			list_add(lista_indices_huecos_a_eliminar, (void*) i);
		}
		else if(linea_fin == hueco->x - 1){
			linea_fin = hueco->y;
			list_add(lista_indices_huecos_a_eliminar, (void*) i);
		}
	}

	/* Elimino los huecos que me expropie */
	for(i = 0; i < list_size(lista_indices_huecos_a_eliminar); i++){
		int indice_hueco_a_eliminar = (int) list_get(lista_indices_huecos_a_eliminar, i);
		t_vector2* v2 = (t_vector2*) list_get(lista_huecos_storage, indice_hueco_a_eliminar - i);
		log_info(logger, "~Elimino hueco numero %d, que contenia (%d, %d)", indice_hueco_a_eliminar, v2->x, v2->y);
		list_remove_and_destroy_element(lista_huecos_storage,  indice_hueco_a_eliminar - i, free);
	}

	/* Creo el hueco */
	t_vector2* nuevo_hueco = malloc(sizeof(t_vector2));
	nuevo_hueco->x = linea_inicio;
	nuevo_hueco->y = linea_fin;
	list_add(lista_huecos_storage, nuevo_hueco);
	log_info(logger, "~Agrego hueco (%d, %d)", linea_inicio, linea_fin);
	list_sort(lista_huecos_storage, _criterio_orden_huecos);

	list_destroy(lista_indices_huecos_a_eliminar);
}

void _fm9_close_seg_pura(unsigned int pid, int nro_seg, int* ok){

	bool _mismo_pid(void* proceso){
		return ((t_proceso*) proceso)->pid == pid;
	}

	bool _mismo_nro_seg(void* fila_tabla){
		return ((t_fila_tabla_segmento*) fila_tabla)->nro_seg == nro_seg;
	}


	pthread_mutex_lock(&sem_mutex_lista_procesos);
	t_proceso* proceso = list_find(lista_procesos, _mismo_pid);

	if(proceso == NULL){
		*ok = FM9_ERROR_SEG_FAULT;
		pthread_mutex_unlock(&sem_mutex_lista_procesos);
		return;
	}

	t_fila_tabla_segmento* fila_tabla = list_find(proceso->lista_tabla_segmentos, _mismo_nro_seg);

	if(fila_tabla == NULL){
		*ok = FM9_ERROR_SEG_FAULT;
		pthread_mutex_unlock(&sem_mutex_lista_procesos);
		return;
	}

	*ok = OK;

	pthread_mutex_lock(&sem_mutex_lista_huecos_storage);
	_fm9_nuevo_hueco_disponible_seg_pura(fila_tabla->base, fila_tabla->base + fila_tabla->limite);
	pthread_mutex_unlock(&sem_mutex_lista_huecos_storage);

	list_remove_and_destroy_by_condition(proceso->lista_tabla_segmentos, _mismo_nro_seg, free);

	pthread_mutex_unlock(&sem_mutex_lista_procesos);
}

void _fm9_liberar_memoria_proceso_seg_pura(unsigned int pid){

	bool _mismo_pid(void* proceso){
		return ((t_proceso*) proceso)->pid == pid;
	}

	void _liberar_memoria_segmento(void* _fila){
		t_fila_tabla_segmento* fila = (t_fila_tabla_segmento*) _fila;
		_fm9_nuevo_hueco_disponible_seg_pura(fila->base, fila->base + fila->limite);
		free(_fila);
	}

	pthread_mutex_lock(&sem_mutex_lista_procesos);
	t_proceso* proceso = (t_proceso*) list_remove_by_condition(lista_procesos, _mismo_pid);
	if(proceso == NULL) return;

	pthread_mutex_lock(&sem_mutex_lista_huecos_storage);
	list_destroy_and_destroy_elements(proceso->lista_tabla_segmentos, _liberar_memoria_segmento);
	pthread_mutex_unlock(&sem_mutex_lista_huecos_storage);

	free(proceso);

	pthread_mutex_unlock(&sem_mutex_lista_procesos);
}
//--------------paginacion---------------
int obtener_Marco_Libre(){
	int i=0;
	int bit_no_encontrado=1;
	while(i < cant_marcos  && bit_no_encontrado){
		if(bitarray_test_bit(bitmapPaginacion,i)==0){
			bit_no_encontrado=0;
		}else{
			i++;
		}

	}
	if(bit_no_encontrado){
		return -1;
	}else{return i;}

}
int marcos_disponibles(){

	int i,j=0;
	for(i=0;i<cant_marcos;i++){
		if(bitarray_test_bit(bitmapPaginacion,i)==0){
			j++;
		}
	}
	return j;
}
int cantidad_paginas_del_archivo(int tamArchivo){
	int x=0;
	if(tamArchivo%tam_pagina!=0){
		x=tamArchivo/tam_pagina+1;
	}else{
		x=tamArchivo/tam_pagina;
	}
	return x;
}
//--------------------------------
void fm9_exit(){
	free(storage);
	close(listening_socket);
	log_destroy(logger);
	config_destroy(config);
}
