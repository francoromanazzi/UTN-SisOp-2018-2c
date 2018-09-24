#include "pcp.h"

void pcp_iniciar(){

	/* Creo el hilo que crea los nuevos DTBs */
	if(pthread_create( &thread_pcp_desbloquear_dummy, NULL, (void*) pcp_desbloquear_dummy_iniciar, NULL) ){
		log_error(logger,"No pude crear el hilo PCP_desbloquear_dummy");
		exit(EXIT_FAILURE);
	}
	log_info(logger, "Creo el hilo PCP_desbloquear_dummy");
	pthread_detach(thread_pcp_desbloquear_dummy);

	/* Creo el hilo que carga la base del archivo en un DTB solicitante bloqueado */
	if(pthread_create( &thread_pcp_cargar_recurso, NULL, (void*) pcp_cargar_archivo_iniciar, NULL) ){
		log_error(logger,"No pude crear el hilo PCP_cargar_recurso");
		exit(EXIT_FAILURE);
	}
	log_info(logger, "Creo el hilo PCP_cargar_recurso");
	pthread_detach(thread_pcp_cargar_recurso);

	/* Tengo que esperar que haya algo en la cola de ready, y esperar que haya CPU disponible */
	while(1){
		sem_wait(&sem_cont_cola_ready); // Espero DTBs en READY
		sem_wait(&sem_cont_cpu_conexiones); // Espero CPUs para poder mandarles el DTB

		/* OK, ya puedo elegir otro DTB de ready para mandar a algun CPU */
		pthread_mutex_lock(&sem_mutex_config_retardo);
		usleep(retardo_planificacion);
		pthread_mutex_unlock(&sem_mutex_config_retardo);

		int cpu_socket = pcp_buscar_cpu_ociosa();
		if(cpu_socket == -1){ // Error de sincro, por alguna razon
			log_error(logger, "PCP no encontro un CPU disponible");
			sem_post(&sem_cont_cola_ready);
			sem_post(&sem_cont_cpu_conexiones);
			continue;
		}

		t_dtb* dtb_elegido = pcp_aplicar_algoritmo();
		if(dtb_elegido == NULL){ // Error de sincro, por alguna razon
			log_error(logger, "PCP no encontro un DTB al planificar");
			conexion_cpu_set_active(cpu_socket);
			sem_post(&sem_cont_cola_ready);
			continue;
		}

		log_info(logger, "Muevo a EXEC y mando a ejecutar el DTB con ID: %d con %d unidades de quantum", dtb_elegido->gdt_id, dtb_elegido->quantum_restante);

		pthread_mutex_lock(&sem_mutex_cola_exec);
		list_add(cola_exec, dtb_elegido);
		pthread_mutex_unlock(&sem_mutex_cola_exec);

		safa_send(cpu_socket, EXEC, dtb_elegido);
	} // Fin while(1)
}

t_dtb* pcp_aplicar_algoritmo(){
	pthread_mutex_lock(&sem_mutex_config_algoritmo);
	pthread_mutex_lock(&sem_mutex_cola_ready);

	// TODO: Gestionar algoritmo
	// Aplico FIFO:
	t_dtb* dtb_elegido = (t_dtb*) list_remove(cola_ready, 0);

	if(dtb_elegido == NULL){ // Fallo la planificacion
		pthread_mutex_unlock(&sem_mutex_cola_ready);
		pthread_mutex_unlock(&sem_mutex_config_algoritmo);
		return NULL;
	}

	pthread_mutex_lock(&sem_mutex_config_quantum);
	dtb_elegido -> quantum_restante = quantum;
	pthread_mutex_unlock(&sem_mutex_config_quantum);

	pthread_mutex_unlock(&sem_mutex_cola_ready);
	pthread_mutex_unlock(&sem_mutex_config_algoritmo);
	return dtb_elegido;
}

int pcp_buscar_cpu_ociosa(){
	int i;
	pthread_mutex_lock(&sem_mutex_cpu_conexiones);
	for(i=0;i<cpu_conexiones->elements_count; i++){
		t_conexion_cpu* cpu = list_get(cpu_conexiones, i);
		if(cpu->en_uso == 0){
			cpu->en_uso = 1;
			pthread_mutex_unlock(&sem_mutex_cpu_conexiones);
			return cpu->socket;
		}
	}
	pthread_mutex_unlock(&sem_mutex_cpu_conexiones);
	return -1;
}

void pcp_mover_dtb(unsigned int id, char* cola_inicio, char* cola_destino){

	bool _tiene_mismo_id(void* data){
		return  ((t_dtb*) data) -> gdt_id == id;
	}

	bool _mismo_id_proceso_a_finalizar(void* _id){
		return (unsigned int) _id == id;
	}

	t_dtb* dtb;

	if(!strcmp(cola_inicio, "READY")){
		dtb = list_remove_by_condition(cola_ready, _tiene_mismo_id);
	}
	else if(!strcmp(cola_inicio, "BLOCK")){
		dtb = list_remove_by_condition(cola_block, _tiene_mismo_id);
	}
	else if(!strcmp(cola_inicio, "EXEC")){
		pthread_mutex_lock(&sem_mutex_lista_procesos_a_finalizar);
		if(list_find(lista_procesos_a_finalizar, _mismo_id_proceso_a_finalizar)){ // Tengo que esperar que CPU le devuelva al planificador este proceso
			pthread_mutex_unlock(&sem_mutex_lista_procesos_a_finalizar);
			return;
		}
		else{ // Lo puedo sacar de EXEC sin problemas
			pthread_mutex_unlock(&sem_mutex_lista_procesos_a_finalizar);
			dtb = list_remove_by_condition(cola_exec, _tiene_mismo_id);
		}

	}
	else return;

	if(!strcmp(cola_destino, "EXIT")){
		log_info(logger, "Finalizo el DTB con ID: %d", id);

		pthread_mutex_lock(&sem_mutex_cant_procesos);
		cant_procesos--;
		pthread_mutex_unlock(&sem_mutex_cant_procesos);

		sem_post(&sem_cont_procesos);

		list_add(cola_exit, dtb);

		// Elimino de la lista todas las apariciones de este proceso, que ya he finalizado:
		pthread_mutex_lock(&sem_mutex_lista_procesos_a_finalizar);
		while(list_remove_by_condition(lista_procesos_a_finalizar, _mismo_id_proceso_a_finalizar));
		pthread_mutex_unlock(&sem_mutex_lista_procesos_a_finalizar);
	}
	else if((!strcmp(cola_inicio, "EXEC") || !strcmp(cola_inicio, "BLOCK")) && !strcmp(cola_destino, "READY")){ //EXEC->READY o BLOCK->READY
		log_info(logger, "Muevo a READY el DTB con ID: %d", id);
		list_add(cola_ready, dtb);
		sem_post(&sem_cont_cola_ready);
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
