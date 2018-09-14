#include "FM9.h"

int main(void) {

	fm9_initialize();

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

void fm9_initialize(){
	config = config_create("/home/utnso/workspace/tp-2018-2c-RegorDTs/configs/FM9.txt");
	mkdir("/home/utnso/workspace/tp-2018-2c-RegorDTs/logs",0777);
	logger = log_create("/home/utnso/workspace/tp-2018-2c-RegorDTs/logs/FM9.log", "FM9", false, LOG_LEVEL_TRACE);

	modo=config_get_string_value(config, "MODO");
	tamanio = config_get_int_value(config, "TAMANIO");
	max_linea = config_get_int_value(config, "MAX_LINEA");
	tam_pagina = config_get_int_value(config, "TAM_PAGINA");
	log_info(logger,"Se realiza la inicializacion del Storage");
	//storage=malloc(tamanio);//???

	if(pthread_create(&thread_consola,NULL,(void*) fm9_consola_init,NULL)){
		log_error(logger,"No se pudo crear el hilo para la consola");
				fm9_exit();
				exit(EXIT_FAILURE);
	}
	log_info(logger,"Se creo el hilo para la consola");

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
