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

	t_list* _TPI_crear_tabla(){
		t_list* ret = list_create();
		int i;

		void __agregar_nueva_fila(){
			t_fila_tabla_paginas_invertida* nueva_fila = malloc(sizeof(t_fila_tabla_paginas_invertida));
			nueva_fila->bit_validez = 0; // Este frame esta vacio inicialmente
			nueva_fila->nro_pagina = 0;
			nueva_fila->pid = 0;
			list_add(ret, (void*) nueva_fila);
		}

		for(i = 0; i < cant_frames; i++){
			__agregar_nueva_fila();
		}

		return ret;
	}


	void _modo_y_estr_administrativas_init(){
		char* bitMapStr;

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

				fm9_dir_logica_a_fisica = &_SEG_dir_logica_a_fisica;
				fm9_dump_pid = &_SEG_dump_pid;
				fm9_dump_estructuras = &_SEG_dump_estructuras;
				fm9_close = &_SEG_close;
				fm9_liberar_memoria_proceso = &_SEG_liberar_memoria_proceso;
			break;

			case TPI:
				bitMapStr = calloc(ceiling(cant_frames, 8), 1);
				bitmap_frames= bitarray_create_with_mode(bitMapStr, ceiling(cant_frames, 8), MSB_FIRST);
				pthread_mutex_init(&sem_mutex_bitmap_frames, NULL);

				tabla_paginas_invertida = _TPI_crear_tabla();
				pthread_mutex_init(&sem_mutex_tabla_paginas_invertida, NULL);

				tabla_archivos_TPI = list_create();
				pthread_mutex_init(&sem_mutex_tabla_archivos_TPI, NULL);

				fm9_dir_logica_a_fisica = &_TPI_dir_logica_a_fisica;
				fm9_dump_pid = &_TPI_dump_pid;
				fm9_dump_estructuras = &_TPI_dump_estructuras;
				fm9_close = &_TPI_close;
				fm9_liberar_memoria_proceso = &_TPI_liberar_memoria_proceso;
			break;

			case SPA:
				lista_procesos = list_create();
				pthread_mutex_init(&sem_mutex_lista_procesos, NULL);

				bitMapStr = calloc(ceiling(cant_frames, 8), 1);
				bitmap_frames= bitarray_create_with_mode(bitMapStr, ceiling(cant_frames, 8), MSB_FIRST);
				pthread_mutex_init(&sem_mutex_bitmap_frames, NULL);

				fm9_dir_logica_a_fisica = &_SPA_dir_logica_a_fisica;
				fm9_dump_pid = &_SPA_dump_pid;
				fm9_dump_estructuras = &_SPA_dump_estructuras;
				fm9_close = &_SPA_close;
				fm9_liberar_memoria_proceso = &_SPA_liberar_memoria_proceso;
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
	tam_frame_bytes = config_get_int_value(config, "TAM_PAGINA");
	tam_frame_lineas = tam_frame_bytes / max_linea;
	cant_frames = tamanio / tam_frame_bytes;
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
				str = strdup(" ");
				str_free = true;
			}
			log_debug(logger, "%d | [%s] len: %d", ok, str, strlen(str)); //
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
				base = fm9_storage_nuevo_archivo(id, &operacion_ok);

				if(operacion_ok == FM9_ERROR_INSUFICIENTE_ESPACIO)
					operacion_ok = ERROR_ABRIR_ESPACIO_INSUFICIENTE_FM9;

				fm9_send(socket, RESULTADO_CREAR_FM9, operacion_ok, base);

			break;

			case ESCRIBIR_FM9:
				desempaquetar_escribir_fm9(msg, &id, &base, &offset, &datos);
				offset--; // [IMPORTANTE] Le resto 1 ya que las lineas comienzan en 1 y yo quiero que empiecen en 0 (v1.5)
				log_info(logger,"DAM me pidio la operacion ESCRIBIR %s del ID: %d con base: %d y offset: %d", datos, id, base, offset);

				fm9_storage_escribir(id, base, offset, datos, &operacion_ok, true);

				if(operacion_ok != OK){ // Intento realocar lo que tengo del archivo a otro lugar mas grande
					fm9_storage_realocar(id, base, offset, &operacion_ok);
					if(operacion_ok == OK){
						fm9_storage_escribir(id, base, offset, datos, &operacion_ok, true);
					}
				}
				free(datos);
				if(operacion_ok != OK)
					operacion_ok = ERROR_ABRIR_ESPACIO_INSUFICIENTE_FM9;

				fm9_send(socket, RESULTADO_ESCRIBIR_FM9, operacion_ok);
			break;

			case GET_FM9:
				desempaquetar_get_fm9(msg, &id, &base, &offset);
				offset--; // [IMPORTANTE] Le resto 1 ya que las lineas comienzan en 1 y yo quiero que empiecen en 0 (v1.5)
				log_info(logger,"DAM me pidio la operacion GET del ID: %d con base: %d y offset: %d", id, base, offset);

				datos = fm9_storage_leer(id, base, offset, &operacion_ok, false); // false para que considere la FI de la ult pag

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
				offset--; // [IMPORTANTE] Le resto 1 ya que las lineas comienzan en 1 y yo quiero que empiecen en 0 (v1.5)
				log_info(logger,"CPU me pidio la operacion ESCRIBIR (asignar) %s del ID: %d con base: %d y offset: %d", datos, id, base, offset);

				fm9_storage_escribir(id, base, offset, datos, &operacion_ok, false);
				free(datos);
				if(operacion_ok != OK){
					operacion_ok = ERROR_ASIGNAR_FALLO_SEGMENTO;
				}
				fm9_send(socket, RESULTADO_ESCRIBIR_FM9, operacion_ok);
			break;

			case GET_FM9:
				desempaquetar_get_fm9(msg, &id, &base, &offset);
				offset--; // [IMPORTANTE] Le resto 1 ya que las lineas comienzan en 1 y yo quiero que empiecen en 0 (v1.5)
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
				if(operacion_ok != OK){
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

int fm9_storage_nuevo_archivo(unsigned int id, int* ok){

	bool _mismo_pid(void* proceso){
		return ((t_proceso*) proceso)->pid == id;
	}

	*ok = OK;

	int ret_base;

	t_proceso* proceso;
	t_fila_tabla_segmento_SEG* nueva_fila_tabla_SEG;

	t_fila_tabla_paginas_invertida* fila_TPI;

	t_fila_tabla_segmento_SPA* nueva_fila_tabla_segmentos_SPA;
	t_fila_tabla_paginas_SPA* nueva_fila_tabla_paginas_SPA;

	switch(modo){
		case SEG:
			pthread_mutex_lock(&sem_mutex_lista_huecos_storage);
			int nro_linea = _SEG_best_fit(1);
			__huecos_print_y_log();
			pthread_mutex_unlock(&sem_mutex_lista_huecos_storage);

			if(nro_linea == -1){
				log_error(logger, "No hay espacio para un nuevo archivo");
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


			nueva_fila_tabla_SEG = malloc(sizeof(t_fila_tabla_segmento_SEG));

			/* Obtengo el nro de segmento */
			if(proceso->lista_tabla_segmentos->elements_count >= 1){
				nueva_fila_tabla_SEG->nro_seg = ((t_fila_tabla_segmento_SEG*) list_get(proceso->lista_tabla_segmentos, proceso->lista_tabla_segmentos->elements_count - 1))->nro_seg + 1;
			}else{
				nueva_fila_tabla_SEG->nro_seg = 0;
			}

			nueva_fila_tabla_SEG->base = nro_linea;
			nueva_fila_tabla_SEG->limite = 0;
			list_add(proceso->lista_tabla_segmentos, nueva_fila_tabla_SEG);

			ret_base = nueva_fila_tabla_SEG->nro_seg;

			log_info(logger, "Agrego el segmento %d del PID: %d, Base: %d, Limite: %d",
					nueva_fila_tabla_SEG->nro_seg, proceso->pid, nueva_fila_tabla_SEG->base, nueva_fila_tabla_SEG->limite);
			pthread_mutex_unlock(&sem_mutex_lista_procesos);
		break;

		case TPI:
			pthread_mutex_lock(&sem_mutex_bitmap_frames);
			int nro_frame_seleccionado = _TPI_SPA_obtener_frame_libre();
			pthread_mutex_unlock(&sem_mutex_bitmap_frames);

			if(nro_frame_seleccionado == -1){
				log_error(logger, "No hay espacio para un nuevo archivo");
				*ok = FM9_ERROR_INSUFICIENTE_ESPACIO;
				return 0;
			}

			pthread_mutex_lock(&sem_mutex_tabla_paginas_invertida);
			fila_TPI = (t_fila_tabla_paginas_invertida*) list_get(tabla_paginas_invertida, nro_frame_seleccionado);
			fila_TPI->pid = id;
			fila_TPI->bit_validez = 1;
			int nro_pagina_base = _TPI_primera_pagina_disponible_proceso(id);
			fila_TPI->nro_pagina = nro_pagina_base;
			pthread_mutex_unlock(&sem_mutex_tabla_paginas_invertida);

			log_info(logger, "Agrego la pagina %d del PID: %d, al frame: %d",
					nro_pagina_base, id, nro_frame_seleccionado);

			t_fila_TPI_archivos* nueva_entrada_archivos_TPI = malloc(sizeof(t_fila_TPI_archivos));
			nueva_entrada_archivos_TPI->pid = id;
			nueva_entrada_archivos_TPI->nro_pag_inicial = nro_pagina_base;
			nueva_entrada_archivos_TPI->nro_pag_final = nueva_entrada_archivos_TPI->nro_pag_inicial;
			nueva_entrada_archivos_TPI->fragm_interna_ult_pagina = tam_frame_lineas; // Al principio, la FI es toda la pag
			log_debug(logger, "\t{nuevo_archivo} FI: %d", nueva_entrada_archivos_TPI->fragm_interna_ult_pagina);
			pthread_mutex_lock(&sem_mutex_tabla_archivos_TPI);
			list_add(tabla_archivos_TPI, nueva_entrada_archivos_TPI);
			pthread_mutex_unlock(&sem_mutex_tabla_archivos_TPI);

			ret_base = nro_pagina_base;
		break;

		case SPA:
			pthread_mutex_lock(&sem_mutex_bitmap_frames);
			int nro_frame = _TPI_SPA_obtener_frame_libre();
			pthread_mutex_unlock(&sem_mutex_bitmap_frames);

			if(nro_frame == -1){
				log_error(logger, "No hay espacio para un nuevo archivo");
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

			nueva_fila_tabla_segmentos_SPA = malloc(sizeof(t_fila_tabla_segmento_SPA));
			nueva_fila_tabla_segmentos_SPA->lista_tabla_paginas = list_create();
			nueva_fila_tabla_paginas_SPA = malloc(sizeof(t_fila_tabla_paginas_SPA));

			/* Obtengo el nro de segmento */
			if(proceso->lista_tabla_segmentos->elements_count >= 1){
				nueva_fila_tabla_segmentos_SPA->nro_seg = ((t_fila_tabla_segmento_SPA*) list_get(proceso->lista_tabla_segmentos, proceso->lista_tabla_segmentos->elements_count - 1))->nro_seg + 1;
			}else{
				nueva_fila_tabla_segmentos_SPA->nro_seg = 0;
			}

			nueva_fila_tabla_paginas_SPA->nro_pagina = 0;
			nueva_fila_tabla_paginas_SPA->nro_frame = nro_frame;

			list_add(nueva_fila_tabla_segmentos_SPA->lista_tabla_paginas, nueva_fila_tabla_paginas_SPA);
			list_add(proceso->lista_tabla_segmentos, nueva_fila_tabla_segmentos_SPA);

			ret_base = nueva_fila_tabla_segmentos_SPA->nro_seg;

			log_info(logger, "Agrego el segmento %d del PID: %d, 1ra pag: 0, 1er frame: %d",
					nueva_fila_tabla_segmentos_SPA->nro_seg, proceso->pid, nro_frame);
			pthread_mutex_unlock(&sem_mutex_lista_procesos);
		break;
	}


	*ok = OK;
	return ret_base;
}

void fm9_storage_realocar(unsigned int id, int base, int offset, int* ok){

	bool _mismo_id(void* proceso){
		return ((t_proceso*) proceso)->pid == id;
	}

	bool _mismo_nro_seg(void* fila_tabla){
		return ((t_fila_tabla_segmento_SEG*) fila_tabla)->nro_seg == base;
	}

	bool _mismo_nro_seg_SPA(void* fila_tabla){
		return ((t_fila_tabla_segmento_SPA*) fila_tabla)->nro_seg == base;
	}



	*ok = OK;

	t_proceso* proceso;
	t_fila_tabla_segmento_SEG* fila_tabla;

	t_fila_tabla_segmento_SPA* fila_tabla_segmento_SPA;
	t_fila_tabla_paginas_SPA* nueva_fila_tabla_paginas_SPA;

    t_fila_TPI_archivos* fila_tabla_archivos_TPI;
	t_fila_tabla_paginas_invertida* fila_TPI;

	t_list* backup_lineas, *backup_dir_logicas;
	int i, lineas_contiguas_disponibles = 0, base_nuevo_segmento = 0, ret_dir_logica, dir_logica_aux, nro_frame;
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
			_SEG_nuevo_hueco_disponible(fila_tabla->base, fila_tabla->base + fila_tabla->limite);
			base_nuevo_segmento = _SEG_best_fit(offset + 1);
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

		case TPI:
			/* Le agrego otra pagina */

			pthread_mutex_lock(&sem_mutex_bitmap_frames);
			nro_frame = _TPI_SPA_obtener_frame_libre();
			pthread_mutex_unlock(&sem_mutex_bitmap_frames);

			if(nro_frame == -1){
				log_error(logger, "No hay frames disponibles");
				*ok = FM9_ERROR_INSUFICIENTE_ESPACIO;
				return;
			}

			pthread_mutex_lock(&sem_mutex_tabla_archivos_TPI);
			int nro_pag_nueva = _TPI_incrementar_ultima_pagina_archivo_de_proceso(id, base);
			pthread_mutex_unlock(&sem_mutex_tabla_archivos_TPI);

			pthread_mutex_lock(&sem_mutex_tabla_paginas_invertida);
			fila_TPI = (t_fila_tabla_paginas_invertida*) list_get(tabla_paginas_invertida, nro_frame);
			fila_TPI->pid = id;
			fila_TPI->bit_validez = 1;
			fila_TPI->nro_pagina = nro_pag_nueva;
			pthread_mutex_unlock(&sem_mutex_tabla_paginas_invertida);

			log_info(logger, "Asigno una pagina en la tabla invertida. PID: %d. Nro pag: %d, nro frame: %d",
					id, nro_pag_nueva, nro_frame);
		break;

		case SPA:
			/* Le agrego otra pagina dentro del segmento al proceso */
			pthread_mutex_lock(&sem_mutex_lista_procesos);
			proceso = list_find(lista_procesos, _mismo_id);
			if(proceso == NULL){
				pthread_mutex_unlock(&sem_mutex_lista_procesos);
				log_error(logger, "No se encontro el proceso al intentar agregar una pagina");
				*ok = FM9_ERROR_NO_ENCONTRADO_EN_ESTR_ADM;
				return;
			}

			fila_tabla_segmento_SPA = list_find(proceso->lista_tabla_segmentos, _mismo_nro_seg_SPA);
			if(fila_tabla_segmento_SPA == NULL){
				pthread_mutex_unlock(&sem_mutex_lista_procesos);
				log_error(logger, "No se encontro el segmento al intentar agregar una pagina");
				*ok = FM9_ERROR_NO_ENCONTRADO_EN_ESTR_ADM;
				return;
			}
			log_info(logger, "Le intento dar otra pagina al segmento %d del PID: %d", fila_tabla_segmento_SPA->nro_seg, proceso->pid);

			pthread_mutex_lock(&sem_mutex_bitmap_frames);
			nro_frame = _TPI_SPA_obtener_frame_libre();
			pthread_mutex_unlock(&sem_mutex_bitmap_frames);

			if(nro_frame == -1){
				pthread_mutex_unlock(&sem_mutex_lista_procesos);
				log_error(logger, "No hay frames disponibles");
				*ok = FM9_ERROR_INSUFICIENTE_ESPACIO;
				return;
			}

			nueva_fila_tabla_paginas_SPA = malloc(sizeof(t_fila_tabla_paginas_SPA));

			nueva_fila_tabla_paginas_SPA->nro_pagina = list_size(fila_tabla_segmento_SPA->lista_tabla_paginas) < 1 ? 0 :
					1 + ((t_fila_tabla_paginas_SPA*) list_get(fila_tabla_segmento_SPA->lista_tabla_paginas, list_size(fila_tabla_segmento_SPA->lista_tabla_paginas) - 1))->nro_pagina;
			nueva_fila_tabla_paginas_SPA->nro_frame = nro_frame;

			list_add(fila_tabla_segmento_SPA->lista_tabla_paginas, nueva_fila_tabla_paginas_SPA);
			log_info(logger, "Agrego una pagina al segmento %d del PID: %d. Nro pag: %d, nro frame: %d",
					fila_tabla_segmento_SPA->nro_seg, proceso->pid, nueva_fila_tabla_paginas_SPA->nro_pagina, nueva_fila_tabla_paginas_SPA->nro_frame);

			pthread_mutex_unlock(&sem_mutex_lista_procesos);
		break;
	} // Fin switch(modo)
}

void fm9_storage_escribir(unsigned int id, int base,  int offset, char* str, int* ok, bool permisos_totales){

	int dir_fisica = fm9_dir_logica_a_fisica(id, base, offset, ok, permisos_totales);
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

	int dir_fisica = fm9_dir_logica_a_fisica(id, base, offset, ok, permisos_totales);
	if(*ok != OK || offset < 0) {
		if(!permisos_totales)
			log_error(logger, "Seg fault al traducir una direccion");
		return NULL;
	}


	void* dir_fisica_bytes = (void*) (storage + (dir_fisica * max_linea));

	char* ret = malloc(max_linea);
	memcpy((void*) ret, dir_fisica_bytes, max_linea);

	if(!strcmp(ret, "")){
		log_debug(logger, "fm9_storage_leer - Linea vacia");
		free(ret);
		ret = strdup(" ");
	}

	return ret;
}

int _SEG_dir_logica_a_fisica(unsigned int pid, int nro_seg, int offset, int* ok, bool __permisos_totales){

	bool _mismo_pid(void* proceso){
		return ((t_proceso*) proceso)->pid == pid;
	}

	bool _mismo_nro_seg(void* fila_tabla){
		return ((t_fila_tabla_segmento_SEG*) fila_tabla)->nro_seg == nro_seg;
	}


	pthread_mutex_lock(&sem_mutex_lista_procesos);
	t_proceso* proceso = list_find(lista_procesos, _mismo_pid);

	if(proceso == NULL) {
		*ok = FM9_ERROR_NO_ENCONTRADO_EN_ESTR_ADM;
		pthread_mutex_unlock(&sem_mutex_lista_procesos);
		return -1;
	}

	t_fila_tabla_segmento_SEG* fila_tabla = list_find(proceso->lista_tabla_segmentos, _mismo_nro_seg);
	if(fila_tabla == NULL) {
		*ok = FM9_ERROR_NO_ENCONTRADO_EN_ESTR_ADM;
		pthread_mutex_unlock(&sem_mutex_lista_procesos);
		return -1;
	}

	if(offset > fila_tabla->limite || offset < 0) {
		*ok = FM9_ERROR_SEG_FAULT;
		pthread_mutex_unlock(&sem_mutex_lista_procesos);
		return -1;
	}

	*ok = OK;
	int ret = fila_tabla->base + offset;
	pthread_mutex_unlock(&sem_mutex_lista_procesos);
	return ret;
}

int _SEG_best_fit(int cant_lineas){
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

void _SEG_nuevo_hueco_disponible(int linea_inicio, int linea_fin){

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

void _SEG_close(unsigned int pid, int nro_seg, int* ok){

	bool _mismo_pid(void* proceso){
		return ((t_proceso*) proceso)->pid == pid;
	}

	bool _mismo_nro_seg(void* fila_tabla){
		return ((t_fila_tabla_segmento_SEG*) fila_tabla)->nro_seg == nro_seg;
	}


	pthread_mutex_lock(&sem_mutex_lista_procesos);
	t_proceso* proceso = list_find(lista_procesos, _mismo_pid);

	if(proceso == NULL){
		*ok = FM9_ERROR_NO_ENCONTRADO_EN_ESTR_ADM;
		pthread_mutex_unlock(&sem_mutex_lista_procesos);
		return;
	}

	t_fila_tabla_segmento_SEG* fila_tabla = list_find(proceso->lista_tabla_segmentos, _mismo_nro_seg);

	if(fila_tabla == NULL){
		*ok = FM9_ERROR_NO_ENCONTRADO_EN_ESTR_ADM;
		pthread_mutex_unlock(&sem_mutex_lista_procesos);
		return;
	}

	*ok = OK;

	pthread_mutex_lock(&sem_mutex_lista_huecos_storage);
	_SEG_nuevo_hueco_disponible(fila_tabla->base, fila_tabla->base + fila_tabla->limite);
	pthread_mutex_unlock(&sem_mutex_lista_huecos_storage);

	list_remove_and_destroy_by_condition(proceso->lista_tabla_segmentos, _mismo_nro_seg, free);

	pthread_mutex_unlock(&sem_mutex_lista_procesos);
}

void _SEG_liberar_memoria_proceso(unsigned int pid){

	bool _mismo_pid(void* proceso){
		return ((t_proceso*) proceso)->pid == pid;
	}

	void _liberar_memoria_segmento(void* _fila){
		t_fila_tabla_segmento_SEG* fila = (t_fila_tabla_segmento_SEG*) _fila;
		_SEG_nuevo_hueco_disponible(fila->base, fila->base + fila->limite);
		free(_fila);
	}

	pthread_mutex_lock(&sem_mutex_lista_procesos);
	t_proceso* proceso = (t_proceso*) list_remove_by_condition(lista_procesos, _mismo_pid);
	if(proceso == NULL) {
		pthread_mutex_unlock(&sem_mutex_lista_procesos);
		return;
	}

	pthread_mutex_lock(&sem_mutex_lista_huecos_storage);
	list_destroy_and_destroy_elements(proceso->lista_tabla_segmentos, _liberar_memoria_segmento);
	pthread_mutex_unlock(&sem_mutex_lista_huecos_storage);

	free(proceso);

	pthread_mutex_unlock(&sem_mutex_lista_procesos);
}


int _TPI_dir_logica_a_fisica(unsigned int pid, int pag_inicial, int offset, int* ok, bool permisos_totales){
	int nro_pagina_target = pag_inicial + (offset / tam_frame_lineas);
	int offset_en_pagina = offset % tam_frame_lineas;


	bool _mismo_pid_y_nro_pag(void* _pagina){
		t_fila_tabla_paginas_invertida* pagina = (t_fila_tabla_paginas_invertida*) _pagina;
		return pagina->pid == pid && pagina->bit_validez == 1 && pagina->nro_pagina == nro_pagina_target;
	}

	bool _archivo_mismo_pid_y_pag_inicial(void* _archivo){
		t_fila_TPI_archivos* archivo = (t_fila_TPI_archivos*) _archivo;
		return archivo->pid == pid && archivo->nro_pag_inicial == pag_inicial;
	}

	int list_get_index(t_list* list, bool (*closure)(void*)){
		int i;
		for(i = 0; i < list_size(list); i++) {
			void* elem = list_get(list, i);
			if(closure(elem)){
				return i;
			}
		}

		return -1;
	}


	pthread_mutex_lock(&sem_mutex_tabla_paginas_invertida);
	t_fila_tabla_paginas_invertida* fila_TPI = list_find(tabla_paginas_invertida, _mismo_pid_y_nro_pag);
	if(fila_TPI == NULL) {
		*ok = FM9_ERROR_SEG_FAULT;
		pthread_mutex_unlock(&sem_mutex_tabla_paginas_invertida);
		return -1;
	}

	/* Compruebo que esa pagina sea de ese archivo */
	pthread_mutex_lock(&sem_mutex_tabla_archivos_TPI);
	t_fila_TPI_archivos* archivo = list_find(tabla_archivos_TPI, _archivo_mismo_pid_y_pag_inicial);
	if(!(fila_TPI->nro_pagina >= archivo->nro_pag_inicial && fila_TPI->nro_pagina <= archivo->nro_pag_final)) {
		*ok = FM9_ERROR_SEG_FAULT;
		pthread_mutex_unlock(&sem_mutex_tabla_paginas_invertida);
		pthread_mutex_unlock(&sem_mutex_tabla_archivos_TPI);
		return -1;
	}
	if(archivo->nro_pag_final == nro_pagina_target){
		/* Es la ultima pagina */
		/* Chequeo la frag int */
		int ultima_linea_valida_en_pag = tam_frame_lineas - archivo->fragm_interna_ult_pagina - 1; // -1 por empezar a contar en 0
		log_debug(logger, "\tULTIMA PAGINA. Ultima_linea_valida_en_pag: %d. FI: %d", ultima_linea_valida_en_pag, archivo->fragm_interna_ult_pagina);
		if(offset_en_pagina > ultima_linea_valida_en_pag){
			/* Esta linea esta dentro de la fragmentacion interna */
			/* Segun los permisos, es error o la frag int disminuye*/
			if(!permisos_totales){ // CPU o DAM GET
				*ok = FM9_ERROR_SEG_FAULT;
				pthread_mutex_unlock(&sem_mutex_tabla_paginas_invertida);
				pthread_mutex_unlock(&sem_mutex_tabla_archivos_TPI);
				return -1;
			}
			else{ // DAM ESCRIBIR
				archivo->fragm_interna_ult_pagina = tam_frame_lineas - offset_en_pagina - 1;
				log_debug(logger, "\tNuevo FI: %d", archivo->fragm_interna_ult_pagina);
			}
		}
	}
	pthread_mutex_unlock(&sem_mutex_tabla_archivos_TPI);

	int nro_frame = list_get_index(tabla_paginas_invertida, _mismo_pid_y_nro_pag);
	pthread_mutex_unlock(&sem_mutex_tabla_paginas_invertida);

	*ok = OK;
	return (nro_frame * tam_frame_lineas) + offset_en_pagina;
}

void _TPI_close(unsigned int id, int pag_inicial, int* ok){
	int pag_arch_inicio = -1, pag_arch_fin = -1, nro_frame = 0;

	bool _misma_pag_inicial_y_pid(void* arch){
		return ((t_fila_TPI_archivos*) arch)->pid == id && ((t_fila_TPI_archivos*) arch)->nro_pag_inicial == pag_inicial;
	}

	void _limpiar_bit_validez_y_limpiar_en_bitmap(void* _pag){
		t_fila_tabla_paginas_invertida* pag = (t_fila_tabla_paginas_invertida*) _pag;
		if(pag->pid == id && pag->nro_pagina >= pag_arch_inicio && pag->nro_pagina <= pag_arch_fin){
			pag->bit_validez = 0;
			bitarray_clean_bit(bitmap_frames, nro_frame);
		}

		nro_frame++;
	}

	void _free_y_setear_limites_arch(void* arch){
		pag_arch_inicio = ((t_fila_TPI_archivos*) arch)->nro_pag_inicial;
		pag_arch_fin = ((t_fila_TPI_archivos*) arch)->nro_pag_final;
		free(arch);
	}


	pthread_mutex_lock(&sem_mutex_tabla_archivos_TPI);
	list_remove_and_destroy_by_condition(tabla_archivos_TPI, _misma_pag_inicial_y_pid, _free_y_setear_limites_arch);
	pthread_mutex_unlock(&sem_mutex_tabla_archivos_TPI);

	pthread_mutex_lock(&sem_mutex_tabla_paginas_invertida);
	pthread_mutex_lock(&sem_mutex_bitmap_frames);
	list_iterate(tabla_paginas_invertida, _limpiar_bit_validez_y_limpiar_en_bitmap);
	pthread_mutex_unlock(&sem_mutex_tabla_paginas_invertida);
	pthread_mutex_unlock(&sem_mutex_bitmap_frames);
}

void _TPI_liberar_memoria_proceso(unsigned int id){

	int nro_frame = 0;

	bool _mismo_pid(void* arch){
		return ((t_fila_TPI_archivos*) arch)->pid == id;
	}

	void list_remove_and_destroy_all_by_condition(t_list* list, bool (*condition)(void*), void (*elem_destroyer)(void*)){
		while((list_find(list, condition)) != NULL)
			list_remove_and_destroy_by_condition(list, condition, elem_destroyer);
	}

	void _limpiar_bit_validez_y_limpiar_en_bitmap(void* _pag){
		t_fila_tabla_paginas_invertida* pag = (t_fila_tabla_paginas_invertida*) _pag;
		if(pag->pid == id){
			pag->bit_validez = 0;
			bitarray_clean_bit(bitmap_frames, nro_frame);
		}

		nro_frame++;
	}


	pthread_mutex_lock(&sem_mutex_tabla_archivos_TPI);
	list_remove_and_destroy_all_by_condition(tabla_archivos_TPI, _mismo_pid, free);
	pthread_mutex_unlock(&sem_mutex_tabla_archivos_TPI);

	pthread_mutex_lock(&sem_mutex_tabla_paginas_invertida);
	pthread_mutex_lock(&sem_mutex_bitmap_frames);
	list_iterate(tabla_paginas_invertida, _limpiar_bit_validez_y_limpiar_en_bitmap);
	pthread_mutex_unlock(&sem_mutex_tabla_paginas_invertida);
	pthread_mutex_unlock(&sem_mutex_bitmap_frames);
}

unsigned int _TPI_incrementar_ultima_pagina_archivo_de_proceso(unsigned int pid, int pag_inicial){

	bool _mismo_pid_y_pag_inicial(void* _archivo){ // (para encontrar el archivo)
		t_fila_TPI_archivos* archivo = (t_fila_TPI_archivos*) _archivo;
		return archivo->pid == pid && archivo->nro_pag_inicial == pag_inicial;
	}


	t_fila_TPI_archivos* archivo = (t_fila_TPI_archivos*) list_find(tabla_archivos_TPI, _mismo_pid_y_pag_inicial);
	log_debug(logger, "\t{_TPI_incrementar_ultima_pagina_archivo_de_proceso} VIEJO FI: %d", archivo->fragm_interna_ult_pagina);
	archivo->fragm_interna_ult_pagina = tam_frame_lineas; // Reset
	log_debug(logger, "\t{_TPI_incrementar_ultima_pagina_archivo_de_proceso} NUEVO FI: %d", archivo->fragm_interna_ult_pagina);
	archivo->nro_pag_final++;
	return archivo->nro_pag_final; // Incremento y retorno
}

unsigned int _TPI_primera_pagina_disponible_proceso(unsigned int pid){

	unsigned int primera_pag_disponible = 0;

	bool _mismo_pid_TPI(void* arch){
		return ((t_fila_TPI_archivos*) arch)->pid == pid;
	}

	void* _nro_pag_mayor(void* arch1, void* arch2){
		if(arch1 == NULL) return arch2;
		return ((t_fila_TPI_archivos*) arch1)->nro_pag_inicial > ((t_fila_TPI_archivos*) arch2)->nro_pag_inicial ? arch1 : arch2;
	}


	pthread_mutex_lock(&sem_mutex_tabla_archivos_TPI);
	t_list* lista_archivos_pid = list_filter(tabla_archivos_TPI, _mismo_pid_TPI);

	t_fila_TPI_archivos* arch = (t_fila_TPI_archivos*) list_fold(lista_archivos_pid, NULL, _nro_pag_mayor);

	if(list_size(lista_archivos_pid) <= 0){ // El pid no tenia paginas
		log_debug(logger, "\t{_TPI_primera_pagina_disponible_proceso} NO TENIA PAGS");
		primera_pag_disponible = 0;
	}
	else {
		primera_pag_disponible = arch->nro_pag_final + 1;
		//log_debug(logger, "\t{_TPI_primera_pagina_disponible_proceso} FI antes: %d", arch->fragm_interna_ult_pagina);
		//arch->fragm_interna_ult_pagina = tam_frame_lineas; // Se resetea la FI ya que es una pag nueva en blanco
		//log_debug(logger, "\t{_TPI_primera_pagina_disponible_proceso} FI despues: %d", arch->fragm_interna_ult_pagina);
	}

	list_destroy(lista_archivos_pid);
	pthread_mutex_unlock(&sem_mutex_tabla_archivos_TPI);
	return primera_pag_disponible;
}


int _TPI_SPA_obtener_frame_libre(){
	int i = 0;
	bool frame_encontrado = false;

	while(i < cant_frames  && !frame_encontrado){
		if(bitarray_test_bit(bitmap_frames, i) == 0){ // Frame disponible
			bitarray_set_bit(bitmap_frames, i); // Lo selecciono como no disponible
			frame_encontrado = true;
		}
		else
			i++;
	}

	return frame_encontrado ? i : -1;
}


int _SPA_dir_logica_a_fisica(unsigned int pid, int nro_seg, int offset, int* ok, bool permisos_totales){

	int nro_pagina_target = offset / tam_frame_lineas;
	int offset_en_pagina = offset % tam_frame_lineas;

	bool _mismo_pid(void* proceso){
		return ((t_proceso*) proceso)->pid == pid;
	}

	bool _mismo_nro_seg(void* fila_tabla){
		return ((t_fila_tabla_segmento_SPA*) fila_tabla)->nro_seg == nro_seg;
	}

	bool _mismo_nro_pag(void* fila_tabla_paginas){
		return ((t_fila_tabla_paginas_SPA*) fila_tabla_paginas)->nro_pagina == nro_pagina_target;
	}

	pthread_mutex_lock(&sem_mutex_lista_procesos);
	t_proceso* proceso = list_find(lista_procesos, _mismo_pid);

	if(proceso == NULL) {
		*ok = FM9_ERROR_NO_ENCONTRADO_EN_ESTR_ADM;
		pthread_mutex_unlock(&sem_mutex_lista_procesos);
		return -1;
	}

	t_fila_tabla_segmento_SPA* fila_tabla = list_find(proceso->lista_tabla_segmentos, _mismo_nro_seg);
	if(fila_tabla == NULL) {
		*ok = FM9_ERROR_NO_ENCONTRADO_EN_ESTR_ADM;
		pthread_mutex_unlock(&sem_mutex_lista_procesos);
		return -1;
	}

	t_fila_tabla_paginas_SPA* fila_tabla_paginas = list_find(fila_tabla->lista_tabla_paginas, _mismo_nro_pag);
	if(fila_tabla_paginas == NULL) {
		*ok = FM9_ERROR_SEG_FAULT;
		pthread_mutex_unlock(&sem_mutex_lista_procesos);
		return -1;
	}

	*ok = OK;
	int ret = (fila_tabla_paginas->nro_frame * tam_frame_lineas) + offset_en_pagina;
	pthread_mutex_unlock(&sem_mutex_lista_procesos);
	return ret;
}

void _SPA_close(unsigned int pid, int nro_seg, int* ok){

	bool _mismo_pid(void* proceso){
		return ((t_proceso*) proceso)->pid == pid;
	}

	bool _mismo_nro_seg(void* fila_tabla){
		return ((t_fila_tabla_segmento_SPA*) fila_tabla)->nro_seg == nro_seg;
	}

	void _clean_bit_frame(void* _fila_tabla_pag){
		t_fila_tabla_paginas_SPA* fila_tabla_pag = (t_fila_tabla_paginas_SPA*) _fila_tabla_pag;
		bitarray_clean_bit(bitmap_frames, fila_tabla_pag->nro_frame);
	}

	void _free_segmento(void* _fila_tabla){
		t_fila_tabla_segmento_SPA* fila_tabla = (t_fila_tabla_segmento_SPA*) _fila_tabla;
		list_destroy_and_destroy_elements(fila_tabla->lista_tabla_paginas, free);
		free(fila_tabla);
	}


	pthread_mutex_lock(&sem_mutex_lista_procesos);
	t_proceso* proceso = list_find(lista_procesos, _mismo_pid);

	if(proceso == NULL){
		*ok = FM9_ERROR_NO_ENCONTRADO_EN_ESTR_ADM;
		pthread_mutex_unlock(&sem_mutex_lista_procesos);
		return;
	}

	t_fila_tabla_segmento_SPA* fila_tabla = list_find(proceso->lista_tabla_segmentos, _mismo_nro_seg);

	if(fila_tabla == NULL){
		*ok = FM9_ERROR_NO_ENCONTRADO_EN_ESTR_ADM;
		pthread_mutex_unlock(&sem_mutex_lista_procesos);
		return;
	}

	*ok = OK;

	pthread_mutex_lock(&sem_mutex_bitmap_frames);
	list_iterate(fila_tabla->lista_tabla_paginas, _clean_bit_frame);
	pthread_mutex_unlock(&sem_mutex_bitmap_frames);

	list_remove_and_destroy_by_condition(proceso->lista_tabla_segmentos, _mismo_nro_seg, _free_segmento);

	pthread_mutex_unlock(&sem_mutex_lista_procesos);
}

void _SPA_liberar_memoria_proceso(unsigned int pid){

	bool _mismo_pid(void* proceso){
		return ((t_proceso*) proceso)->pid == pid;
	}

	void _liberar_memoria_segmento(void* _fila){

		void __liberar_memoria_pagina(void* _fila_tabla_pag){
			t_fila_tabla_paginas_SPA* fila_tabla_pag = (t_fila_tabla_paginas_SPA*) _fila_tabla_pag;
			bitarray_clean_bit(bitmap_frames, fila_tabla_pag->nro_frame);
			free(_fila_tabla_pag);
		}

		t_fila_tabla_segmento_SPA* fila = (t_fila_tabla_segmento_SPA*) _fila;
		list_destroy_and_destroy_elements(fila->lista_tabla_paginas, __liberar_memoria_pagina);
		free(fila);
	}

	pthread_mutex_lock(&sem_mutex_lista_procesos);
	t_proceso* proceso = (t_proceso*) list_remove_by_condition(lista_procesos, _mismo_pid);
	if(proceso == NULL) {
		pthread_mutex_unlock(&sem_mutex_lista_procesos);
		return;
	}

	pthread_mutex_lock(&sem_mutex_bitmap_frames);
	list_destroy_and_destroy_elements(proceso->lista_tabla_segmentos, _liberar_memoria_segmento);
	pthread_mutex_unlock(&sem_mutex_bitmap_frames);

	free(proceso);

	pthread_mutex_unlock(&sem_mutex_lista_procesos);
}


void fm9_exit(){
	free(storage);
	close(listening_socket);
	log_destroy(logger);
	config_destroy(config);
}
