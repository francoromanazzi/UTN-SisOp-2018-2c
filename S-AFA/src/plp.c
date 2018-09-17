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
		//usleep(retardo_planificacion); // No es necesario en PLP

		sem_wait(&sem_cont_puedo_iniciar_op_dummy); // Espero a que pueda iniciar una op dummy (no puede haber mas de 1 en simultaneo)
		sem_wait(&sem_cont_cola_new); // Espero a que haya algun DTB en NEW
		sem_wait(&sem_cont_procesos); // Espero a que el grado de multiprogramacion me permita iniciar una op dummy

		/* OK, ya puedo avisar a PCP que desbloquee el DUMMY */
		pthread_mutex_lock(&sem_mutex_cola_new);
		t_dtb* dtb_a_pasar_a_ready = (t_dtb*) list_get(cola_new, 0);
		pthread_mutex_unlock(&sem_mutex_cola_new);

		id_nuevo_dtb = dtb_a_pasar_a_ready->gdt_id;
		ruta_escriptorio_nuevo_dtb = dtb_a_pasar_a_ready->ruta_escriptorio;

		sem_post(&sem_bin_desbloquear_dummy); // Le aviso a thread_pcp_desbloquear_dummy

		/* Veo si alguno de esos DTB tiene el flag de inicializado en 1, a.k.a ya puede pasar a READY*/
		//t_dtb* dtb_que_puede_pasar_a_ready;
		//pthread_mutex_lock(&sem_mutex_cola_new);
		//if((dtb_que_puede_pasar_a_ready = list_find(cola_new, _flag_inicializado_en_uno)) != NULL){
		//	pthread_mutex_unlock(&sem_mutex_cola_new);
		//	plp_mover_dtb(dtb_que_puede_pasar_a_ready->gdt_id, "READY");
		//	continue;
		//}
		//pthread_mutex_unlock(&sem_mutex_cola_new);

		//sem_post(&sem_cont_cola_new); // Como no habia ninguno listo para pasar a READY, dejo como estaba la cola de NEW

		/* Espero a que el gestor termine de ejecutar la op dummy, si es que lo estaba haciendo*/
		//sem_wait(&sem_bin_op_dummy_1);

		//ruta_escriptorio_nuevo_dtb = ((t_dtb*) list_get(cola_new, 0)) -> ruta_escriptorio;

		/* Le aviso a gestor que ya puede iniciar otra operacion dummy */
		//sem_post(&sem_bin_op_dummy_0);
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
		cant_procesos--;
		sem_post(&sem_cont_procesos);
		list_add(cola_exit, dtb);
	}
	else if(!strcmp(cola_destino, "READY")){ // NEW -> READY
		log_info(logger, "Muevo a READY el DTB con ID: %d", id);
		cant_procesos++;
		pthread_mutex_lock(&sem_mutex_cola_ready);
		list_add(cola_ready, dtb);
		pthread_mutex_unlock(&sem_mutex_cola_ready);
		sem_post(&sem_cont_cola_ready);
	}
}
