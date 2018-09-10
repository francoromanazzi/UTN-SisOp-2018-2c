#ifndef DAM_H_
#define DAM_H_

	#include <stdio.h>
	#include <stdlib.h>
	#include <sys/stat.h>
 	#include <sys/types.h>
	#include <commons/config.h>
	#include <commons/log.h>
	#include <shared/socket.h>
	#include <shared/util.h>
	#include <shared/msg.h>
	#include <stdbool.h>

	/* Constantes */
	#define IP "127.0.0.1"

	/* Variables globales */
	t_config* config;
	t_log* logger;
	int safa_socket;
	int fm9_socket;
	int mdj_socket;
	int listenning_socket;


	int dam_connect_to_safa();
	int dam_connect_to_mdj();
	int dam_connect_to_fm9();
	int dam_manejador_de_eventos(int socket, t_msg* msg);
	void config_create_fixed(char* path);
	void dam_exit();



#endif /* DAM_H_ */
