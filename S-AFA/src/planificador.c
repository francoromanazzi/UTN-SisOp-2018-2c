#include "planificador.h"

void planificador_iniciar(){
	cant_procesos = 0;
	pthread_mutex_init(&sem_mutex_cant_procesos, NULL);
	sem_init(&sem_cont_procesos, 0, config_get_int_value(config, "MULTIPROGRAMACION"));

	sem_init(&sem_bin_crear_dtb_0, 0, 0);
	sem_init(&sem_bin_crear_dtb_1, 0, 1);

	sem_init(&sem_cont_puedo_iniciar_op_dummy, 0 ,1);

	sem_init(&sem_bin_desbloquear_dummy, 0, 0);

	ruta_escriptorio_nuevo_dtb = NULL;

	pthread_mutex_init(&sem_mutex_ruta_escriptorio_nuevo_dtb, NULL);

	cola_new = list_create();
	pthread_mutex_init(&sem_mutex_cola_new, NULL);
	sem_init(&sem_cont_cola_new, 0, 0);

	cola_ready = list_create();
	pthread_mutex_init(&sem_mutex_cola_ready, NULL);
	sem_init(&sem_cont_cola_ready, 0, 0);

	cola_block = list_create();
	pthread_mutex_init(&sem_mutex_cola_block, NULL);
	pthread_mutex_lock(&sem_mutex_cola_block);
	list_add(cola_block, dtb_create_dummy());
	pthread_mutex_unlock(&sem_mutex_cola_block);

	cola_exec = list_create();
	pthread_mutex_init(&sem_mutex_cola_exec, NULL);

	cola_exit = list_create();
	pthread_mutex_init(&sem_mutex_cola_exit, NULL);

	if(pthread_create( &thread_pcp, NULL, (void*) pcp_iniciar, NULL) ){
		log_error(logger,"No pude crear el hilo PCP");
		exit(EXIT_FAILURE);
	}
	log_info(logger, "Creo el hilo PCP");


	if(pthread_create( &thread_plp, NULL, (void*) plp_iniciar, NULL) ){
		log_error(logger,"No pude crear el hilo PLP");
		exit(EXIT_FAILURE);
	}
	log_info(logger, "Creo el hilo PLP");

	pthread_detach(thread_pcp);
	pthread_detach(thread_plp);

	t_msg* msg;
	while(1){ // Espero mensajes de SAFA
		sem_wait(&sem_cont_cola_mensajes);
		pthread_mutex_lock(&sem_mutex_cola_mensajes);
		msg = msg_duplicar(list_get(cola_mensajes, 0)); // Veo cual es el primer mensaje, y me lo copio (lo libero al final del while)

		if(msg->header->emisor == SAFA && msg->header->tipo_mensaje == CREAR){ // Me interesa este mensaje
			/* Gestor de programas me pidio crear el DTB */
			list_remove_and_destroy_element(cola_mensajes, 0, msg_free_v2);
			pthread_mutex_unlock(&sem_mutex_cola_mensajes);

			sem_wait(&sem_bin_crear_dtb_1); // Espero a que termine la creacion de un DTB anterior
			pthread_mutex_lock(&sem_mutex_ruta_escriptorio_nuevo_dtb);
			ruta_escriptorio_nuevo_dtb = desempaquetar_string(msg);
			pthread_mutex_unlock(&sem_mutex_ruta_escriptorio_nuevo_dtb);
			sem_post(&sem_bin_crear_dtb_0); // Le aviso a thread_plp_crear_dtb
		}
		else if(msg->header->emisor == CPU && msg->header->tipo_mensaje == BLOCK){ // Me interesa este mensaje
			list_remove_and_destroy_element(cola_mensajes, 0, msg_free_v2);
			pthread_mutex_unlock(&sem_mutex_cola_mensajes);

			t_dtb* dtb_recibido;
			dtb_recibido = desempaquetar_dtb(msg);
			planificador_actualizar_archivos_dtb(dtb_recibido);

			/* Por ahi me habian pedido finalizar este DTB (por ejemplo, en pcp_mover_dtb) */



			pcp_mover_dtb(dtb_recibido->gdt_id, "EXEC", "BLOCK");
			dtb_destroy(dtb_recibido);
		}
		else if(msg->header->emisor == CPU && msg->header->tipo_mensaje == READY){ // Me interesa este mensaje
			list_remove_and_destroy_element(cola_mensajes, 0, msg_free_v2);
			pthread_mutex_unlock(&sem_mutex_cola_mensajes);

			t_dtb* dtb_recibido;
			dtb_recibido = desempaquetar_dtb(msg);
			planificador_actualizar_archivos_dtb(dtb_recibido);
			pcp_mover_dtb(dtb_recibido->gdt_id, "EXEC", "READY");
			sem_post(&sem_cont_cola_ready); // Aviso a PCP que hay uno nuevo en READY
			dtb_destroy(dtb_recibido);
		}
		else if(msg->header->emisor == CPU && msg->header->tipo_mensaje == EXIT){ // Me interesa este mensaje
			list_remove_and_destroy_element(cola_mensajes, 0, msg_free_v2);
			pthread_mutex_unlock(&sem_mutex_cola_mensajes);

			t_dtb* dtb_recibido;
			dtb_recibido = desempaquetar_dtb(msg);
			planificador_finalizar_dtb(dtb_recibido->gdt_id);
			dtb_destroy(dtb_recibido);
		}
		else{ // No me interesa el mensaje
			pthread_mutex_unlock(&sem_mutex_cola_mensajes);
			sem_post(&sem_cont_cola_mensajes); // Vuelvo a incrementar el contador de mensajes, porque no lo use
		}
		msg_free(&msg);

	} // Fin while(1)
}

