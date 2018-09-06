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
	#include <stdbool.h>

	/* Constantes */
	#define IP "127.0.0.1"
	#define BACKLOG 20

	/* Variables globales */
	t_config* config;
	t_log* logger;
	int safa_socket;
	int fm9_socket;
	int mdj_socket;
	int listenning_socket;

	void config_create_fixed(char* path);
	void dam_exit();

#endif /* DAM_H_ */
