#include "socket.h"


static void close_conection(void *conexion);
static bool close_conection_condition(void *conexion);


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

	if(listen(server_socket, BACKLOG) == -1) return -1;
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

int socket_aceptar_conexion(int socketServidor) {
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	int socketCliente = accept(socketServidor, (struct sockaddr *) &addr, &addrlen);
	if (socketCliente < 0) {

		perror("Error al aceptar cliente");
		return -1;
	}

	return socketCliente;
}

char* get_ip_socket(int fd){
	struct sockaddr_in addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);
	int res = getpeername(fd, (struct sockaddr *)&addr, &addr_size);
	if(res == -1){
		return NULL;
	}
	char ipNodo[20];
	strcpy(ipNodo, inet_ntoa(addr.sin_addr));
	return strdup(ipNodo);
}

void socket_start_listening_select(int socketListener, int (*manejadorDeEvento)(int, t_msg*)){
	//Por si me mandan un socket con problemas
	if(socketListener == -1) return;

	t_list* conexiones = list_create();
	int activity, fdMax = socketListener;
	fd_set readfds;

	while(1){
		//Vacio el set e incluyo al socket de escucha
		FD_ZERO(&readfds);
		FD_SET(socketListener, &readfds);

		//Agrego a todas las conexiones
		for(int i = 0; i < list_size(conexiones); i++){
			FD_SET( ((t_conexion*) list_get(conexiones, i))->socket , &readfds);
		}

		//Esperamos que ocurra algo con alguna de las conexiones (inclusive con el socket de escucha)
		activity = select( fdMax + 1, &readfds, NULL, NULL, NULL);

		if (activity < 0) {
			//Ocurrio un error
			continue;
		}

		//Checkeamos si ocurrio algo con el socket de escucha
		if(FD_ISSET(socketListener, &readfds)){
			t_conexion* conexion = malloc(sizeof(t_conexion));
			conexion->addr = malloc(sizeof(struct sockaddr));

			//Calcula el espacio de la direccion de la nueva conexion
			socklen_t addrSize = sizeof(conexion->addr);

			//Acepto esta nueva conexion
			int nuevoSocket = accept(socketListener, conexion->addr, &addrSize);
			conexion->socket = nuevoSocket;

			//Pregunto si salio toodo bien
			if(nuevoSocket == -1) close_conection(conexion);

			//Añado la conexion a la lista
			list_add(conexiones, conexion);

			//Creamos un nuevo mensaje para avisar de la nueva conexion
			t_msg *msg = malloc(sizeof(t_msg));
			msg->header = malloc(sizeof(t_header));
			msg->header->emisor = DESCONOCIDO;
			msg->header->tipo_mensaje = CONEXION;
			msg->header->payload_size = 0;
			msg->payload = NULL;

			//Llamo a la funcion encargada de manejar las nuevas conexiones
			manejadorDeEvento(conexion->socket, msg);
			msg_free(&msg);

			//Seteo el nuevo fd maximo
			fdMax = nuevoSocket > fdMax ? nuevoSocket:fdMax;
		}

		//Recorremos preguntando por cada conexion si ocurrio algun evento
		for(int i = 0; i < list_size(conexiones); i++){

			if(FD_ISSET( ((t_conexion*) list_get(conexiones, i))->socket , &readfds )){ //Ocurrio algo con este socket
				//Recibo el mensaje
				t_msg* msg = malloc(sizeof(t_msg));
				msg->header = NULL;
				msg->payload = NULL;
				if(msg_await( ((t_conexion*) list_get(conexiones, i))->socket, msg) == -1){
					msg->header = malloc(sizeof(t_header));
					msg->header->emisor = ((t_conexion*) list_get(conexiones, i))->conectado;
					msg->header->tipo_mensaje = DESCONEXION;
					msg->header->payload_size = 0;
					msg->payload = NULL;
					manejadorDeEvento(((t_conexion*) list_get(conexiones, i))->socket, msg);
					socket_a_destruir = ((t_conexion*) list_get(conexiones, i))->socket;
					list_remove_and_destroy_by_condition(conexiones, close_conection_condition, close_conection);
				}else{
					((t_conexion*) list_get(conexiones, i))->conectado = msg->header->emisor;
					if( manejadorDeEvento(((t_conexion*) list_get(conexiones, i))->socket, msg) == -1){
						//Si es -1 significa que por alguna razon quiere que cierre la conexion
						socket_a_destruir = ((t_conexion*) list_get(conexiones, i))->socket;
						list_remove_and_destroy_by_condition(conexiones, close_conection_condition, close_conection);
					}
				}
				msg_free(&msg);
			}
		}

	}
}

static void close_conection(void *conexion){
	if(conexion != NULL){
		if(((t_conexion*)conexion)->socket != -1)
			close(((t_conexion*)conexion)->socket);
		if(((t_conexion*)conexion)->addr != NULL)
			free(((t_conexion*)conexion)->addr);
		free(conexion);
	}
	return;
}

static bool close_conection_condition(void *conexion){
	if(conexion != NULL){
		if ((((t_conexion*)conexion)->socket) == socket_a_destruir){
			return true;
		}
	}
	return false;
}

