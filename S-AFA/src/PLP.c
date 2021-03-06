#include "PLP.h"

void plp_iniciar(){
	cant_procesos = 0;
	dummy_disponible = true, avisar_a_pcp_nuevo_DTB_en_READY= false;
	lista_procesos_a_mover_a_ready_finalizados_por_consola = list_create();

	/* Espero mensajes */
	while(1){
		sem_wait(&sem_cont_cola_msg_plp);
		t_safa_msg* msg = safa_protocol_desencolar_msg(PLP);
		plp_gestionar_msg(msg);
		safa_protocol_msg_free(msg);
	}
}

void plp_gestionar_msg(t_safa_msg* msg){
	int ok, base, data_len;
	unsigned int id;
	char* path;
	t_dtb* dtb;
	void* data;

	bool _mismo_id(void* dtb){
		return ((t_dtb*) dtb)->gdt_id == id;
	}

	bool _mismo_id_v2(void* _id){
		return id == (unsigned int) _id;
	}

	void _aumentar_sentencias_ejecutadas(void* _dtb){
		t_dtb* dtb = (t_dtb*) _dtb;
		(dtb->metricas.cant_sentencias_ejecutadas_en_new)++;
	}

	switch(msg->emisor){
		case S_AFA:
			switch(msg->tipo_msg){
				case RESULTADO_ABRIR_DAM:
					safa_protocol_desempaquetar_resultado_abrir(msg->data, &ok, &id, &path, &base);

					if(list_remove_by_condition(lista_procesos_a_mover_a_ready_finalizados_por_consola, _mismo_id_v2) != NULL){ // Lo habian finalizado por consola, no tengo que hacer nada
						free(path);
						return;
					}

					if((dtb = list_find(cola_new, _mismo_id)) == NULL){
						safa_protocol_encolar_msg_y_avisar(PLP, PCP, RESULTADO_ABRIR_DAM, msg->data);
					}
					else{
						if(ok == OK){
							log_info(logger, "[PLP] Finalizo la op DUMMY del DTB con ID: %d. Lo muevo a READY", id);
							plp_cargar_archivo(dtb, base);
							plp_mover_dtb(id, ESTADO_READY);
						}
						else{
							log_error(logger, "[PLP] Error %d", ok);
							log_info(logger, "[PLP] Finalizo el DTB con ID: %d", id);
							dtb->flags.error_nro = ok;
							plp_mover_dtb(id, ESTADO_EXIT);
						}
					}
					free(path);
				break;

				case GRADO_MULTIPROGRAMACION_AUMENTADO:
					plp_intentar_iniciar_op_dummy();
				break;

				case SENTENCIA_EJECUTADA_:
					list_iterate(cola_new, _aumentar_sentencias_ejecutadas);
				break;
			}
		break;

		case PLP:
			switch(msg->tipo_msg){
				case PROCESO_FINALIZADO:
					cant_procesos--;
					plp_intentar_iniciar_op_dummy();
				break;

				case FIN_OP_DUMMY:
					op_dummy_en_progreso = false;
					plp_intentar_iniciar_op_dummy();
					if(avisar_a_pcp_nuevo_DTB_en_READY){
						safa_protocol_encolar_msg_y_avisar(PLP, PCP, NUEVO_DTB_EN_READY);
						avisar_a_pcp_nuevo_DTB_en_READY = false;
					}

				break;
			}
		break;

		case PCP:
			switch(msg->tipo_msg){
				case PROCESO_FINALIZADO:
					cant_procesos--;
					plp_intentar_iniciar_op_dummy();
				break;

				case DUMMY_DISPONIBLE:
					dummy_disponible = true;
					plp_intentar_iniciar_op_dummy();
				break;
			}
		break;

		case CONSOLA:
			switch(msg->tipo_msg){
				case CREAR_DTB:
					plp_crear_dtb_y_encolar((char*) msg->data);
					plp_intentar_iniciar_op_dummy();
				break;

				case EXIT_DTB_CONSOLA:
					id = (unsigned int) msg->data;
					if((dtb = plp_encontrar_dtb(id)) == NULL){ // Ese DTB no estaba en NEW, le aviso a PCP
						safa_protocol_encolar_msg_y_avisar(PLP, PCP, EXIT_DTB_CONSOLA, id);
					}
					else{ // Finalizo ese DTB
						log_info(logger, "[PLP] Finalizo el DTB con ID: %d (solicitado desde consola)", id);
						safa_protocol_encolar_msg_y_avisar(PLP, CONSOLA, EXIT_DTB_CONSOLA, id, 1);
						if(id == id_solicitante_actual_op_dummy){ // Este proceso estaba esperando un resultado de abrir escriptorio
							list_add(lista_procesos_a_mover_a_ready_finalizados_por_consola, (void*) id);
						}
						plp_mover_dtb(id, ESTADO_EXIT);

					}
					msg->data = NULL;
				break;

				case STATUS:
					safa_protocol_encolar_msg_y_avisar(PLP, PCP, STATUS, plp_empaquetar_status());
				break;

				case STATUS_DTB:
					id = (unsigned int) msg->data;
					if((dtb = dtb_copiar(list_find(cola_new, _mismo_id))) == NULL){
						safa_protocol_encolar_msg_y_avisar(PLP, PCP, STATUS_DTB, msg->data);
					}
					else{
						safa_protocol_encolar_msg_y_avisar(PLP, CONSOLA, STATUS_DTB, dtb);
					}
					msg->data = NULL;
				break;

				case METRICAS_DTB:
					id = (unsigned int) msg->data;
					if((dtb = list_find(cola_new, _mismo_id)) == NULL)
						safa_protocol_encolar_msg_y_avisar(PLP, PCP, METRICAS_DTB, msg->data);
					else
						safa_protocol_encolar_msg_y_avisar(PLP, CONSOLA, METRICAS_DTB, dtb->metricas.cant_sentencias_ejecutadas_en_new);

					msg->data = NULL;
				break;

				case METRICAS_SENTENCIAS_EXIT:
					safa_protocol_encolar_msg_y_avisar(PLP, CONSOLA, METRICAS_SENTENCIAS_EXIT, plp_metricas_sentencias_exit());
				break;
			}
		break;
	}
}

