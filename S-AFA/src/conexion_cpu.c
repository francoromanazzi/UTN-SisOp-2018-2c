#include "conexion_cpu.h"

void conexion_cpu_set_active(int socket){
	bool _mismo_socket(void* conexion_cpu_en_lista){
		return ((t_conexion_cpu*) conexion_cpu_en_lista)->socket == socket;
	}

	t_conexion_cpu* conexion = list_find(cpu_conexiones, _mismo_socket);
	conexion->en_uso = 0;
}
