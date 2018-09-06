#ifndef PLP_H_
#define PLP_H_
	#include <stdbool.h>
	#include <commons/collections/list.h>
	#include <commons/log.h>
	#include "planificador.h"
	#include "S-AFA.h"
	#include "DTB.h"

	void plp_iniciar();
	void plp_crear_dtb_y_encolar(char* path);
	void plp_desencolar(unsigned int id);

#endif /* PLP_H_ */
