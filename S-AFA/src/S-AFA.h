#ifndef S_AFA_H_
#define S_AFA_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <sys/stat.h>
 	#include <sys/types.h>
	#include <commons/config.h>
	#include <commons/log.h>
	#include <commons/collections/list.h>
	#include <string.h>
	#include <stdbool.h>
	#include <pthread.h>
	#include <semaphore.h>
	#include <shared/socket.h>
	#include "planificador.h"
	#include "conexion_cpu.h"
	#include "gestor.h"

	/* Constantes */
	#define IP "127.0.0.1"

	/* Variables globales */
	int retardo_planificacion;

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



	void safa_initialize();
	void safa_encolar_mensaje(t_msg* msg);

	int safa_manejador_de_eventos(int socket, t_msg* msg); 	// Devuelvo -1 si quiero cerrar esa conexion
	void safa_iniciar_estado_operatorio();
	void safa_exit();


#endif /* S_AFA_H_ */
