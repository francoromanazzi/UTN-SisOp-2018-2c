#ifndef MDJ_H_
#define MDJ_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <commons/config.h>
	#include <commons/log.h>
	#include <shared/socket.h>
	#include <shared/util.h>

	/* Constantes */
	#define IP "127.0.0.1"

	/* Variables globales */
	t_config* config;
	t_log* logger;
	int listenning_socket;
	int dam_socket;

	void mdj_esperar_ordenes_dam();
	void config_create_fixed(char* path);
	void crearEstructuras();
	void mdj_exit();

#endif /* MDJ_H_ */
