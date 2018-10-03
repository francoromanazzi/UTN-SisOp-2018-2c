#include "conexion_cpu.h"

void conexion_cpu_set_active(int socket){
	bool _mismo_socket(void* conexion_cpu_en_lista){
		return ((t_conexion_cpu*) conexion_cpu_en_lista)->socket == socket;
	}

	pthread_mutex_lock(&sem_mutex_cpu_conexiones);
	t_conexion_cpu* conexion = list_find(cpu_conexiones, _mismo_socket);
	conexion->en_uso = 0;
	pthread_mutex_unlock(&sem_mutex_cpu_conexiones);
}

void conexion_cpu_set_inactive(int socket){
	bool _mismo_socket(void* conexion_cpu_en_lista){
		return ((t_conexion_cpu*) conexion_cpu_en_lista)->socket == socket;
	}

	pthread_mutex_lock(&sem_mutex_cpu_conexiones);
	t_conexion_cpu* conexion = list_find(cpu_conexiones, _mismo_socket);
	conexion->en_uso = 1;
	pthread_mutex_unlock(&sem_mutex_cpu_conexiones);
}

void conexion_cpu_add_new(int socket){
	t_conexion_cpu*  nueva_conexion_cpu = malloc(sizeof(t_conexion_cpu));
	nueva_conexion_cpu->socket = socket;
	nueva_conexion_cpu->en_uso = 0;

	pthread_mutex_lock(&sem_mutex_cpu_conexiones);
	list_add(cpu_conexiones, (void*) nueva_conexion_cpu);
	pthread_mutex_unlock(&sem_mutex_cpu_conexiones);
}

void conexion_cpu_disconnect(int socket){

	bool _mismo_fd_socket(void* conexion_cpu_en_lista){
		return ((t_conexion_cpu*) conexion_cpu_en_lista)->socket == socket;
	}

	pthread_mutex_lock(&sem_mutex_cpu_conexiones);
	list_remove_and_destroy_by_condition(cpu_conexiones, _mismo_fd_socket, free);
	pthread_mutex_unlock(&sem_mutex_cpu_conexiones);
}

int conexion_cpu_get_available(){
	int i;

	pthread_mutex_lock(&sem_mutex_cpu_conexiones);
	for(i = 0; i<cpu_conexiones->elements_count; i++){
		t_conexion_cpu* cpu = list_get(cpu_conexiones, i);
		if(cpu->en_uso == 0){
			cpu->en_uso = 1;
			pthread_mutex_unlock(&sem_mutex_cpu_conexiones);
			return cpu->socket;
		}
	}
	pthread_mutex_unlock(&sem_mutex_cpu_conexiones);
	return -1;
}










