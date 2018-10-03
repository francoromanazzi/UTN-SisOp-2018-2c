#include "PCP.h"

void pcp_iniciar(){
	lista_procesos_a_finalizar_en_exec = list_create();

	/* Espero mensajes */
	while(1){
		sem_wait(&sem_cont_cola_msg_pcp);
		t_safa_msg* msg = safa_protocol_desencolar_msg(PCP);
		pcp_gestionar_msg(msg);
		safa_protocol_msg_free(msg);
	}
}

void pcp_gestionar_msg(t_safa_msg* msg){
	int ok, base;
	unsigned int id;
	char* path;
	t_dtb* dtb;
	void* data;
	t_status* status;

	bool _mismo_id(void* _dtb){
		return ((t_dtb*) _dtb)->gdt_id == id;
	}

	bool _mismo_id_v2(void* _id){
		return dtb->gdt_id == (unsigned int) _id;
	}

	switch(msg->emisor){
		case S_AFA:
			switch(msg->tipo_msg){
				case READY_DTB:
					dtb = (t_dtb*) msg->data;

					/* Me fijo si me habian pedido finalizar este DTB*/
					if(list_find(lista_procesos_a_finalizar_en_exec, _mismo_id_v2)){ // Me habian pedido finalizarlo
						list_remove_by_condition(lista_procesos_a_finalizar_en_exec, _mismo_id_v2);
						log_info(logger, "[PCP] Finalizo el DTB con ID: %d (solicitado desde consola) (tuve que esperarlo a que vuelva de EXEC)", dtb->gdt_id);
						pcp_mover_dtb(dtb->gdt_id, ESTADO_EXEC, ESTADO_EXIT);
					}
					else{
						log_info(logger, "[PCP] Muevo a READY el DTB con ID: %d", ((t_dtb*) (msg->data))->gdt_id);
						pcp_actualizar_dtb(dtb);
						pcp_mover_dtb(dtb->gdt_id, ESTADO_EXEC, ESTADO_READY);
					}
					dtb_destroy(dtb);
					msg->data=NULL;
				break;

				case BLOCK_DTB:
					dtb = (t_dtb*) msg->data;

					/* Me fijo si me habian pedido finalizar este DTB*/
					if((id = (int) list_find(lista_procesos_a_finalizar_en_exec, _mismo_id_v2)) && !dtb_es_dummy((void*) dtb)){ // Me habian pedido finalizarlo
						list_remove_by_condition(lista_procesos_a_finalizar_en_exec, _mismo_id_v2);
						log_info(logger, "[PCP] Finalizo el DTB con ID: %d (solicitado desde consola) (tuve que esperarlo a que vuelva de EXEC)", dtb->gdt_id);
						pcp_mover_dtb(dtb->gdt_id, ESTADO_EXEC, ESTADO_EXIT);
					}
					else{
						if(dtb_es_dummy((void*) dtb)){
							log_info(logger, "[PCP] Muevo a BLOCK el DUMMY. El solicitante tiene ID: %d", ((t_dtb*) (msg->data))->gdt_id);
							pcp_actualizar_dtb(dtb);
							pcp_mover_dtb(0, ESTADO_EXEC, ESTADO_BLOCK);
							safa_protocol_encolar_msg_y_avisar(PCP, PLP, DUMMY_DISPONIBLE);
						}

						else{
							log_info(logger, "[PCP] Muevo a BLOCK el DTB con ID: %d", ((t_dtb*) (msg->data))->gdt_id);
							pcp_actualizar_dtb(dtb);
							pcp_mover_dtb(dtb->gdt_id, ESTADO_EXEC, ESTADO_BLOCK);
						}
					}
					dtb_destroy(dtb);
					msg->data=NULL;
				break;

				case EXIT_DTB:
					dtb = (t_dtb*) msg->data;

					/* Me fijo si me habian pedido finalizar este DTB*/
					if(list_find(lista_procesos_a_finalizar_en_exec, _mismo_id_v2)){ // Me habian pedido finalizarlo
						list_remove_by_condition(lista_procesos_a_finalizar_en_exec, _mismo_id_v2);
						log_info(logger, "[PCP] Finalizo el DTB con ID: %d (solicitado desde consola) (tuve que esperarlo a que vuelva de EXEC)", dtb->gdt_id);
					}
					else{
						if(dtb->flags.error_nro != OK){
							log_error(logger, "Error %d", dtb->flags.error_nro);
						}

						log_info(logger, "[PCP] Finalizo el DTB con ID: %d", dtb->gdt_id);
						pcp_actualizar_dtb(dtb);

					}
					pcp_mover_dtb(dtb->gdt_id, ESTADO_EXEC, ESTADO_EXIT);
					dtb_destroy(dtb);
					msg->data=NULL;
				break;

				case NUEVO_CPU_DISPONIBLE:
					pcp_intentar_ejecutar_dtb();
				break;
			}
		break;

		case PLP:
			switch(msg->tipo_msg){
				case NUEVO_DTB_EN_READY:
					pcp_intentar_ejecutar_dtb();
				break;

				case DESBLOQUEAR_DUMMY:
					log_info(logger, "[PCP] Recibi de PLP un pedido de desbloqueo de DUMMY");
					safa_protocol_desempaquetar_desbloquear_dummy(msg->data, &id, &path);

					/* Encuentro al DUMMY*/
					dtb = (t_dtb*) list_find(cola_block, dtb_es_dummy);
					dtb->gdt_id = id;
					free(dtb->ruta_escriptorio);
					dtb->ruta_escriptorio = path;
					log_info(logger, "[PCP] Cargo en el DUMMY: ID: %d ESCRIPTORIO: %s",dtb->gdt_id, dtb->ruta_escriptorio);

					log_info(logger, "[PCP] Muevo el DUMMY de BLOCK a READY");
					pcp_mover_dtb(id, ESTADO_BLOCK, ESTADO_READY);
				break;

				case RESULTADO_ABRIR_DAM:
					safa_protocol_desempaquetar_resultado_abrir(msg->data, &ok, &id, &path, &base);

					if((dtb = list_find(cola_block, _mismo_id)) == NULL){
						log_error(logger, "[PCP] No pude cargar el archivo %s en el DTB con ID: %d porque no lo encontre en BLOCK y PLP no lo habia encontrado en NEW", path, id);
					}
					else{
						if(ok == OK){
							log_info(logger, "[PCP] Cargo el archivo %s en el DTB con ID: %d", path, id);
							pcp_cargar_archivo(dtb, path, base);
							pcp_mover_dtb(id, ESTADO_BLOCK, ESTADO_READY);
						}
						else{
							log_error(logger, "[PCP] Error %d", ok);
							pcp_mover_dtb(id, ESTADO_BLOCK, ESTADO_EXIT);
						}
					}
				break;

				case EXIT_DTB_CONSOLA:
					id = (unsigned int) msg->data;
					if((dtb = pcp_encontrar_dtb(id)) == NULL){ // No encontre el DTB en el sistema
						safa_protocol_encolar_msg_y_avisar(PCP, CONSOLA, EXIT_DTB_CONSOLA, id, 0);
					}
					else{
						if(dtb->estado_actual == ESTADO_EXIT){ // Si ya estaba en exit
							safa_protocol_encolar_msg_y_avisar(PCP, CONSOLA, EXIT_DTB_CONSOLA, id, 3);
						}
						else if(dtb->estado_actual != ESTADO_EXEC){ // No esta en exec, lo puedo finalizar inmediatamente
							safa_protocol_encolar_msg_y_avisar(PCP, CONSOLA, EXIT_DTB_CONSOLA, id, 1);
							pcp_mover_dtb(id, dtb->estado_actual, ESTADO_EXIT);
						}
						else{ // Tengo que esperar que vuelva de EXEC
							safa_protocol_encolar_msg_y_avisar(PCP, CONSOLA, EXIT_DTB_CONSOLA, id, 2);
							list_add(lista_procesos_a_finalizar_en_exec, (void*) id);
						}
					}
					msg->data = NULL;
				break;

				case STATUS:
					status = status_copiar((t_status*) (msg->data));
					safa_protocol_encolar_msg_y_avisar(PCP, CONSOLA, STATUS, pcp_empaquetar_status(status));
					status_free((t_status*) (msg->data));
					msg->data = NULL;
				break;

				case STATUS_PCB:
					id = (unsigned int) msg->data;
					dtb = dtb_copiar(pcp_encontrar_dtb(id));
					safa_protocol_encolar_msg_y_avisar(PCP, CONSOLA, STATUS_PCB, dtb);
					msg->data = NULL;
				break;

			}
		break;

		case PCP:
			switch(msg->tipo_msg){
				case NUEVO_DTB_EN_READY:
					pcp_intentar_ejecutar_dtb();
				break;

				case NUEVO_CPU_DISPONIBLE:
					pcp_intentar_ejecutar_dtb();
				break;
			}
		break;

		case CONSOLA:
			switch(msg->tipo_msg){

			}
		break;
	}
}

