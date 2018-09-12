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
	#include <shared/msg.h>

	/* Variables globales */
	pthread_t thread_pcp;
	pthread_t thread_plp;

	t_list* cola_new;
	t_list* cola_ready;
	pthread_mutex_t mutex_cola_ready; // Porque tanto PCP como PLP acceden a esta lista
	t_list* cola_block;
	t_list* cola_exec;
	t_list* cola_exit;

	int cant_procesos;

	t_list* rutas_escriptorios_dtb_dummy;
	bool operacion_dummy_en_ejecucion;

	void planificador_iniciar();
	void planificador_crear_dtb_y_encolar(char* path);
	bool planificador_finalizar_dtb(unsigned int id);
	t_dtb* planificador_encontrar_dtb(unsigned int id, char** estado_actual);
	void planificador_cargar_nuevo_path_vacio_en_dtb(t_dtb* dtb_a_actualizar);
	void planificador_cargar_archivo_en_dtb(t_msg* msg);

#endif /* PLANIFICADOR_H_ */
