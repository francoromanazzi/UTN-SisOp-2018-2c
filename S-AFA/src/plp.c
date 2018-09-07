#include "plp.h"

void plp_iniciar(){
	/* Tengo que comparar constantemente la cantidad de procesos y el grado de multiprogramacion */
	while(1){
		if(cant_procesos > config_get_int_value(config, "MULTIPROGRAMACION"))
			break;
		if(!list_is_empty(cola_new)){
			/* Inicio operacion dummy */
			// gestor_iniciar_op_dummy();

			/* Fin operacion dummy */
			// plp_mover_dtb(id,"READY");
		}
	}
}

void plp_crear_dtb_y_encolar(char* path){
	t_dtb* nuevo_dtb = dtb_create(path);
	list_add(cola_new, nuevo_dtb);
	log_info(logger, "Creo el DTB con ID: %d del escriptorio: %s", nuevo_dtb->gdt_id, path);
}

void plp_mover_dtb(unsigned int id, char* cola_destino){
	bool _tiene_mismo_id(void* data){
		return  ((t_dtb*) data) -> gdt_id == id;
	}

	t_dtb* dtb = list_remove_by_condition(cola_new, _tiene_mismo_id);

	if(!strcmp(cola_destino, "EXIT")){
		log_info(logger, "Finalizo el DTB con ID: %d", id);
		list_add(cola_exit, dtb);
	}
	else if(!strcmp(cola_destino, "READY")){
		log_info(logger, "Muevo a READY el DTB con ID: %d", id);
		pthread_mutex_lock(&mutex_cola_ready);
		list_add(cola_ready, dtb);
		pthread_mutex_unlock(&mutex_cola_ready);
	}
}