void pcp_intentar_ejecutar_dtb(){
	/* 1RA CONDICION: Que haya DTB en READY */
	pthread_mutex_lock(&sem_mutex_cola_ready);
	if(list_is_empty(cola_ready)){
		pthread_mutex_unlock(&sem_mutex_cola_ready);
		return;
	}
	pthread_mutex_unlock(&sem_mutex_cola_ready);

	/* 2DA CONDICION: Que haya CPU disponible */
	int cpu_socket;
	if((cpu_socket = conexion_cpu_get_available()) == -1){
		return;
	}

	/* OK, mando a ejecutar un DTB segun el algoritmo */
	log_info(logger, "[PCP] Puedo mandar a ejecutar un DTB");

	usleep(safa_config_get_int_value("RETARDO_PLANIF"));

	char* algoritmo = safa_config_get_string_value("ALGORITMO");
	t_dtb* dtb_seleccionado = NULL;

	pthread_mutex_lock(&sem_mutex_cola_ready);
	if((dtb_seleccionado = list_find(cola_ready, dtb_es_dummy)) == NULL){ // Si no encontre al DUMMY, busco segun el algoritmo
		if(!strcmp(algoritmo, "RR")){
			if((dtb_seleccionado = list_get(cola_ready, 0)) != NULL){ // Fifo
				dtb_seleccionado->quantum_restante = safa_config_get_int_value("QUANTUM"); // Reseteo quantum
			}
		}
		else if(!strcmp(algoritmo, "VRR")){
			if((dtb_seleccionado = list_find(cola_ready, dtb_le_queda_quantum)) == NULL){ // Si no encontre en la cola de max prioridad
				if((dtb_seleccionado = list_get(cola_ready, 0)) != NULL){ // Fifo
					dtb_seleccionado->quantum_restante = safa_config_get_int_value("QUANTUM"); // Reseteo quantum
				}
			}
		}
		else if(!strcmp(algoritmo, "PROPIO")){

		}
	}
	pthread_mutex_unlock(&sem_mutex_cola_ready);

	if(dtb_seleccionado == NULL){ // Ocurrio un error de sincro
		log_error(logger, "[PCP] No encontre DTB al planificar");
		conexion_cpu_set_active(cpu_socket);
		return;
	}

	if(dtb_es_dummy((void*) dtb_seleccionado))
		log_info(logger, "[PCP] Mando a ejecutar el DUMMY. El ID del solicitante es: %d", dtb_seleccionado->gdt_id);
	else
		log_info(logger, "[PCP] Mando a ejecutar el DTB con ID: %d por %d unidades de quantum", dtb_seleccionado->gdt_id, dtb_seleccionado->quantum_restante);

	pcp_mover_dtb(dtb_seleccionado->gdt_id, ESTADO_READY, ESTADO_EXEC);
	safa_send(cpu_socket, EXEC, dtb_seleccionado);
}

