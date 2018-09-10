#include "pcp.h"

void pcp_iniciar(){
	/* Tengo que revisar constantemente la cola de ready, y mandarle a las CPU los dtb */
	while(1){
		usleep(retardo_planificacion);
	}
}

void pcp_mover_dtb(unsigned int id, char* cola_inicio, char* cola_destino){

	bool _tiene_mismo_id(void* data){
		return  ((t_dtb*) data) -> gdt_id == id;
	}

	t_dtb* dtb;

	if(!strcmp(cola_inicio, "READY"))
		dtb = list_remove_by_condition(cola_ready, _tiene_mismo_id);
	else if(!strcmp(cola_inicio, "BLOCK"))
		dtb = list_remove_by_condition(cola_block, _tiene_mismo_id);
	else if(!strcmp(cola_inicio, "EXEC")){
		/* TODO: Si cola_destino == "EXEC", esperar a que termine la instruccion */
		dtb = list_remove_by_condition(cola_exec, _tiene_mismo_id);
	}
	else return;

	if(!strcmp(cola_destino, "EXIT")){
		log_info(logger, "Finalizo el DTB con ID: %d", id);
		list_add(cola_exit, dtb);
	}
	else if((!strcmp(cola_inicio, "EXEC") || !strcmp(cola_inicio, "BLOCK")) && !strcmp(cola_destino, "READY")){ //EXEC->READY o BLOCK->READY
		log_info(logger, "Muevo a READY el DTB con ID: %d", id);
		pthread_mutex_lock(&mutex_cola_ready);
		list_add(cola_ready, dtb);
		pthread_mutex_unlock(&mutex_cola_ready);
	}
	else if(!strcmp(cola_inicio, "EXEC") && !strcmp(cola_destino, "BLOCK")){ // EXEC->BLOCK
		log_info(logger, "Bloqueo el DTB con ID: %d", id);
		list_add(cola_block, dtb);
	}
	else if(!strcmp(cola_inicio, "READY") && !strcmp(cola_destino, "EXEC")){ // READY-> EXEC
		log_info(logger, "Muevo a EXEC el DTB con ID: %d", id);
		list_add(cola_exec, dtb);
	}

}
