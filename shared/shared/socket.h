#ifndef SHARED_SOCKET_H_
#define SHARED_SOCKET_H_

	#include <stdio.h>
	#include <string.h>
	#include <stdlib.h>
	#include <sys/socket.h>
	#include <commons/collections/list.h>
	#include <netdb.h>
	#include <unistd.h>
	#include "msg.h"

	#define BACKLOG 100

	typedef struct {
		int socket;
		e_emisor conectado;
		__SOCKADDR_ARG addr;
	} t_conexion;

	int socket_a_destruir;

	/**
	* @NAME: socket_create_listener
	* @DESC: Creo un socket de escucha y lo devuelvo, o -1 se hubo error. Me pongo a escuchar.
	* @PARAMS:
	* 		ip   - ip del server. Si es NULL, usa el localhost: 127.0.0.1
	* 		port - puerto en el que escuchar
	*/
	int socket_create_listener(char* ip, char* port);

	/**
	* @NAME: socket_connect_to_server
	* @DESC: Me conecto al server, y devuelvo el socket, o -1 si hubo error
	*/
	int socket_connect_to_server(char* ip, char* port);

	/**
	* @NAME: socket_aceptar_conexion
	* @DESC: Acepta una nueva conexión y devuelve el nuevo socket conectado
	*/
	int socket_aceptar_conexion(int socket_servidor);

	/**
	* @NAME: socket_get_ip
	* @DESC: Devuelve la IP de un socket
	*/
	char* socket_get_ip(int fd);

	/**
	* @NAME: socket_start_listening_select
	* @DESC: Gestiona eventos en un socket con la funcion que es parametro
	*/
	void socket_start_listening_select(int socketListener, int (*manejadorDeEvento)(int, t_msg*));


#endif /* SHARED_SOCKET_H_ */
