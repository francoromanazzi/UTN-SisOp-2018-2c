#ifndef S_AFA_H_
#define S_AFA_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <stdarg.h>
	#include <time.h>
	#include <string.h>
	#include <stdbool.h>
	#include <pthread.h>
	#include <semaphore.h>
	#include <sys/stat.h>
 	#include <sys/types.h>

	#include <commons/config.h>
	#include <commons/log.h>
	#include <commons/collections/list.h>

	#include <shared/socket.h>
	#include <shared/msg.h>

	#include "conexion_cpu.h"
	#include "safa_protocol.h"
	#include "PCP.h"
	#include "PLP.h"
	#include "consola.h"
	#include "safa_util.h"
	#include "metricas.h"


	/* Constantes */
	#define CONFIG_PATH "../configs/S-AFA.txt"
	#define LOG_DIRECTORY_PATH "../logs/"
	#define LOG_PATH "../logs/S-AFA.log"

	/* Variables globales */

	bool estado_operatorio;
	bool cpu_conectado;
	bool dam_conectado;
	bool op_dummy_en_progreso;

	t_list* cola_new;
	t_list* cola_ready; pthread_mutex_t sem_mutex_cola_ready;
	t_list* cola_exec;
	t_list* cola_block;
	t_list* cola_exit; pthread_mutex_t sem_mutex_cola_exit;

	sem_t sem_bin_crear_dtb_0, sem_bin_crear_dtb_1; // Sem binario entre consola y PLP

	int listening_socket;
	int dam_socket;
	int fd_inotify;
	int ultimo_socket_cpu;

	t_list* cpu_conexiones;
	pthread_mutex_t sem_mutex_cpu_conexiones;

	t_dictionary* tabla_recursos; // Key->nombre_recurso, Value->t_safa_semaforo
	pthread_mutex_t sem_mutex_tabla_recursos;

	sem_t sem_cont_recepcion_interrupcion_cpu;

	t_list* lista_cpus_desalojables; // Para algoritmos con desalojo
	pthread_mutex_t sem_mutex_lista_cpus_desalojables;

	typedef struct{
		int valor;
		t_list* lista_pid_asignados; // Lista de t_vector2 (PID, cant_asignada)
		t_list* lista_pid_bloqueados; // Lista de PID
	} t_safa_semaforo;

	int safa_initialize();
	void inotify_config_iniciar();
	void safa_iniciar_estado_operatorio();

	int safa_send(int socket, e_tipo_msg tipo_msg, ...);

	int safa_manejador_de_eventos(int socket, t_msg* msg); 	// Devuelvo -1 si quiero cerrar esa conexion
	void safa_manejar_inotify();

	bool safa_recursos_wait(unsigned int id, char* nombre_recurso);
	void safa_recursos_signal(unsigned int id, char* nombre_recurso);
	void safa_recursos_liberar_pid(unsigned int id);

	void safa_exit();

#endif /* S_AFA_H_ */
