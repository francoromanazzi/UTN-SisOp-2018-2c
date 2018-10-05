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
			mensaje_a_enviar = empaquetar_int(OK);
		break;

		case ESCRIBIR_FM9: // A FM9
			base = va_arg(arguments, int);
			offset = va_arg(arguments, int);
			datos = va_arg(arguments, char*);
			mensaje_a_enviar = empaquetar_escribir_fm9(base, offset, datos);
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
	int ok, i = 0, offset = 0;
	int resultadoManejar = 1; // Valor de retorno. -1 si quiero cerrar el hilo que atendia a esa CPU
	unsigned int id;
	char* path, *stream, **strings_a_enviar_a_fm9, **split_temp, *temp, *temp2, *linea_incompleta = NULL;
	int base = 0;
	bool str_anterior_terminaba_en_salto_linea = false;
	int offset_fm9 = 0; // Despues sacar
	int ok_escribir_fm9 = OK; // TODO: Usarlo despues de cada invocacion a _enviar_string_a_fm9

	void _enviar_string_a_fm9(char* str){
		log_info(logger, "Le envio %s a FM9. Base: %d, offset: %d", str, base, offset_fm9);
		dam_send(fm9_socket, ESCRIBIR_FM9, base, offset_fm9, str);
		offset_fm9++;

		msg_recibido = malloc(sizeof(t_msg));
		msg_await(fm9_socket, msg_recibido);
		ok_escribir_fm9 = desempaquetar_int(msg_recibido);
		msg_free(&msg_recibido);
	}

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
					break;
				}
				msg_free(&msg_recibido);

				/* Ok, el archivo existe. Comienzo la transferencia del archivo de MDJ a FM9*/
				//Primero le pido a FM9 que reserve el espacio suficiente para el archivo, y que me de la base
				dam_send(fm9_socket, CREAR_FM9);
				msg_recibido = malloc(sizeof(t_msg));
				msg_await(fm9_socket, msg_recibido);
				ok = desempaquetar_int(msg_recibido);
				if(ok == ERROR_ABRIR_ESPACIO_INSUFICIENTE_FM9){ // Fallo FM9, le aviso a SAFA
					dam_send(safa_socket, RESULTADO_ABRIR, ok, id, path, base);
					msg_free(&msg_recibido);
					free(path);
					break;
				}
				msg_free(&msg_recibido);
				base = ok;
				ok = OK;

				offset = 0;
				while(1){
					dam_send(mdj_socket, GET_MDJ, path, offset, config_get_int_value(config, "TRANSFER_SIZE"));
					offset += config_get_int_value(config, "TRANSFER_SIZE");
					msg_recibido = malloc(sizeof(t_msg));
					msg_await(mdj_socket, msg_recibido);
					stream = desempaquetar_string(msg_recibido);
					msg_free(&msg_recibido);

					log_info(logger, "Recibi de MDJ: %s.", stream);

					/* Me fijo si habia quedado incompleta una linea, del pedido anterior */
					if(linea_incompleta != NULL){
						if(!string_starts_with(stream, "\n")){ // Habia que agregarle cosas a la linea anterior
							split_temp = string_split(stream, "\n");
							temp = strdup(split_temp[0]);
							string_append(&linea_incompleta, temp);
							temp2 = string_substring_from(stream, strlen(temp)); // Saco del stream al faltante de la linea
							free(stream);
							stream = temp2;

							free(temp);
							split_liberar(split_temp);
						}
						if(string_contains(stream, "\n")){ // No hace falta otro pedido mas
							_enviar_string_a_fm9(linea_incompleta);
							if(ok_escribir_fm9 != OK){ // Fallo FM9, le aviso a SAFA
								dam_send(safa_socket, RESULTADO_ABRIR, ok_escribir_fm9, id, path, base);
								free(linea_incompleta);
								return resultadoManejar;
							}
							free(linea_incompleta);
							linea_incompleta = NULL;
						}
					}

					strings_a_enviar_a_fm9 = string_split(stream, "\n");

					if(split_cant_elem(strings_a_enviar_a_fm9) > 0 && !string_ends_with(stream, "\n")){ // No esta completa la ultima linea, me la guardo para el siguiente pedido
						linea_incompleta = strdup(strings_a_enviar_a_fm9[split_cant_elem(strings_a_enviar_a_fm9) - 1]);
						free(strings_a_enviar_a_fm9[split_cant_elem(strings_a_enviar_a_fm9) - 1]);
						strings_a_enviar_a_fm9[split_cant_elem(strings_a_enviar_a_fm9) - 1] = NULL;
					}

					string_iterate_lines(strings_a_enviar_a_fm9, _enviar_string_a_fm9);
					split_liberar(strings_a_enviar_a_fm9);

					if(ok_escribir_fm9 != OK){ // Fallo FM9, le aviso a SAFA
						dam_send(safa_socket, RESULTADO_ABRIR, ok_escribir_fm9, id, path, base);
						return resultadoManejar;
					}

					if(string_contains(stream, "\n\n") || (string_starts_with(stream, "\n") && str_anterior_terminaba_en_salto_linea)){
						free(stream);
						_enviar_string_a_fm9(" ");
						if(ok_escribir_fm9 != OK){ // Fallo FM9, le aviso a SAFA
							dam_send(safa_socket, RESULTADO_ABRIR, ok_escribir_fm9, id, path, base);
							return resultadoManejar;
						}
						break; // Termine de trasferir toodo el archivo
					}

					if(string_ends_with(stream, "\n")){
						str_anterior_terminaba_en_salto_linea = true;
					}
					else{
						str_anterior_terminaba_en_salto_linea = false;
					}

					free(stream);
				}

				/* Le mando a SAFA el resultado de abrir el archivo */
				dam_send(safa_socket, RESULTADO_ABRIR, ok, id, path, base);
				free(path);
			break;

			case FLUSH:
				log_info(logger, "Iniciando operacion FLUSH");
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
/*
int enviarArchivo(void** payload, int tamPayload){
	int resultadoTransferencia = 1;

	if(tamPayload != -1){

		t_msg* mensajeResultado = msg_create(DAM,RESULTADO_ABRIR,payload, tamPayload);

		if(msg_send(fm9_socket, mensajeResultado) == -1){
			log_info(logger, "Problemas al enviar linea de archivo a FM9");
			msg_free(mensajeResultado);
			resultadoTransferencia = -1;
		}

		if (msg_await(MDJ,mensajeResultado) == -1){
			log_info(logger, "Problema al recibir linea de archivo de MDJ");
			resultadoTransferencia = -1;
		}else{
			resultadoTransferencia = enviarArchivo(mensajeResultado->payload, mensajeResultado->header->payload_size);
		}

		msg_free(mensajeResultado);
	}
	return resultadoTransferencia;
}
*/

void dam_exit(){
	close(safa_socket);
	close(listenning_socket);
	log_destroy(logger);
	config_destroy(config);
}
