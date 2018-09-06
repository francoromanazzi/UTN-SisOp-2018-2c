#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <commons/collections/list.h>
	#include <stdbool.h>
	#include <string.h>
	#include "S-AFA.h"
	#include "DTB.h"
	#include "plp.h"
	#include "pcp.h"

	/* Variables globales */
	pthread_t thread_pcp;
	pthread_t thread_plp;

	t_list* cola_new;
	t_list* cola_ready;
	pthread_mutex_t mutex_cola_ready;
	t_list* cola_block;

	int cant_procesos;

	void planificador_iniciar();
	void planificador_crear_dtb_y_encolar(char* path);
	t_dtb* planificador_encontrar_dtb(unsigned int id, char** estado_actual);


#endif /* PLANIFICADOR_H_ */
