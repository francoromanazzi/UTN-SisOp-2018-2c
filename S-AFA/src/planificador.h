#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <commons/collections/list.h>
	#include <stdbool.h>
	#include <string.h>
	#include "S-AFA.h"
	#include <shared/DTB.h>
	#include "plp.h"
	#include "pcp.h"
	#include <shared/msg.h>

	/* Variables globales */
	pthread_t thread_pcp;
	pthread_t thread_plp;

	sem_t sem_cont_procesos;

	sem_t sem_bin_crear_dtb_0; // Para crear un DTB nuevo a la vez
	sem_t sem_bin_crear_dtb_1; //  "     "

	sem_t sem_bin_fin_op_dummy;
	sem_t sem_cont_puedo_iniciar_op_dummy;

	sem_t sem_bin_desbloquear_dummy;

	t_list* cola_new;
	sem_t sem_cont_cola_new;
	pthread_mutex_t sem_mutex_cola_new;

	t_list* cola_ready;
	sem_t sem_cont_cola_ready;
	pthread_mutex_t sem_mutex_cola_ready; // Porque tanto PCP como PLP acceden a esta lista

	t_list* cola_block;
	pthread_mutex_t sem_mutex_cola_block;

	t_list* cola_exec;
	pthread_mutex_t sem_mutex_cola_exec;

	t_list* cola_exit;
	pthread_mutex_t sem_mutex_cola_exit;

	int cant_procesos;
	pthread_mutex_t sem_mutex_cant_procesos;

	int id_nuevo_dtb;
	char* ruta_escriptorio_nuevo_dtb;
	pthread_mutex_t sem_mutex_ruta_escriptorio_nuevo_dtb;

	void planificador_iniciar();
	bool planificador_finalizar_dtb(unsigned int id);
	t_dtb* planificador_encontrar_dtb_y_copiar(unsigned int id, char** estado_actual);
	void planificador_cargar_nuevo_path_vacio_en_dtb(t_dtb* dtb_a_actualizar);
	void planificador_cargar_archivo_en_dtb(t_msg* msg);

#endif /* PLANIFICADOR_H_ */
