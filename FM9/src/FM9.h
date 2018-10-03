#ifndef FM9_H_
#define FM9_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <sys/stat.h>
 	#include <sys/types.h>
	#include <unistd.h>
	#include <sys/socket.h>
	#include <pthread.h>

	#include <commons/config.h>
	#include <commons/log.h>
	#include <shared/socket.h>
	#include <shared/util.h>

	#include "FM9_consola.h"


	/* Constantes */
	#define IP "127.0.0.1"

	#define CONFIG_PATH "../configs/FM9.txt"
	#define LOG_DIRECTORY_PATH "../logs/"
	#define LOG_PATH "../logs/FM9.log"

	/* Variables globales */
	t_config* config;
	t_log* logger;
	int listening_socket;
	pthread_t thread_consola;
	char* modo;
	int tamanio;
	int max_linea;
	int tam_pagina;
	int transfer_size;

	char* storage;

	int marca;

	int fm9_initialize();

	int fm9_send(int socket, e_tipo_msg tipo_msg, void* data);

	bool fm9_crear_nuevo_hilo(int socket_nuevo_cliente);
	void fm9_nuevo_cliente_iniciar(int socket);
	int fm9_manejar_nuevo_mensaje(int socket, t_msg* msg);
	int escribirEnMemoria(int tam,char* contenido);
	void fm9_exit();

#endif /* FM9_H_ */
