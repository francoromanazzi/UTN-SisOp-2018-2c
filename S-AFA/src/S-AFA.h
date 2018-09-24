#ifndef S_AFA_H_
#define S_AFA_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <stdarg.h>
	#include <string.h>
	#include <stdbool.h>
	#include <pthread.h>
	#include <semaphore.h>
	#include <sys/stat.h>
 	#include <sys/types.h>
	#include <sys/inotify.h>

	#include <commons/config.h>
	#include <commons/log.h>
	#include <commons/collections/list.h>
	#include <shared/socket.h>
	#include "planificador.h"
	#include "conexion_cpu.h"
	#include "gestor.h"

	/* Constantes */
	#define IP "127.0.0.1"
	#define CONFIG_PATH "/home/utnso/workspace/tp-2018-2c-RegorDTs/configs/S-AFA.txt"

	/* Variables globales */
	long int retardo_planificacion;
	pthread_mutex_t sem_mutex_config_retardo;
	int quantum;
	pthread_mutex_t sem_mutex_config_quantum;
	int multiprogramacion;
	pthread_mutex_t sem_mutex_config_multiprogramacion;
	char* algoritmo;
	pthread_mutex_t sem_mutex_config_algoritmo;

	pthread_t thread_inotify_config;
	pthread_t thread_gestor;
	pthread_t thread_planificador;

	t_config* config;
	t_log* logger;

	bool estado_operatorio;
	bool cpu_conectado;
	bool dam_conectado;

	int listening_socket;
	int dam_socket;

	sem_t sem_bin_plp_cargar_archivo;
	sem_t sem_bin_pcp_cargar_archivo;
	t_msg* msg_resultado_abrir;

	t_list* cpu_conexiones;
	sem_t sem_cont_cpu_conexiones;
	pthread_mutex_t sem_mutex_cpu_conexiones;

	t_list* cola_mensajes;
	sem_t sem_cont_cola_mensajes;
	pthread_mutex_t sem_mutex_cola_mensajes;

	t_list* lista_procesos_a_finalizar;
	pthread_mutex_t sem_mutex_lista_procesos_a_finalizar;


	int safa_initialize();
	void safa_inotify_config_iniciar();
	void safa_iniciar_estado_operatorio();

	int safa_send(int socket, e_tipo_msg tipo_msg, ...);

	int safa_manejador_de_eventos(int socket, t_msg* msg); 	// Devuelvo -1 si quiero cerrar esa conexion
	void safa_encolar_mensaje(t_msg* msg);

	void safa_exit();


#endif /* S_AFA_H_ */
