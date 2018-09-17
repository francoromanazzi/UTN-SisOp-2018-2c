#ifndef PLP_H_
#define PLP_H_
	#include <stdbool.h>
	#include <commons/collections/list.h>
	#include <commons/collections/dictionary.h>
	#include <commons/log.h>
	#include <commons/config.h>
	#include "S-AFA.h"
	#include <shared/DTB.h>
	#include "plp_cargar_recurso.h"
	#include "plp_crear_dtb.h"


	pthread_t thread_plp_crear_dtb;
	pthread_t thread_plp_cargar_recurso;

	void plp_iniciar();
	void plp_mover_dtb(unsigned int id, char* cola_destino);

#endif /* PLP_H_ */
