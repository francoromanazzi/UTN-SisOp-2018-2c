#include "gestor.h"

void gestor_iniciar(){

	bool _es_dummy(void* data){
		return  ((t_dtb*) data) -> gdt_id == 0;
	}

	/* Creo el hilo consola del gestor de programas */
	if(pthread_create( &thread_gestor_consola, NULL, (void*) gestor_consola_iniciar, NULL) ){
		log_error(logger,"No pude crear el hilo para la consola del gestor");
			safa_exit();
		exit(EXIT_FAILURE);
	}
	log_info(logger, "Creo el hilo para la consola del gestor");
	pthread_detach(thread_gestor);

	while(1){
		/* Espero pedido de PLP para iniciar operacion DUMMY */
		sem_wait(&sem_bin_op_dummy_0);

		/* Inicio op dummy */
		// Le cargo al dummy el escriptorio
		t_dtb* dtb_dummy = list_find(cola_block, _es_dummy);
		dtb_dummy->ruta_escriptorio = ruta_escriptorio_dtb_dummy;

		pcp_mover_dtb(0, "BLOCK", "READY"); // Desbloqueo dummy

		sem_wait(&sem_bin_fin_op_dummy); // Espero que PCP reciba de DAM la base del escriptorio
		/* Aviso a PLP que ya termine la op dummy */
		sem_post(&sem_bin_op_dummy_1);
	}
}