void pcp_actualizar_dtb(t_dtb* dtb_recibido){
	bool _mismo_id(void* dtb){
		return ((t_dtb*) dtb)->gdt_id == dtb_recibido->gdt_id;
	}

	t_dtb* dtb_a_actualizar;

	void _actualizar_archivos_abiertos(char* key, void* data){
		dictionary_put(dtb_a_actualizar->archivos_abiertos, key, data);
	}

	if(dtb_es_dummy(dtb_recibido)){
		dtb_a_actualizar = list_find(cola_exec, dtb_es_dummy);
		dtb_a_actualizar->gdt_id = 0;
		free(dtb_a_actualizar->ruta_escriptorio);
		dtb_a_actualizar->ruta_escriptorio = strdup("");
	}
	else{
		dtb_a_actualizar = list_find(cola_exec, _mismo_id);
		dtb_a_actualizar->pc = dtb_recibido->pc;
		dtb_a_actualizar->quantum_restante = dtb_recibido->quantum_restante;
		dtb_a_actualizar->flags.error_nro = dtb_recibido->flags.error_nro;
		dictionary_clean(dtb_a_actualizar->archivos_abiertos); // Borro los archivos abiertos
		dictionary_iterator(dtb_recibido->archivos_abiertos, _actualizar_archivos_abiertos); // Los vuelvo a poner, uno por uno
	}
}

