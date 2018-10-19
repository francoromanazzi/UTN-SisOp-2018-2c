#ifndef PLP_H_
#define PLP_H_

	#include "S-AFA.h"
	#include <shared/protocol.h>

	int cant_procesos;
	bool dummy_disponible;

	void plp_iniciar();
	void plp_gestionar_msg(t_safa_msg* msg);
	t_dtb* plp_encontrar_dtb(unsigned int id);
	void plp_crear_dtb_y_encolar(char* path_escriptorio);
	void plp_intentar_iniciar_op_dummy();
	void plp_cargar_archivo(t_dtb* dtb_a_actualizar, t_list* lista_direcciones);
	t_status* plp_empaquetar_status();
	void plp_mover_dtb(unsigned int id, e_estado fin);

#endif /* PLP_H_ */
