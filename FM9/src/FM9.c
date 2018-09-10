#include "FM9.h"

int main(void) {
	config = config_create("../../configs/FM9.txt");
	mkdir("../../logs",0777);
	logger = log_create("../../logs/FM9.log", "FM9", false, LOG_LEVEL_TRACE);

	if((listening_socket = socket_create_listener(IP, config_get_string_value(config, "PUERTO"))) == -1){
		log_error(logger,"No se pudo crear socket de escucha");
		fm9_exit();
		exit(EXIT_FAILURE);
	}
	log_info(logger, "Comienzo a escuchar  por el socket %d", listening_socket);

	socket_start_listening_select(listening_socket, fm9_manejador_de_eventos);

	fm9_exit();
	return EXIT_SUCCESS;
}

int fm9_manejador_de_eventos(int socket, t_msg* msg){
	log_info(logger, "EVENTO: Emisor: %d, Tipo: %d, Tamanio: %d, Mensaje: %s",msg->header->emisor,msg->header->tipo_mensaje,msg->header->payload_size,(char*) msg->payload);

	if(msg->header->emisor == DAM){
		switch(msg->header->tipo_mensaje){
			case CONEXION:
				log_info(logger,"Se me conecto DAM");
			break;

			case ESCRIBIR:
				log_info(logger,"DAM me pidio la operacion ESCRIBIR");
			break;

			case GET:
				log_info(logger,"DAM me pidio la operacion GET");
			break;

			default:
				log_info(logger,"No entendi el mensaje de DAM");
		}

	}
	else if(msg->header->emisor == CPU){
		switch(msg->header->tipo_mensaje){
			case GET:
				log_info(logger,"CPU me pidio la operacion GET");
			break;

			default:
				log_info(logger,"No entendi el mensaje de DAM");
		}
	}
	else if(msg->header->emisor == DESCONOCIDO){
		log_info(logger, "Me hablo alguien desconocido");
	}
	return 1;
}


void fm9_exit(){
	close(listening_socket);
	log_destroy(logger);
	config_destroy(config);
}
