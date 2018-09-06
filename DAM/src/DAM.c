#include "DAM.h"

int main(void) {
	config_create_fixed("../../configs/DAM.txt");
	mkdir("../../logs",0777);
	logger = log_create("../../logs/DAM.log", "DAM", false, LOG_LEVEL_TRACE);

	/* Me conecto a S-AFA, FM9 y MDJ */
	safa_socket = socket_connect_to_server(config_get_string_value(config, "IP_SAFA"), config_get_string_value(config, "PUERTO_SAFA"));
	//fm9_socket  = socket_connect_to_server(config_get_string_value(config, "IP_FM9"), config_get_string_value(config, "PUERTO_FM9"));;
	//mdj_socket  = socket_connect_to_server(config_get_string_value(config, "IP_MDJ"), config_get_string_value(config, "PUERTO_MDJ"));;

	/* Verifico que pude conectarme con todos */
	if(safa_socket <= 0)
		log_error(logger, "No pude conectarme a SAFA");
	else
		log_info(logger, "Me conecte a SAFA");

	/* Me quedo esperando conexiones de CPU y/o pedidos de S-AFA */
	listenning_socket = socket_create_listener(IP, config_get_string_value(config, "PUERTO"));
	listen(listenning_socket, BACKLOG); // ???????

	dam_exit();
	return EXIT_SUCCESS;
}

void config_create_fixed(char* path){
	config = config_create(path);
	util_config_fix_comillas(&config, "IP_SAFA");
	util_config_fix_comillas(&config, "IP_FM9");
	util_config_fix_comillas(&config, "IP_MDJ");
}


void dam_exit(){
	close(safa_socket);
	// close(fm9_socket);
	// close(mdj_socket);
	close(listenning_socket);
	log_destroy(logger);
	config_destroy(config);
}
