#include "pcp_desbloquear_dummy.h"

void pcp_desbloquear_dummy_iniciar(){

	bool _es_dummy(void* data){
		return  ((t_dtb*) data) -> flags.inicializado == 0;
	}

	while(1){
		/* Espero que PLP me pida desbloquear el DUMMY */
		sem_wait(&sem_bin_desbloquear_dummy);

		/* Cargo en el DUMMY el ID del solicitante y su ruta de escriptorio */
		pthread_mutex_lock(&sem_mutex_cola_block);
		t_dtb* dtb_dummy = list_find(cola_block, _es_dummy);


		dtb_dummy->gdt_id = id_nuevo_dtb;
		dictionary_clean(dtb_dummy->archivos_abiertos); // Reseteo el diccionario de archivos abiertos del dummy

		pthread_mutex_lock(&sem_mutex_ruta_escriptorio_nuevo_dtb);
		dtb_dummy->ruta_escriptorio = ruta_escriptorio_nuevo_dtb;
		pthread_mutex_unlock(&sem_mutex_ruta_escriptorio_nuevo_dtb);

		pthread_mutex_unlock(&sem_mutex_cola_block);
		pcp_mover_dtb(id_nuevo_dtb, "BLOCK", "READY");
	}

}
