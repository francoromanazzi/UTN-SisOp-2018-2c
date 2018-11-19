#ifndef CONEXION_CPU_H_
#define CONEXION_CPU_H_
	#include "S-AFA.h"

	typedef struct{
		int socket;
		int socket_interrupciones;
		int en_uso;
	} t_conexion_cpu;


	void conexion_cpu_set_active(int socket);

	void conexion_cpu_set_inactive(int socket);

	void conexion_cpu_add_new(int socket, int socket_interrupciones);

	void conexion_cpu_disconnect(int socket);

	int conexion_cpu_get_available();

	int conexion_cpu_get_socket_interrupciones(int socket);

#endif /* CONEXION_CPU_H_ */
