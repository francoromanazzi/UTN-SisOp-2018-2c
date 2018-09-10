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

	for(;;) sleep(20);
	mdj_exit();
	return EXIT_SUCCESS;
}

void config_create_fixed(char* path){
	config = config_create(path);
	util_config_fix_comillas(&config, "PUNTO_MONTAJE");
}

void mdj_exit(){
	log_destroy(logger);
	config_destroy(config);
}
