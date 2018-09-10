#ifndef PCP_H_
#define PCP_H_
	#include <stdbool.h>
	#include <string.h>
	#include <commons/collections/list.h>
	#include <commons/log.h>
	#include <commons/config.h>
	#include <shared/protocol.h>
	#include "planificador.h"
	#include "S-AFA.h"
	#include "DTB.h"

	void pcp_iniciar();
	t_dtb* pcp_aplicar_algoritmo();
	int pcp_buscar_cpu_ociosa();
	void pcp_mover_dtb(unsigned int id, char* cola_inicio, char* cola_destino);

#endif /* PCP_H_ */
