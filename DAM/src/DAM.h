#ifndef DAM_H_
#define DAM_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <stdarg.h>
	#include <sys/stat.h>
 	#include <sys/types.h>
	#include <stdbool.h>
	#include <pthread.h>

	#include <commons/config.h>
	#include <commons/log.h>
	#include <commons/string.h>
	#include <shared/socket.h>
	#include <shared/util.h>
	#include <shared/msg.h>


	/* Constantes */
	#define IP "127.0.0.1"

	#define CONFIG_PATH "../configs/DAM.txt"
	#define LOG_DIRECTORY_PATH "../logs/"
	#define LOG_PATH "../logs/DAM.log"

	/* Variables globales */
	t_config* config;
	t_log* logger;

	int safa_socket;
	int listenning_socket;


	int dam_initialize();

	int dam_send(int socket, e_tipo_msg tipo_msg, ...);

	bool dam_crear_nuevo_hilo(int socket_nuevo_cliente);
	void dam_nuevo_cliente_iniciar(int socket);
	int dam_manejar_nuevo_mensaje(int socket, t_msg* msg, int mdj_socket, int fm9_socket);

	int dam_transferencia_mdj_a_fm9(int mdj_socket, int* mdj_offset, int fm9_socket, int* fm9_offset,
			unsigned int id, char* path, int base, int* ok, char** linea_incompleta_buffer_anterior);

	void dam_exit();

#endif /* DAM_H_ */