void pcp_cargar_archivo(t_dtb* dtb_a_actualizar, char* path, int base){
	dictionary_remove(dtb_a_actualizar->archivos_abiertos, path);
	dictionary_put(dtb_a_actualizar->archivos_abiertos, path, (void*) base);
}

t_dtb* pcp_encontrar_dtb(unsigned int id){

	bool _mismo_id(void* dtb){
		return ((t_dtb*) dtb)->gdt_id == id;
	}
	t_dtb* ret;

	pthread_mutex_lock(&sem_mutex_cola_ready);
	if((ret = list_find(cola_ready, _mismo_id)) != NULL){
		pthread_mutex_unlock(&sem_mutex_cola_ready);
		return ret;
	}
	pthread_mutex_unlock(&sem_mutex_cola_ready);

	if((ret = list_find(cola_exec, _mismo_id)) != NULL){
		return ret;
	}

	if((ret = list_find(cola_block, _mismo_id)) != NULL){
		return ret;
	}

	pthread_mutex_lock(&sem_mutex_cola_exit);
	if((ret = list_find(cola_exit, _mismo_id)) != NULL){
		pthread_mutex_unlock(&sem_mutex_cola_exit);
		return ret;
	}
	pthread_mutex_unlock(&sem_mutex_cola_exit);
	return NULL;
}

t_status* pcp_empaquetar_status(t_status* status){

	list_destroy(status->ready);
	list_destroy(status->exec);
	list_destroy(status->block);
	list_destroy(status->exit);

	pthread_mutex_lock(&sem_mutex_cola_ready);
	status->ready = list_duplicate(cola_ready);
	pthread_mutex_unlock(&sem_mutex_cola_ready);
	list_iterate(status->ready, dtb_copiar_sobreescribir);

	status->exec = list_duplicate(cola_exec);
	list_iterate(status->exec, dtb_copiar_sobreescribir);

	status->block = list_duplicate(cola_block);
	list_iterate(status->block, dtb_copiar_sobreescribir);

	pthread_mutex_lock(&sem_mutex_cola_exit);
	status->exit = list_duplicate(cola_exit);
	pthread_mutex_unlock(&sem_mutex_cola_exit);
	list_iterate(status->exit, dtb_copiar_sobreescribir);
	return status;
}

void pcp_mover_dtb(unsigned int id, e_estado inicio, e_estado fin){

	bool _mismo_id(void* dtb){
		return ((t_dtb*) dtb)->gdt_id == id;
	}

	t_dtb* dtb;

	switch(inicio){
		case ESTADO_READY:
			pthread_mutex_lock(&sem_mutex_cola_ready);
			dtb = list_remove_by_condition(cola_ready, _mismo_id);
			pthread_mutex_unlock(&sem_mutex_cola_ready);
		break;

		case ESTADO_BLOCK:
			dtb = list_remove_by_condition(cola_block, _mismo_id);
		break;

		case ESTADO_EXEC:
			dtb = list_remove_by_condition(cola_exec, _mismo_id);
		break;
	}

	switch(fin){
		case ESTADO_READY:
			dtb->estado_actual = ESTADO_READY;
			pthread_mutex_lock(&sem_mutex_cola_ready);
			list_add(cola_ready, (void*) dtb);
			pthread_mutex_unlock(&sem_mutex_cola_ready);
			safa_protocol_encolar_msg_y_avisar(PCP, PCP, NUEVO_DTB_EN_READY);
		break;

		case ESTADO_BLOCK:
			dtb->estado_actual = ESTADO_BLOCK;
			list_add(cola_block, (void*) dtb);
			safa_protocol_encolar_msg_y_avisar(PCP, PCP, NUEVO_CPU_DISPONIBLE);
		break;

		case ESTADO_EXEC:
			dtb->estado_actual = ESTADO_EXEC;
			list_add(cola_exec, (void*) dtb);
		break;

		case ESTADO_EXIT:
			if(dtb->estado_actual == ESTADO_EXEC) safa_protocol_encolar_msg_y_avisar(PCP, PCP, NUEVO_CPU_DISPONIBLE);

			dtb->estado_actual = ESTADO_EXIT;
			pthread_mutex_lock(&sem_mutex_cola_exit);
			list_add(cola_exit, (void*) dtb);
			pthread_mutex_unlock(&sem_mutex_cola_exit);
			safa_protocol_encolar_msg_y_avisar(PCP, PLP, PROCESO_FINALIZADO);
		break;
	}
}
