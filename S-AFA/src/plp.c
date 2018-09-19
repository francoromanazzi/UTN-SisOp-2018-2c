#include "plp.h"

void plp_iniciar(){

	bool _flag_inicializado_en_uno(void* dtb){
		return ((t_dtb*) dtb)->flags.inicializado == 1;
	}

	/* Creo el hilo que crea los nuevos DTBs */
	if(pthread_create( &thread_plp_crear_dtb, NULL, (void*) plp_crear_dtb_iniciar, NULL) ){
		log_error(logger,"No pude crear el hilo PLP_crear_DTB");
		exit(EXIT_FAILURE);
	}
	log_info(logger, "Creo el hilo PLP_crear_DTB");
	pthread_detach(thread_plp_crear_dtb);

	/* Creo el hilo que carga la base del escriptorio y mueve a READY */
	if(pthread_create( &thread_plp_cargar_recurso, NULL, (void*) plp_cargar_recurso_iniciar, NULL) ){
		log_error(logger,"No pude crear el hilo PLP_cargar_recurso");
		exit(EXIT_FAILURE);
	}
	log_info(logger, "Creo el hilo PLP_cargar_recurso");
	pthread_detach(thread_plp_cargar_recurso);

	/* Tengo que comparar constantemente la cantidad de procesos y el grado de multiprogramacion */
	while(1){
		sem_wait(&sem_cont_puedo_iniciar_op_dummy); // Espero a que pueda iniciar una op dummy (no puede haber mas de 1 en simultaneo)
		sem_wait(&sem_cont_cola_new); // Espero a que haya algun DTB en NEW
		sem_wait(&sem_cont_procesos); // Espero a que el grado de multiprogramacion me permita iniciar una op dummy

		/* OK, ya puedo avisar a PCP que desbloquee el DUMMY */
		pthread_mutex_lock(&sem_mutex_cola_new);
		t_dtb* dtb_a_pasar_a_ready = (t_dtb*) list_get(cola_new, 0);
		pthread_mutex_unlock(&sem_mutex_cola_new);

		id_nuevo_dtb = dtb_a_pasar_a_ready->gdt_id;

		pthread_mutex_lock(&sem_mutex_ruta_escriptorio_nuevo_dtb);
		//free_memory((void**) &ruta_escriptorio_nuevo_dtb);
		ruta_escriptorio_nuevo_dtb = dtb_a_pasar_a_ready->ruta_escriptorio;
		pthread_mutex_unlock(&sem_mutex_ruta_escriptorio_nuevo_dtb);

		sem_post(&sem_bin_desbloquear_dummy); // Le aviso a thread_pcp_desbloquear_dummy

	} // Fin while(1)
}

void plp_mover_dtb(unsigned int id, char* cola_destino){
	bool _tiene_mismo_id(void* data){
		return  ((t_dtb*) data) -> gdt_id == id;
	}

	pthread_mutex_lock(&sem_mutex_cola_new);
	t_dtb* dtb = list_remove_by_condition(cola_new, _tiene_mismo_id);
	pthread_mutex_unlock(&sem_mutex_cola_new);

	if(!strcmp(cola_destino, "EXIT")){ // NEW -> EXIT
		log_info(logger, "Finalizo el DTB con ID: %d", id);

		pthread_mutex_lock(&sem_mutex_cola_exit);
		list_add(cola_exit, dtb);
		pthread_mutex_unlock(&sem_mutex_cola_exit);

		pthread_mutex_lock(&sem_mutex_cant_procesos);
		cant_procesos--;
		pthread_mutex_unlock(&sem_mutex_cant_procesos);

		sem_post(&sem_cont_procesos);
	}
	else if(!strcmp(cola_destino, "READY")){ // NEW -> READY
		log_info(logger, "Muevo a READY el DTB con ID: %d", id);

		pthread_mutex_lock(&sem_mutex_cola_ready);
		list_add(cola_ready, dtb);
		pthread_mutex_unlock(&sem_mutex_cola_ready);

		pthread_mutex_lock(&sem_mutex_cant_procesos);
		cant_procesos++;
		pthread_mutex_unlock(&sem_mutex_cant_procesos);

		sem_post(&sem_cont_cola_ready);
	}
}