t_dtb* plp_encontrar_dtb(unsigned int id){

	bool _mismo_id(void* dtb){
		return ((t_dtb*) dtb)->gdt_id == id;
	}

	return (t_dtb*) list_find(cola_new, _mismo_id);
}

void plp_crear_dtb_y_encolar(char* path_escriptorio){
	sem_wait(&sem_bin_crear_dtb_0);
	list_add(cola_new, dtb_create(path_escriptorio));
	sem_post(&sem_bin_crear_dtb_1);
}

void plp_intentar_iniciar_op_dummy(){
	if(op_dummy_en_progreso) return;
	if(!dummy_disponible) return;
	if(list_is_empty(cola_new)) return;
	if(cant_procesos >= safa_config_get_int_value("MULTIPROGRAMACION")) return;

	/* OK, puedo iniciar una OP dummy */
	op_dummy_en_progreso = true;
	dummy_disponible = false;
	cant_procesos++;
	t_dtb* dtb_elegido = (t_dtb*) list_get(cola_new, 0);
	id_solicitante_actual_op_dummy = dtb_elegido->gdt_id;
	log_info(logger, "[PLP] Inicio OP DUMMY, ID: %d, ESCRIPTORIO: %s", dtb_elegido->gdt_id, dtb_elegido->ruta_escriptorio);
	log_info(logger, "[PLP] Le pido a PCP desbloquear el dummy");
	safa_protocol_encolar_msg_y_avisar(PLP, PCP, DESBLOQUEAR_DUMMY, dtb_elegido->gdt_id, dtb_elegido->ruta_escriptorio);
}

void plp_cargar_archivo(t_dtb* dtb_a_actualizar, int base){
	dictionary_remove(dtb_a_actualizar->archivos_abiertos, dtb_a_actualizar->ruta_escriptorio);
	dictionary_put(dtb_a_actualizar->archivos_abiertos, dtb_a_actualizar->ruta_escriptorio, (void*) base);
}

int plp_metricas_sentencias_exit(){
	int cant_dtbs = 0, acumulador_cant_sentencias = 0;

	void _acumular_sentencias(void* dtb){
		cant_dtbs++;
		acumulador_cant_sentencias += ((t_dtb*) dtb)->metricas.cant_sentencias_totales;
	}

	pthread_mutex_lock(&sem_mutex_cola_exit);
	list_iterate(cola_exit, _acumular_sentencias);
	pthread_mutex_unlock(&sem_mutex_cola_exit);

	return cant_dtbs > 0 ? acumulador_cant_sentencias / cant_dtbs : -1;
}

t_status* plp_empaquetar_status(){
	t_status* ret = malloc(sizeof(t_status));

	void dtb_duplicar_y_agregar_a_ret_new(void* dtb_original){
		list_add(ret->new, (void*) dtb_copiar(((t_dtb*) dtb_original)));
	}


	ret->cant_procesos_activos = cant_procesos;

	ret->new = list_create();
	list_iterate(cola_new, dtb_duplicar_y_agregar_a_ret_new);

	ret->ready = list_create();
	ret->exec = list_create();
	ret->block = list_create();
	ret->exit = list_create();

	return ret;
}

void plp_mover_dtb(unsigned int id, e_estado fin){

	bool _mismo_id(void* dtb){
		return ((t_dtb*) dtb)->gdt_id == id;
	}

	t_dtb* dtb = list_remove_by_condition(cola_new, _mismo_id);

	switch(fin){
		case ESTADO_READY:
			dtb->estado_actual = ESTADO_READY;
			pthread_mutex_lock(&sem_mutex_cola_ready);
			list_add(cola_ready, (void*) dtb);
			pthread_mutex_unlock(&sem_mutex_cola_ready);

			avisar_a_pcp_nuevo_DTB_en_READY = true;
			safa_protocol_encolar_msg_y_avisar(PLP, PLP, FIN_OP_DUMMY);
		break;

		case ESTADO_EXIT:
			dtb->estado_actual = ESTADO_EXIT;
			pthread_mutex_lock(&sem_mutex_cola_exit);
			list_add(cola_exit, (void*) dtb);
			pthread_mutex_unlock(&sem_mutex_cola_exit);

			if(id == id_solicitante_actual_op_dummy){
				safa_protocol_encolar_msg_y_avisar(PLP, PLP, PROCESO_FINALIZADO);
				safa_protocol_encolar_msg_y_avisar(PLP, PLP, FIN_OP_DUMMY);
			}
		break;
	}
}








