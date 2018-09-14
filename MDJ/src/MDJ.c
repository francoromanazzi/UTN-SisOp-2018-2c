#include "MDJ.h"

int main(void) {
	config_create_fixed("../../configs/MDJ.txt");
	mkdir("../../logs",0777);
	logger = log_create("../../logs/MDJ.log", "MDJ", false, LOG_LEVEL_TRACE);

	if((listenning_socket = socket_create_listener(IP, config_get_string_value(config, "PUERTO"))) == -1){
		log_error(logger, "No pude crear el socket de escucha");
		mdj_exit();
		exit(EXIT_FAILURE);
	}
	log_info(logger,"Escucho en el socket %d", listenning_socket);
	dam_socket = socket_aceptar_conexion(listenning_socket);
	log_info(logger,"Se me conecto DAM, en el socket %d", dam_socket);

	while(1)
		mdj_esperar_ordenes_dam();

	mdj_exit();
	return EXIT_SUCCESS;
}

void mdj_esperar_ordenes_dam(){
	t_msg* msg = malloc(sizeof(t_msg));
	msg_await(dam_socket, msg);
	if(msg->header->tipo_mensaje == VALIDAR){
		log_info(logger, "Recibi ordenes de DAM de validar un archivo");
		char* path_a_validar = desempaquetar_string(msg);
		int archivo_existe = 1;

		// TODO: Validar que el archivo exista

		/* Le comunico a Diego el resultado (1->OK, 0->NO_OK) */
		t_msg* mensaje_a_enviar = msg_create(MDJ, RESULTADO_VALIDAR, (void**) archivo_existe, sizeof(int));
		int resultado_send = msg_send(dam_socket, *mensaje_a_enviar);
		msg_free(&mensaje_a_enviar);

		free(path_a_validar);
		msg_free(&msg);
	}
	else if(msg->header->tipo_mensaje == CREAR){
		log_info(logger, "Recibi ordenes de DAM de crear un archivo");
		msg_free(&msg);
	}
	else if(msg->header->tipo_mensaje == GET){
		log_info(logger, "Recibi ordenes de DAM de obtener datos");
		msg_free(&msg);
	}
	else if(msg->header->tipo_mensaje == ESCRIBIR){
		log_info(logger, "Recibi ordenes de DAM de guardar datos");
		msg_free(&msg);
	}
	else if(msg->header->tipo_mensaje == DESCONEXION){
		log_info(logger, "Se desconecto DAM");
		msg_free(&msg);
	}
	else{
		log_error(logger, "No entendi el mensaje de DAM");
		msg_free(&msg);
	}
}

void config_create_fixed(char* path){
	config = config_create(path);
	util_config_fix_comillas(&config, "PUNTO_MONTAJE");
}

void mdj_exit(){
	log_destroy(logger);
	config_destroy(config);
}
