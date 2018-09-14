#include "msg.h"

t_msg* msg_create(e_emisor emisor,e_tipo_msg tipo_msg, void** data, unsigned int data_len){
	t_msg* ret = malloc(sizeof(t_msg));
	ret->payload = malloc(data_len);
	ret->header = malloc(sizeof(t_header));
	ret->header->tipo_mensaje = tipo_msg;
	ret->header->emisor = emisor;
	ret->header->payload_size = data_len;
	memcpy(ret->payload, &data, data_len);
	return ret;
}

int msg_send(int socket, t_msg msg){
	//Compruebo que el socket exista, y que el mensaje no sea nulo
	if(socket == -1 ||  msg.payload == NULL || msg.header == NULL) return -1;

	//Calculo el tamaño total a enviar
	int total_size = sizeof(t_header) + msg.header->payload_size;

	// Creo el buffer, y empaqueto los datos
	void* buffer = malloc(total_size);
	memcpy(buffer, msg.header, sizeof(t_header));
	memcpy(buffer + sizeof(t_header), msg.payload, msg.header->payload_size);

	//Envio, y devuelvo el resultado del envio, que es la cant de bytes enviados o -1 si hubo error
	int resultado = send(socket, buffer, total_size, 0);

	free(buffer);
	return resultado;
}

int msg_await(int socket, t_msg* msg){
	if(socket == -1) return -1;

	t_header* header = malloc(sizeof(t_header));
	int result_recv = recv(socket, header, sizeof(t_header), MSG_WAITALL);
	if(result_recv == -1){
		free(header);
		return -1;
	}

	void* payload = malloc(header->payload_size + 1);
	result_recv = recv(socket, payload, header->payload_size, MSG_WAITALL);
	if(result_recv < 1){
		free(header);
		free(payload);
		return -1;
	}

	msg->header = header;
	msg->payload = payload;
	return result_recv;
}

void msg_free(t_msg** msg){
	if(msg != NULL && (*msg) != NULL){
		if((*msg)->header != NULL){
			if((*msg)->payload != NULL && (*msg)->header->payload_size > 0)
				free((*msg)->payload);
			free((*msg)->header);
		}
		free(*msg);
	}
}

void msg_mostrar(t_msg msg){
	printf("Emisor: %d\n",msg.header->emisor);
	printf("Tipo mensaje: %d\n",msg.header->tipo_mensaje);
	printf("Tamanio mensaje: %d\n",msg.header->payload_size);
	printf("Mensaje: %s\n",(char*) msg.payload);
}













