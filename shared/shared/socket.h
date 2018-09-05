#ifndef SHARED_SOCKET_H_
#define SHARED_SOCKET_H_

	#include <stdio.h>
	#include <string.h>
	#include <stdlib.h>
	#include <sys/socket.h>
	#include <netdb.h>
	#include <unistd.h>

	/**
	* @NAME: socket_create_listener
	* @DESC: Creo un socket de escucha y lo devuelvo, o -1 se hubo error. No me pongo a escuchar.
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


#endif /* SHARED_SOCKET_H_ */
