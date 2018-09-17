#include "gestor.h"

void gestor_iniciar(){

	bool _es_dummy(void* data){
		return  ((t_dtb*) data) -> gdt_id == 0;
	}

	/* Creo el hilo consola del gestor de programas */
	if(pthread_create( &thread_gestor_consola, NULL, (void*) gestor_consola_iniciar, NULL) ){
		log_error(logger,"No pude crear el hilo para la consola del gestor");
			safa_exit();
		exit(EXIT_FAILURE);
	}
	log_info(logger, "Creo el hilo para la consola del gestor");
	pthread_detach(thread_gestor);

	t_msg* msg;
	while(1){ // Espero mensajes de SAFA
		sem_wait(&sem_cont_cola_mensajes);
		pthread_mutex_lock(&sem_mutex_cola_mensajes);
		msg = msg_duplicar(list_get(cola_mensajes, 0)); // Veo cual es el primer mensaje, y me lo copio (lo libero al final del while)

		if(msg->header->emisor == DAM && msg->header->tipo_mensaje == RESULTADO_ABRIR){ // Me interesa este mensaje
			list_remove_and_destroy_element(cola_mensajes, 0, msg_free_v2);
			pthread_mutex_unlock(&sem_mutex_cola_mensajes);

			/* Me comunico con los planificadores*/
			msg_resultado_abrir = msg;
			sem_post(&sem_cont_cargar_recurso);

			//planificador_cargar_archivo_en_dtb(msg);
		}
		else{ // No me interesa el mensaje
			pthread_mutex_unlock(&sem_mutex_cola_mensajes);
			sem_post(&sem_cont_cola_mensajes); // Vuelvo a incrementar el contador de mensajes, porque no lo use
		}
		msg_free(&msg);
	} // Fin while(1)
}


