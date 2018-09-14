#ifndef PLP_H_
#define PLP_H_
	#include <stdbool.h>
	#include <commons/collections/list.h>
	#include <commons/collections/dictionary.h>
	#include <commons/log.h>
	#include <commons/config.h>
	#include "S-AFA.h"
	#include <shared/DTB.h>

	void plp_iniciar();
	void plp_crear_dtb_y_encolar(char* path);
	void plp_mover_dtb(unsigned int id, char* cola_destino);

#endif /* PLP_H_ */
