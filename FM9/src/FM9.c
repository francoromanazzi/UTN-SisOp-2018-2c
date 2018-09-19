#include "FM9.h"

int main(void) {

	fm9_initialize();

	if((listening_socket = socket_create_listener(IP, config_get_string_value(config, "PUERTO"))) == -1){
		log_error(logger,"No se pudo crear socket de escucha");
		fm9_exit();
		exit(EXIT_FAILURE);
	}
	log_info(logger, "Comienzo a escuchar  por el socket %d", listening_socket);

	while(1){
		int nuevo_cliente = socket_aceptar_conexion(listening_socket);
		if( !fm9_crear_nuevo_hilo(nuevo_cliente)){
			log_error(logger, "No pude crear un nuevo hilo para atender una nueva conexion");
			fm9_exit();
		}
		log_info(logger, "Creo un nuevo hilo para atender los pedidos de un nuevo cliente");
	}

	//socket_start_listening_select(listening_socket, fm9_manejador_de_eventos);

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

int fm9_send(int socket, e_tipo_msg tipo_msg, void* data){
	t_msg* mensaje_a_enviar;
	int ret;

	switch(tipo_msg){
		case RESULTADO_GET:
			mensaje_a_enviar = empaquetar_resultado_get(data, strlen((char*) data));
		break;
	}
	mensaje_a_enviar->header->emisor = FM9;
	mensaje_a_enviar->header->tipo_mensaje = tipo_msg;
	ret = msg_send(socket, *mensaje_a_enviar);
	msg_free(&mensaje_a_enviar);
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
		msg_await(socket, nuevo_mensaje);
		if(fm9_manejar_nuevo_mensaje(socket, nuevo_mensaje) == -1){
			log_info(logger, "Cierro el hilo que atendia a este cliente");
			// TODO: Cerrar el hilo?
			return;
		}
	}
}

int fm9_manejar_nuevo_mensaje(int socket, t_msg* msg){
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
			case CONEXION:
				log_info(logger,"Se me conecto CPU");
			break;

			case DESCONEXION:
				log_info(logger,"Se me desconecto CPU");
			break;

			case GET:
				log_info(logger,"CPU me pidio la operacion GET");

				int base, offset;
				desempaquetar_get(msg, &base, &offset);

				// Aca deberia buscar en su memoria

				fm9_send(socket, RESULTADO_GET, (void*) "concentrar"); // Hardcodeado
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


void fm9_exit(){
	close(listening_socket);
	log_destroy(logger);
	config_destroy(config);
}
