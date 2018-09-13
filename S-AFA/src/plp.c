#include "plp.h"

void plp_iniciar(){

	bool _flag_inicializado_en_uno(void* dtb){
		return ((t_dtb*) dtb)->flags.inicializado == 1;
	}

	/* Tengo que comparar constantemente la cantidad de procesos y el grado de multiprogramacion */
	while(1){
		usleep(retardo_planificacion);
		if(cant_procesos >= config_get_int_value(config, "MULTIPROGRAMACION"))
			continue;
		if(!list_is_empty(cola_new)){
			/* Veo si alguno de esos DTB tiene el flag de inicializado en 1, a.k.a ya puede pasar a READY*/
			t_dtb* dtb_que_puede_pasar_a_ready;
			if((dtb_que_puede_pasar_a_ready = list_find(cola_new, _flag_inicializado_en_uno)) != NULL){
				plp_mover_dtb(dtb_que_puede_pasar_a_ready->gdt_id, "READY");
				continue;
			}

			/* Intento iniciar operacion DUMMY */
			if(operacion_dummy_en_ejecucion)
				continue;
			/* Inicio operacion dummy */
			operacion_dummy_en_ejecucion = true;
			pcp_mover_dtb(0, "BLOCK", "READY"); // Desbloqueo dummy

			/* Agrego una ruta del escriptorio a cargar en memoria, para que PCP se lo mande a CPU cuando seleccione al DUMMY */
			list_add(rutas_escriptorios_dtb_dummy, strdup( ((t_dtb*) list_get(cola_new, 0)) -> ruta_escriptorio) );
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
		cant_procesos++;
		//pthread_mutex_lock(&mutex_cola_ready);
		list_add(cola_ready, dtb);
		//pthread_mutex_unlock(&mutex_cola_ready);
	}
}
