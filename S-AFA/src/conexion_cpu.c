#include "conexion_cpu.h"

void conexion_cpu_set_active(int socket){
	bool _mismo_socket(void* conexion_cpu_en_lista){
		return ((t_conexion_cpu*) conexion_cpu_en_lista)->socket == socket;
	}

	pthread_mutex_lock(&sem_mutex_cpu_conexiones);
	t_conexion_cpu* conexion = list_find(cpu_conexiones, _mismo_socket);
	conexion->en_uso = 0;
	pthread_mutex_unlock(&sem_mutex_cpu_conexiones);
	sem_post(&sem_cont_cpu_conexiones);
}

void conexion_cpu_set_inactive(int socket){
	bool _mismo_socket(void* conexion_cpu_en_lista){
		return ((t_conexion_cpu*) conexion_cpu_en_lista)->socket == socket;
	}

	sem_wait(&sem_cont_cpu_conexiones);
	pthread_mutex_lock(&sem_mutex_cpu_conexiones);
	t_conexion_cpu* conexion = list_find(cpu_conexiones, _mismo_socket);
	pthread_mutex_unlock(&sem_mutex_cpu_conexiones);

	conexion->en_uso = 1;
}

void conexion_cpu_add_new(int socket){
	t_conexion_cpu*  nueva_conexion_cpu = malloc(sizeof(t_conexion_cpu));
	nueva_conexion_cpu->socket = socket;
	nueva_conexion_cpu->en_uso = 0;

	pthread_mutex_lock(&sem_mutex_cpu_conexiones);
	list_add(cpu_conexiones, (void*) nueva_conexion_cpu);
	pthread_mutex_unlock(&sem_mutex_cpu_conexiones);
	sem_post(&sem_cont_cpu_conexiones);
}

void conexion_cpu_disconnect(int socket){

	bool _mismo_fd_socket(void* conexion_cpu_en_lista){
		return ((t_conexion_cpu*) conexion_cpu_en_lista)->socket == socket;
	}

	sem_wait(&sem_cont_cpu_conexiones);
	pthread_mutex_lock(&sem_mutex_cpu_conexiones);
	list_remove_and_destroy_by_condition(cpu_conexiones, _mismo_fd_socket, free);
	pthread_mutex_unlock(&sem_mutex_cpu_conexiones);
}
