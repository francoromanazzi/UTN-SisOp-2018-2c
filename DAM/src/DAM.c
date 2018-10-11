#include "DAM.h"

int main(void) {
	if(dam_initialize() == -1){
		dam_exit();
		return EXIT_FAILURE;
	}

	/* Me quedo esperando conexiones de CPU */
	while(1){
		int nuevo_cliente = socket_aceptar_conexion(listenning_socket);
		if( !dam_crear_nuevo_hilo(nuevo_cliente)){
			log_error(logger, "No pude crear un nuevo hilo para atender una nueva conexion");
			dam_exit();
			return EXIT_FAILURE;
		}
		log_info(logger, "Creo un nuevo hilo para atender los pedidos de un nuevo cliente");
	}

	dam_exit();
	return EXIT_SUCCESS;
}

int dam_initialize(){

	void config_create_fixed(){
		config = config_create(CONFIG_PATH);
		util_config_fix_comillas(&config, "IP_SAFA");
		util_config_fix_comillas(&config, "IP_FM9");
		util_config_fix_comillas(&config, "IP_MDJ");
	}

	config_create_fixed();
	mkdir(LOG_DIRECTORY_PATH,0777);
	logger = log_create(LOG_PATH, "DAM", false, LOG_LEVEL_TRACE);

	/* Me conecto a SAFA (todos los hilos usan este socket) */
	safa_socket = socket_connect_to_server(config_get_string_value(config, "IP_SAFA"), config_get_string_value(config, "PUERTO_SAFA"));
	dam_send(safa_socket, CONEXION);

	log_info(logger, "Me conecte a SAFA");

	if((listenning_socket = socket_create_listener(IP, config_get_string_value(config, "PUERTO"))) == -1){
		log_error(logger, "No pude crear el socket de escucha");
		return -1;
	}
	log_info(logger, "Escucho en el socket %d", listenning_socket);
	return 1;
}

int dam_send(int socket, e_tipo_msg tipo_msg, ...){
	t_msg* mensaje_a_enviar;
	int ret;

	int ok, offset, size, base, cant_lineas;
	unsigned int id;
	char* path, *datos;

	va_list arguments;
	va_start(arguments, tipo_msg);

	switch(tipo_msg){
		case CONEXION: // A SAFA al inicio. Cada hilo se lo manda a MDJ y FM9
			mensaje_a_enviar = empaquetar_int(OK);
		break;

		case RESULTADO_ABRIR: // A SAFA
			ok = va_arg(arguments, int);
			id = va_arg(arguments, unsigned int);
			path = va_arg(arguments, char*);
			base = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_resultado_abrir(ok, id, path, base);
		break;

		case VALIDAR:// A MDJ
			path = va_arg(arguments, char*);
			mensaje_a_enviar = empaquetar_string(path);
		break;

		case CREAR_MDJ: // A MDJ
			path = va_arg(arguments, char*);
			cant_lineas = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_int(OK); //TODO: Empaquetar
		break;

		case GET_MDJ: // A MDJ
			path = va_arg(arguments, char*);
			offset = va_arg(arguments, int);
			size = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_get_mdj(path, offset, size);
		break;

		case ESCRIBIR_MDJ: // A MDJ
			path = va_arg(arguments, char*);
			offset = va_arg(arguments, int);
			size = va_arg(arguments, int);
			datos = va_arg(arguments, char*);
			mensaje_a_enviar = empaquetar_int(OK); //TODO: Empaquetar
		break;

		case BORRAR: // A MDJ
			path = va_arg(arguments, char*);
			mensaje_a_enviar = empaquetar_string(path);
		break;

		case CREAR_FM9: // A FM9
			id = va_arg(arguments, unsigned int);
			mensaje_a_enviar = empaquetar_int(id);
		break;

		case ESCRIBIR_FM9: // A FM9
			id = va_arg(arguments, unsigned int);
			base = va_arg(arguments, int);
			offset = va_arg(arguments, int);
			datos = va_arg(arguments, char*);
			mensaje_a_enviar = empaquetar_escribir_fm9(id, base, offset, datos);
		break;
	}
	mensaje_a_enviar->header->emisor = DAM;
	mensaje_a_enviar->header->tipo_mensaje = tipo_msg;
	ret = msg_send(socket, *mensaje_a_enviar);
	msg_free(&mensaje_a_enviar);

	va_end(arguments);
	return ret;
}

bool dam_crear_nuevo_hilo(int socket_nuevo_cliente){
	pthread_t nuevo_thread;
	if(pthread_create( &nuevo_thread, NULL, (void*) dam_nuevo_cliente_iniciar, (void*) socket_nuevo_cliente) ){
		return false;
	}
	pthread_detach(nuevo_thread);
	return true;
}

