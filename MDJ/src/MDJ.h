#ifndef MDJ_H_
#define MDJ_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <commons/config.h>
	#include <commons/log.h>
	#include <shared/socket.h>
	#include <shared/util.h>

	/* Constantes */
	#define IP "127.0.0.1"

	/* Variables globales */
	t_config* config;
	t_log* logger;

	void config_create_fixed(char* path);
	void mdj_exit();

#endif /* MDJ_H_ */
