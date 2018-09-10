#ifndef FM9_H_
#define FM9_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <sys/stat.h>
 	#include <sys/types.h>
	#include <commons/config.h>
	#include <commons/log.h>
	#include <shared/socket.h>
	#include <shared/util.h>
	#include <unistd.h>
	#include <sys/socket.h>


	/* Constantes */
	#define IP "127.0.0.1"


	/* Variables globales */
	t_config* config;
	t_log* logger;
	int listening_socket;


	int fm9_manejador_de_eventos(int socket, t_msg* msg);
	void fm9_exit();

#endif /* FM9_H_ */
