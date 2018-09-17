#include "plp.h"

void plp_iniciar(){

	bool _flag_inicializado_en_uno(void* dtb){
		return ((t_dtb*) dtb)->flags.inicializado == 1;
	}

	/* Tengo que comparar constantemente la cantidad de procesos y el grado de multiprogramacion */
	while(1){
		//usleep(retardo_planificacion); // No es necesario en PLP

		sem_wait(&sem_cont_cola_new);
		sem_wait(&sem_cont_procesos);


		/* Veo si alguno de esos DTB tiene el flag de inicializado en 1, a.k.a ya puede pasar a READY*/
		t_dtb* dtb_que_puede_pasar_a_ready;
		pthread_mutex_lock(&sem_mutex_cola_new);
		if((dtb_que_puede_pasar_a_ready = list_find(cola_new, _flag_inicializado_en_uno)) != NULL){
			pthread_mutex_unlock(&sem_mutex_cola_new);
			plp_mover_dtb(dtb_que_puede_pasar_a_ready->gdt_id, "READY");
			continue;
		}
		pthread_mutex_unlock(&sem_mutex_cola_new);

		sem_post(&sem_cont_cola_new); // Como no habia ninguno listo para pasar a READY, dejo como estaba la cola de NEW

		/* Espero a que el gestor termine de ejecutar la op dummy, si es que lo estaba haciendo*/
		sem_wait(&sem_bin_op_dummy_1);

		ruta_escriptorio_dtb_dummy = ((t_dtb*) list_get(cola_new, 0)) -> ruta_escriptorio;

		/* Le aviso a gestor que ya puede iniciar otra operacion dummy */
		sem_post(&sem_bin_op_dummy_0);
	} // Fin while(1)
}


void plp_mover_dtb(unsigned int id, char* cola_destino){
	bool _tiene_mismo_id(void* data){
		return  ((t_dtb*) data) -> gdt_id == id;
	}

	pthread_mutex_lock(&sem_mutex_cola_new);
	t_dtb* dtb = list_remove_by_condition(cola_new, _tiene_mismo_id);
	pthread_mutex_unlock(&sem_mutex_cola_new);

	if(!strcmp(cola_destino, "EXIT")){ // NEW -> EXIT
		log_info(logger, "Finalizo el DTB con ID: %d", id);
		cant_procesos--;
		sem_post(&sem_cont_procesos);
		list_add(cola_exit, dtb);
	}
	else if(!strcmp(cola_destino, "READY")){ // NEW -> READY
		log_info(logger, "Muevo a READY el DTB con ID: %d", id);
		cant_procesos++;
		pthread_mutex_lock(&sem_mutex_cola_ready);
		list_add(cola_ready, dtb);
		pthread_mutex_unlock(&sem_mutex_cola_ready);
		sem_post(&sem_cont_cola_ready);
	}
}
