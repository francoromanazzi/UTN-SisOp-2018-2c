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