void dam_nuevo_cliente_iniciar(int socket){

	/* Me conecto a MDJ y FM9 */
	int mdj_socket  = socket_connect_to_server(config_get_string_value(config, "IP_MDJ"), config_get_string_value(config, "PUERTO_MDJ"));
	dam_send(mdj_socket, CONEXION, NULL);

	int fm9_socket  = socket_connect_to_server(config_get_string_value(config, "IP_FM9"), config_get_string_value(config, "PUERTO_FM9"));
	dam_send(fm9_socket, CONEXION, NULL);

	while(1){
		t_msg* nuevo_mensaje = malloc(sizeof(t_msg));
		if(msg_await(socket, nuevo_mensaje) == -1 || dam_manejar_nuevo_mensaje(socket, nuevo_mensaje, mdj_socket, fm9_socket) == -1){
			log_info(logger, "Cierro el hilo que atendia a este cliente");
			free(nuevo_mensaje);
			close(mdj_socket);
			close(fm9_socket);
			close(socket);
			pthread_exit(NULL);
			return;
		}
		msg_free(&nuevo_mensaje);
	} // Fin while(1)
}

int dam_manejar_nuevo_mensaje(int socket, t_msg* msg, int mdj_socket, int fm9_socket){
	log_info(logger, "EVENTO: Emisor: %d, Tipo: %d, Tamanio: %d",msg->header->emisor,msg->header->tipo_mensaje,msg->header->payload_size);

	t_msg* msg_recibido;
	int ok, base, cant_lineas;
	int resultadoManejar = 1; // Valor de retorno. -1 si quiero cerrar el hilo que atendia a esa CPU
	unsigned int id;
	char* path;

	/* Para dam_transferencia_mdj_a_fm9, porque las variables estaticas se comparten entre hilos :( */
	char* linea_incompleta_buffer_anterior = NULL;
	int offset_mdj = 0, offset_fm9 = 0;

	if(msg->header->emisor == CPU){
		switch(msg->header->tipo_mensaje){
			case CONEXION:
				log_info(logger, "Se conecto CPU");
			break;

			case DESCONEXION:
				log_info(logger, "Se desconecto CPU");
				resultadoManejar = -1;
			break;

			case ABRIR:
				desempaquetar_abrir(msg,&path,&id);
				log_info(logger, "Ehhh, voy a busar %s para %d", path, id);

				/* Le pregunto a MDJ si el archivo existe */
				dam_send(mdj_socket, VALIDAR, path);
				msg_recibido = malloc(sizeof(t_msg));
				msg_await(mdj_socket, msg_recibido);
				ok = desempaquetar_int(msg_recibido);
				if(ok != OK){ // Fallo MDJ, le aviso a SAFA
					dam_send(safa_socket, RESULTADO_ABRIR, ok, id, path, base);
					msg_free(&msg_recibido);
					free(path);
					return resultadoManejar;
				}
				msg_free(&msg_recibido);

				/* Ok, el archivo existe. Comienzo la transferencia del archivo de MDJ a FM9*/
				//Primero le pido a FM9 que reserve el espacio suficiente para el archivo, y que me de la base
				dam_send(fm9_socket, CREAR_FM9, id);
				msg_recibido = malloc(sizeof(t_msg));
				msg_await(fm9_socket, msg_recibido);
				ok = desempaquetar_int(msg_recibido);
				if(ok == ERROR_ABRIR_ESPACIO_INSUFICIENTE_FM9){ // Fallo FM9, le aviso a SAFA
					dam_send(safa_socket, RESULTADO_ABRIR, ok, id, path, base);
					msg_free(&msg_recibido);
					free(path);
					return resultadoManejar;
				}
				msg_free(&msg_recibido);
				base = ok;
				ok = OK;

				/* Transfiero de MDJ a FM9 */
				while(dam_transferencia_mdj_a_fm9(
						mdj_socket, &offset_mdj,
						fm9_socket, &offset_fm9,
						id, path, base, &ok,
						&linea_incompleta_buffer_anterior)); // Mira todos esos parametros papa

				/* Le mando a SAFA el resultado de abrir el archivo */
				dam_send(safa_socket, RESULTADO_ABRIR, ok, id, path, base);
				free(path);
			break;

			case FLUSH:
				log_info(logger, "Iniciando operacion FLUSH");
				desempaquetar_flush(msg, &id, &path, &base);
				// TODO
			break;

			case CREAR_MDJ:
				log_info(logger, "Iniciando operacion CREAR");
				desempaquetar_crear(msg, &path, &cant_lineas);

				dam_send(mdj_socket, CREAR_MDJ, path, cant_lineas);
				msg_recibido = malloc(sizeof(t_msg));
				msg_await(mdj_socket, msg_recibido);
				ok = desempaquetar_int(msg_recibido);

				if(ok != OK){
					dam_send(safa_socket, RESULTADO_CREAR_MDJ, ok);
					msg_free(&msg_recibido);
					free(path);
					return resultadoManejar;
				}
			break;

			case BORRAR:
				log_info(logger, "Iniciando operacion BORRAR");
				path = desempaquetar_string(msg);
				// TODO
			break;

			default:
				log_info(logger, "No entendi el mensaje de CPU");
		}
	}
	else if(msg->header->emisor == DESCONOCIDO){
		log_info(logger, "Me hablo alguien desconocido");
	}

	return resultadoManejar;
}

