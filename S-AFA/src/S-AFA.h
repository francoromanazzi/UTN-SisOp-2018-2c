#ifndef S_AFA_H_
#define S_AFA_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <sys/stat.h>
 	#include <sys/types.h>
	#include <commons/config.h>
	#include <commons/log.h>
	#include <string.h>
	#include <stdbool.h>
	#include <pthread.h>
	#include <shared/socket.h>
	#include "gestor_programas.h"
	#include "planificador.h"

	/* Constantes */
	#define IP "127.0.0.1"
	#define BACKLOG 10

	/* Variables globales */
	pthread_t thread_consola;
	t_config* config;
	t_log* logger;
	int listening_socket;
	int nuevo_cliente_socket;
	int dam_socket;


	void safa_exit(void);


#endif /* S_AFA_H_ */