void planificador_actualizar_archivos_dtb(t_dtb* dtb){
	t_dtb* dtb_a_modificar;

	bool _mismo_id(void* data){
		return  ((t_dtb*) data) -> gdt_id == dtb->gdt_id;
	}

	void _actualizar_archivos_abiertos(char* key, void* data){
		dictionary_put(dtb_a_modificar->archivos_abiertos, key, data);
	}

	if(dtb->flags.inicializado == 0){ // DUMMY
		return; // No tengo que hacer nada
	}
	else{ // No DUMMY
		// Busco al DTB en EXEC
		pthread_mutex_lock(&sem_mutex_cola_exec);
		if((dtb_a_modificar = list_find(cola_exec, _mismo_id)) != NULL){ // Encontre al DTB en EXEC
			dictionary_clean(dtb_a_modificar->archivos_abiertos); // Borro los archivos abiertos
			dictionary_iterator(dtb->archivos_abiertos, _actualizar_archivos_abiertos); // Los vuelvo a poner, uno por uno
		}
		pthread_mutex_unlock(&sem_mutex_cola_exec);
	}
}

t_dtb* planificador_encontrar_dtb_y_copiar(unsigned int id_target, char** estado_actual){
	t_dtb* dtb = NULL;

	bool _tiene_mismo_id_y_no_dummy(void* data){
		return  ((t_dtb*) data) -> gdt_id == id_target && ((t_dtb*) data) -> flags.inicializado == 1;
	}
	bool _es_dummy(void* data){
		return ((t_dtb*) data) -> flags.inicializado == 0;
	}

	bool (*buscador)(void*);
	if(id_target == 0) buscador = &_es_dummy; // Quiere buscar al dummy
	else buscador = &_tiene_mismo_id_y_no_dummy;

	pthread_mutex_lock(&sem_mutex_cola_new);
	pthread_mutex_lock(&sem_mutex_cola_ready);
	pthread_mutex_lock(&sem_mutex_cola_block);
	pthread_mutex_lock(&sem_mutex_cola_exec);
	if( (dtb = dtb_copiar( list_find(cola_new, buscador)) ) != NULL){
		*estado_actual = strdup("NEW");
	}
	else if( (dtb = dtb_copiar( list_find(cola_ready, buscador)) ) != NULL){
		*estado_actual = strdup("READY");
	}
	else if( (dtb = dtb_copiar( list_find(cola_block, buscador)) ) != NULL){
		*estado_actual = strdup("BLOCK");
	}
	else if( (dtb = dtb_copiar( list_find(cola_exec, buscador)) ) != NULL){
		*estado_actual = strdup("EXEC");
	}
	else{
		*estado_actual= strdup("No encontrado");
	}
	pthread_mutex_unlock(&sem_mutex_cola_new);
	pthread_mutex_unlock(&sem_mutex_cola_ready);
	pthread_mutex_unlock(&sem_mutex_cola_block);
	pthread_mutex_unlock(&sem_mutex_cola_exec);

	return dtb;
}

bool planificador_finalizar_dtb(unsigned int id){
	char* estado;
	t_dtb* dtb = planificador_encontrar_dtb_y_copiar(id, &estado);
	dtb_destroy(dtb);

	if(!strcmp(estado,"NEW"))
		plp_mover_dtb(id, "EXIT");
	else if(!strcmp(estado,"READY") || !strcmp(estado,"BLOCK") || !strcmp(estado,"EXEC"))
		pcp_mover_dtb(id, estado, "EXIT");
	else{ // No encontrado
		free(estado);
		return false;
	}
	free(estado);
	return true;
}






























