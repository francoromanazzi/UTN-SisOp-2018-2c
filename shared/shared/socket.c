#include "socket.h"

int socket_create_listener(char* ip, char* port){
	if(port == NULL) return -1;

	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;    // No importa si uso IPv4 o IPv6
	hints.ai_flags = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
	hints.ai_socktype = SOCK_STREAM;  // Indica que usaremos el protocolo TCP

	getaddrinfo(ip, port, &hints, &server_info);  // Si IP es NULL, usa el localhost

	int server_socket = socket(server_info->ai_family, server_info->ai_socktype,server_info->ai_protocol);

	int activado = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));

	if(server_socket == -1 || bind(server_socket, server_info->ai_addr, server_info->ai_addrlen) == -1){
		freeaddrinfo(server_info);
		return -1;
	}

	freeaddrinfo(server_info);

	// if(listen(server_socket, 5) == -1) return -1;
	return server_socket;
}


int socket_connect_to_server(char* ip, char* port) {
	if(ip == NULL || port == NULL) return -1;

	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;    // No importa si uso IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM;  // Indica que usaremos el protocolo TCP

	getaddrinfo(ip, port, &hints, &server_info);

	int server_socket = socket(server_info->ai_family, server_info->ai_socktype,server_info->ai_protocol);

	int retorno = connect(server_socket, server_info->ai_addr, server_info->ai_addrlen);

	freeaddrinfo(server_info);

	return (retorno < 0 || server_socket == -1) ? -1 : server_socket;
}

