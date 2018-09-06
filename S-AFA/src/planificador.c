#include "planificador.h"

void planificador_iniciar(){
	cant_procesos = 0;
	cola_new = list_create();
	cola_ready = list_create();
	pthread_mutex_init(&mutex_cola_ready, NULL);
	cola_block = list_create();

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
	t_dtb* nuevo_dtb = dtb_create(path);
	list_add(cola_new, nuevo_dtb);
	log_info(logger, "Creo el DTB con ID: %d del escriptorio: %s", nuevo_dtb->gdt_id, path);
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
	// TODO: y si esta en ejecucion? si esta en exit?
	else
		*estado_actual= strdup("No encontrado");
	return dtb;
}
