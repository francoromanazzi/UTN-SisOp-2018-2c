#include "planificador.h"

void planificador_iniciar(){
	cant_procesos = 0;
	cola_new = list_create();
	cola_ready = list_create();
	pthread_mutex_init(&mutex_cola_ready, NULL);
	cola_block = list_create();
	list_add(cola_block, dtb_create_dummy());
	cola_exec = list_create();
	cola_exit = list_create();

	rutas_escriptorios_dtb_dummy = list_create();

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

	pthread_join(thread_pcp, NULL);
	pthread_join(thread_plp, NULL);
}

void planificador_crear_dtb_y_encolar(char* path){
	plp_crear_dtb_y_encolar(path);
}

t_dtb* planificador_encontrar_dtb(unsigned int id_target, char** estado_actual){
	t_dtb* dtb = NULL;

	bool _tiene_mismo_id(void* data){
		return  ((t_dtb*) data) -> gdt_id == id_target;
	}

	if( (dtb = dtb_copiar( list_find(cola_new, _tiene_mismo_id)) ) != NULL)
		*estado_actual = strdup("NEW");
	else if( (dtb = dtb_copiar( list_find(cola_ready, _tiene_mismo_id)) ) != NULL)
		*estado_actual = strdup("READY");
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
	t_dtb* dtb = planificador_encontrar_dtb(id, &estado);
	dtb_destroy(dtb);

	if(!strcmp(estado,"NEW"))
		plp_mover_dtb(id, "EXIT");
	else if(!strcmp(estado,"READY") || !strcmp(estado,"BLOCK") || !strcmp(estado,"EXEC")){
		pcp_mover_dtb(id, estado, "EXIT");
	}
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

	desempaquetar_resultado_abrir(msg, &ok, &id, &path, &base);

	/* Busco que DTB queria este recurso */
	/* Busco en NEW (por si estaba esperando el resultado de la operacion dummy) y en BLOCK */

	if((dtb = list_find(cola_new, _mismo_id)) == NULL)
		if((dtb = list_find(cola_block, _mismo_id)) == NULL){
			log_error(logger, "No pude cargar el archivo %s en el DTB %d porque no lo encontre ni en NEW ni en BLOCK", path, id);
			return;
		}

	if(!ok){ // No se encontro el recurso, asi que lo aborto
		planificador_finalizar_dtb(dtb->gdt_id);
	}
	else{
		dictionary_remove_and_destroy(dtb->archivos_abiertos, path, free);
		dictionary_put(dtb->archivos_abiertos, path, &base);
	}
	if(dtb->flags.inicializado == 0){ // Si estaba en NEW, esperando que carguen el  escriptorio
		/* Finalizo la operacion DUMMY */
		dtb->flags.inicializado = 1;
		operacion_dummy_en_ejecucion = false;
	}

}






























