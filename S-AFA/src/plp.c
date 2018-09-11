#include "plp.h"

void plp_iniciar(){
	/* Tengo que comparar constantemente la cantidad de procesos y el grado de multiprogramacion */
	while(1){
		usleep(retardo_planificacion);
		if(cant_procesos > config_get_int_value(config, "MULTIPROGRAMACION"))
			break;
		if(!list_is_empty(cola_new)){
			/* Inicio operacion dummy */
			pcp_mover_dtb(0, "BLOCK", "READY"); // Desbloqueo dummy

			/* Creo el dummy que voy a enviarle al gestor, para que se lo mande a CPU */
			char* estado_dummy;
			t_dtb* dummy_a_enviar = planificador_encontrar_dtb(0, &estado_dummy); // TODO: liberar esta memoria

			/* Cargo en el dummy a enviar la ruta del escriptorio */
			free(dummy_a_enviar->ruta_escriptorio);
			dummy_a_enviar->ruta_escriptorio = strdup(((t_dtb*) list_get(cola_new, 0)) -> ruta_escriptorio);

			/* Le mando el dummy a gestor */
			gestor_iniciar_op_dummy(dummy_a_enviar);
			if(strcmp(estado_dummy,"READY")){ // El dummy no llego a ready, por alguna razon?
				free(estado_dummy);
				break;
			}
			free(estado_dummy);

			/* Espero a que el gestor me diga que CPU termino la operacion dummy, y lo bloqueo */

			sleep(2); // Despues sacarlo!!!!
			//pcp_mover_dtb(0, "READY", "EXEC"); // DESPUES SACAR Y DEJAR LA DE ARRIBA
			//pcp_mover_dtb(0, "EXEC", "BLOCK"); // DESPUES SACAR Y DEJAR LA DE ARRIBA

			/* Se tienen que cargar en memoria los archivos necesarios */


			/* Fin operacion dummy */
			t_dtb* dtb = dtb_copiar(list_get(cola_new, 0));
			void dictionary_copy_element(char* key, void* data){
				dictionary_put(dtb->archivos_abiertos, key, (int*) data);
			}
			dictionary_iterator(dummy_a_enviar->archivos_abiertos, dictionary_copy_element);
			plp_mover_dtb(dtb->gdt_id, "READY");
			dtb_destroy(dtb);
		}
	}
}

void plp_crear_dtb_y_encolar(char* path){
	t_dtb* nuevo_dtb = dtb_create(path);
	list_add(cola_new, nuevo_dtb);
	log_info(logger, "Creo el DTB con ID: %d del escriptorio: %s", nuevo_dtb->gdt_id, path);
}

void plp_mover_dtb(unsigned int id, char* cola_destino){
	bool _tiene_mismo_id(void* data){
		return  ((t_dtb*) data) -> gdt_id == id;
	}

	t_dtb* dtb = list_remove_by_condition(cola_new, _tiene_mismo_id);

	if(!strcmp(cola_destino, "EXIT")){
		log_info(logger, "Finalizo el DTB con ID: %d", id);
		list_add(cola_exit, dtb);
	}
	else if(!strcmp(cola_destino, "READY")){
		log_info(logger, "Muevo a READY el DTB con ID: %d", id);
		pthread_mutex_lock(&mutex_cola_ready);
		list_add(cola_ready, dtb);
		pthread_mutex_unlock(&mutex_cola_ready);
	}
}
