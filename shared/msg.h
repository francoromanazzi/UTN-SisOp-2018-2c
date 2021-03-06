#ifndef SHARED_MSG_H_
#define SHARED_MSG_H_
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>

	#include <commons/log.h>

	#include "_common_includes.h"
	#include "util.h"
	#include "DTB.h"
	
	typedef enum emisor{DESCONOCIDO, SAFA, CPU, DAM, FM9, MDJ} e_emisor;
	typedef enum {
		CONEXION, CONEXION_INTERRUPCIONES, DESCONEXION, HANDSHAKE,

		/* SAFA - SAFA */
		INOTIFY,

		/* SAFA - CPU */
		EXEC, READY, BLOCK, EXIT, INTERRUPCION,
		TIEMPO_RESPUESTA, SENTENCIA_EJECUTADA,
		WAIT, RESULTADO_WAIT,
		SIGNAL, RESULTADO_SIGNAL,

		/* SAFA - CPU - FM9 */
		LIBERAR_MEMORIA_FM9, RESULTADO_LIBERAR_MEMORIA_FM9,

		/* MDJ - DAM */
		VALIDAR, RESULTADO_VALIDAR,
		CREAR_MDJ, RESULTADO_CREAR_MDJ,
		GET_MDJ, RESULTADO_GET_MDJ,
		ESCRIBIR_MDJ, RESULTADO_ESCRIBIR_MDJ,
		BORRAR, RESULTADO_BORRAR,

		/* A DAM */
		FLUSH, RESULTADO_FLUSH,
		ABRIR, RESULTADO_ABRIR,

		/* CPU - FM9 y DAM - FM9 */
		GET_FM9, RESULTADO_GET_FM9,
		ESCRIBIR_FM9, RESULTADO_ESCRIBIR_FM9,

		/* FM9 - DAM */
		CREAR_FM9, RESULTADO_CREAR_FM9,
		RESERVAR_LINEA_FM9, RESULTADO_RESERVAR_LINEA_FM9,
		CLOSE, RESULTADO_CLOSE

	} e_tipo_msg;

	typedef struct {
		e_emisor emisor;
		e_tipo_msg tipo_mensaje;
		int payload_size;
	} __attribute__((packed)) t_header;

	typedef struct {
		t_header* header;
		void* payload;
	} t_msg;


	/**
	* @NAME: msg_create
	* @DESC: Crea un mensaje
	*/
	t_msg* msg_create(e_emisor emisor,e_tipo_msg tipo_msg, void** data, unsigned int data_len);

	/**
	* @NAME: msg_send
	* @DESC: Envia un mensaje a un socket
	*/
	int msg_send(int socket, t_msg mensaje);

	/**
	* @NAME: msg_await
	* @DESC: Espera hasta recibir un mensaje. El socket ya debe estar escuchando
	*/
	int msg_await(int socket, t_msg* mensaje);

	/**
	* @NAME: msg_free
	* @DESC: Libera la memoria de un mensaje
	*/
	void msg_free(t_msg** msg);

	/**
	* @NAME: msg_free_v2
	* @DESC: Libera la memoria de un mensaje, recibe un t_msg* casteado a void* en vez de un t_msg**
	*/
	void msg_free_v2(void* msg);

	/**
	* @NAME: msg_mostrar
	* @DESC: Muestra por pantalla toda la info de un mensaje
	*/
	void msg_mostrar(t_msg msg);

	/**
	* @NAME: msg_duplicar
	* @DESC: Duplica un mensaje
	*/
	t_msg* msg_duplicar(t_msg* msg);

#endif /* SHARED_MSG_H_ */