// Return: 0-> Transferencia completada, 1-> Transferencia no completada
int dam_transferencia_mdj_a_fm9(int mdj_socket, int* mdj_offset, int fm9_socket, int* fm9_offset,
		unsigned int id, char* path, int base, int* ok, char** linea_incompleta_buffer_anterior){

	void* buffer;
	char* buffer_str;
	int buffer_size, ok_escribir_fm9 = OK;
	t_msg* msg_recibido;

	void _enviar_linea_a_fm9_y_liberarla(char* str){
		log_info(logger, "Le envio %s a FM9. Base: %d, offset: %d", str, base, *fm9_offset);
		dam_send(fm9_socket, ESCRIBIR_FM9, id , base, *fm9_offset, str);
		free(str);
		(*fm9_offset)++;

		msg_recibido = malloc(sizeof(t_msg));
		msg_await(fm9_socket, msg_recibido);
		ok_escribir_fm9 = desempaquetar_int(msg_recibido);
		msg_free(&msg_recibido);
	}

	char** _buffer_to_lineas(){ // Utiliza "buffer", y utiliza y modifica "linea_incompleta_buffer_anterior"
		char** ret = NULL;
		char* linea_actual = strdup("");
		int nro_linea_actual = 0, i, c;

		if(*linea_incompleta_buffer_anterior != NULL){
			string_append(&linea_actual, *linea_incompleta_buffer_anterior);
			free(*linea_incompleta_buffer_anterior);
			*linea_incompleta_buffer_anterior = NULL;
		}

		for(i = 0; i < buffer_size; i++){ // Recorro el buffer
			if(buffer_str[i] == '\n'){ // Fin de linea_actual
				ret = realloc(ret, sizeof(char*) * (nro_linea_actual + 1));
				*(ret + nro_linea_actual) = strdup(linea_actual);
				free(linea_actual);
				linea_actual = strdup("");
				nro_linea_actual++;
			}
			else{ // La linea_actual no termino
				char c = buffer_str[i];
				char *c_str = malloc(sizeof(char) * 2);
				c_str[0] = c;
				c_str[1] = '\0';
				string_append(&linea_actual, c_str);
				free(c_str);
			}
		}

		if(strcmp(linea_actual, "")){ // Habia linea incompleta al final del buffer
			*linea_incompleta_buffer_anterior = linea_actual;
		}
		else{
			free(linea_actual);
			*linea_incompleta_buffer_anterior = NULL;
		}

		/* Agrego un NULL al final, para el string_iterate_lines */
		if(ret != NULL){
			ret = realloc(ret, sizeof(char*) * (nro_linea_actual + 1));
			*(ret + nro_linea_actual) = NULL;
		}
		return ret;
	}


	/* Le pido a MDJ obtener datos */
	dam_send(mdj_socket, GET_MDJ, path, *mdj_offset, config_get_int_value(config, "TRANSFER_SIZE"));
	(*mdj_offset) += config_get_int_value(config, "TRANSFER_SIZE");
	msg_recibido = malloc(sizeof(t_msg));
	msg_await(mdj_socket, msg_recibido);
	desempaquetar_void_ptr(msg_recibido, &buffer, &buffer_size); // Si buffer_size < transfer_size => Fin de archivo (return 0)
	msg_free(&msg_recibido);

	buffer_str = malloc(buffer_size + 1);
	memcpy((void*) buffer_str, buffer, buffer_size);
	free(buffer);
	buffer_str[buffer_size] = '\0';
	log_info(logger, "Recibi de MDJ: %s.", buffer_str);


	/* Divido al buffer en lineas */
	char** lineas = _buffer_to_lineas();
	free(buffer_str);

	/* Le mando las lineas (si hay) a FM9 */
	if(lineas != NULL){
		string_iterate_lines(lineas, _enviar_linea_a_fm9_y_liberarla);
		*ok = ok_escribir_fm9;
		free(lineas);
	}

	if(buffer_size < config_get_int_value(config, "TRANSFER_SIZE")){
		if(*linea_incompleta_buffer_anterior != NULL){
			_enviar_linea_a_fm9_y_liberarla(*linea_incompleta_buffer_anterior);
			*ok = ok_escribir_fm9;
			*linea_incompleta_buffer_anterior = NULL;
		}
		*mdj_offset = 0;
		*fm9_offset = 0;
		return 0;
	}
	return 1;
}

void dam_exit(){
	close(safa_socket);
	close(listenning_socket);
	log_destroy(logger);
	config_destroy(config);
}
