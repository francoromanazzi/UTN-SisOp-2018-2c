#include "planificador.h"

void planificador_iniciar(){
	cant_procesos = 0;
	sem_init(&sem_cont_procesos, 0, config_get_int_value(config, "MULTIPROGRAMACION"));

	sem_init(&sem_bin_op_dummy_0, 0, 0);
	sem_init(&sem_bin_op_dummy_1, 0, 1);
	sem_init(&sem_bin_fin_op_dummy, 0, 0);

	cola_new = list_create();
	pthread_mutex_init(&sem_mutex_cola_new, NULL);
	sem_init(&sem_cont_cola_new, 0, 0);

	cola_ready = list_create();
	pthread_mutex_init(&sem_mutex_cola_ready, NULL);
	sem_init(&sem_cont_cola_ready, 0, 0);

	cola_block = list_create();
	list_add(cola_block, dtb_create_dummy());

	cola_exec = list_create();

	cola_exit = list_create();

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
			char* path = desempaquetar_string(msg);
			planificador_crear_dtb_y_encolar(path);
		}
		else if(msg->header->emisor == DAM && msg->header->tipo_mensaje == RESULTADO_ABRIR){ // Me interesa este mensaje
			list_remove_and_destroy_element(cola_mensajes, 0, msg_free_v2);
			pthread_mutex_unlock(&sem_mutex_cola_mensajes);
			planificador_cargar_archivo_en_dtb(msg);
		}
		else if(msg->header->emisor == CPU && msg->header->tipo_mensaje == BLOCK){ // Me interesa este mensaje
			list_remove_and_destroy_element(cola_mensajes, 0, msg_free_v2);
			pthread_mutex_unlock(&sem_mutex_cola_mensajes);
			t_dtb* dtb_recibido;
			dtb_recibido = desempaquetar_dtb(msg);
			planificador_cargar_nuevo_path_vacio_en_dtb(dtb_recibido);
			pcp_mover_dtb(dtb_recibido->gdt_id, "EXEC", "BLOCK");
			dtb_destroy(dtb_recibido);
		}
		else if(msg->header->emisor == CPU && msg->header->tipo_mensaje == READY){ // Me interesa este mensaje
			list_remove_and_destroy_element(cola_mensajes, 0, msg_free_v2);
			pthread_mutex_unlock(&sem_mutex_cola_mensajes);
			t_dtb* dtb_recibido;
			dtb_recibido = desempaquetar_dtb(msg);
			planificador_cargar_nuevo_path_vacio_en_dtb(dtb_recibido);
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

void planificador_crear_dtb_y_encolar(char* path){
	t_dtb* nuevo_dtb = dtb_create(path);
	pthread_mutex_lock(&sem_mutex_cola_new);
	list_add(cola_new, nuevo_dtb);
	pthread_mutex_unlock(&sem_mutex_cola_new);
	sem_post(&sem_cont_cola_new);

	log_info(logger, "Creo el DTB con ID: %d del escriptorio: %s", nuevo_dtb->gdt_id, path);
}

t_dtb* planificador_encontrar_dtb_y_copiar(unsigned int id_target, char** estado_actual){
	t_dtb* dtb = NULL;

	bool _tiene_mismo_id(void* data){
		return  ((t_dtb*) data) -> gdt_id == id_target;
	}
	pthread_mutex_lock(&sem_mutex_cola_new);
	pthread_mutex_lock(&sem_mutex_cola_ready);
	if( (dtb = dtb_copiar( list_find(cola_new, _tiene_mismo_id)) ) != NULL){
		*estado_actual = strdup("NEW");
		pthread_mutex_unlock(&sem_mutex_cola_new);
	}
	else if( (dtb = dtb_copiar( list_find(cola_ready, _tiene_mismo_id)) ) != NULL){
		*estado_actual = strdup("READY");
		pthread_mutex_unlock(&sem_mutex_cola_ready);
	}
	else if( (dtb = dtb_copiar( list_find(cola_block, _tiene_mismo_id)) ) != NULL)
		*estado_actual = strdup("BLOCK");
	else if( (dtb = dtb_copiar( list_find(cola_exec, _tiene_mismo_id)) ) != NULL)
		*estado_actual = strdup("EXEC");
	else
		*estado_actual= strdup("No encontrado");
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

void planificador_cargar_nuevo_path_vacio_en_dtb(t_dtb* dtb){
	t_dtb* dtb_a_modificar;

	bool _mismo_id(void* data){
		return  ((t_dtb*) data) -> gdt_id == dtb->gdt_id;
	}

	void _agregar_nuevo_path(char* key, void* data){
		if(!dictionary_has_key(dtb_a_modificar->archivos_abiertos, key)){ // Si ya tenia a ese archivo en su DTB
			dictionary_put(dtb_a_modificar->archivos_abiertos, key, data);
		}
	}

	if(dtb->gdt_id == 0){ // DUMMY
		return; // No tengo que hacer nada
	}
	else{ // No DUMMY
		// Busco al DTB en BLOCK
		if((dtb_a_modificar = list_find(cola_block, _mismo_id)) != NULL){
			dictionary_iterator(dtb->archivos_abiertos, _agregar_nuevo_path);
		}
	}
}

void planificador_cargar_archivo_en_dtb(t_msg* msg){
	int ok;
	unsigned int id;
	char* path;
	int base;
	t_dtb* dtb; // DTB a actualizar

	bool _mismo_id(void* data){
		return ((t_dtb*) data)->gdt_id == id;
	}

	bool _queria_ese_recurso(void* data){
		// Un DTB solicitante del dummy queria ese recurso SII en su diccionario de archivos abiertos tenia como clave la ruta
		// de su escriptorio, y como valor, un -1, indicando que todavia no se habia abierto ese archivo
		//return (dictionary_has_key(((t_dtb*) data)->archivos_abiertos, path) && (*((int*) dictionary_get(((t_dtb*) data)->archivos_abiertos, path))) == -1);
		return !strcmp(((t_dtb*)data)->ruta_escriptorio, path);
	}

	desempaquetar_resultado_abrir(msg, &ok, &id, &path, &base);
	log_info(logger,"OK: %d, ID: %d, PATH: %s, BASE: %d",ok,id,path,base);

	/* Busco en BLOCK el DTB que queria este recurso (puede ser el DUMMY, eso lo manejo despues)*/

	if((dtb = list_find(cola_block, _mismo_id)) == NULL){
		log_error(logger, "No pude cargar el archivo %s en el DTB %d porque no lo encontre en BLOCK", path, id);
		free(path);
		return;
	}

	if(id == 0){ // El DUMMY era el solicitante, asi que busco el verdadero ID del que quiere pasar a READY
		dtb = list_find(cola_new, _queria_ese_recurso);

		/* Finalizo la operacion DUMMY */
		dtb->flags.inicializado = 1;
		sem_post(&sem_bin_fin_op_dummy);
	}

	if(!ok){ // No se encontro el recurso, asi que lo aborto
		planificador_finalizar_dtb(dtb->gdt_id);
		free(path);
	}
	else{
		//dictionary_remove_and_destroy(dtb->archivos_abiertos, path, free);
		dictionary_remove(dtb->archivos_abiertos, path);
		dictionary_put(dtb->archivos_abiertos, path, (void*) base);
		free(path);
	}
}






























