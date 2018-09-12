#include "plp.h"

void plp_iniciar(){
	/* Tengo que comparar constantemente la cantidad de procesos y el grado de multiprogramacion */
	while(1){
		usleep(retardo_planificacion);
		if(operacion_dummy_en_ejecucion)
			break;
		if(cant_procesos > config_get_int_value(config, "MULTIPROGRAMACION"))
			break;
		if(!list_is_empty(cola_new)){
			/* Inicio operacion dummy */
			operacion_dummy_en_ejecucion = true;
			pcp_mover_dtb(0, "BLOCK", "READY"); // Desbloqueo dummy

			/* Agrego una ruta del escriptorio a cargar en memoria, para que PCP se lo mande a CPU cuando seleccione al DUMMY */
			list_add(rutas_escriptorios_dtb_dummy, strdup( ((t_dtb*) list_get(cola_new, 0)) -> ruta_escriptorio) );


			/* TODO: Me tienen que avisar el resultado de cargar en memoria el escriptorio (mandado por diego) */


			//while(1){

			//	usleep(retardo_planificacion);
			//}



			//sleep(2); // Despues sacarlo!!!!

			/* En este momento, el DUMMY tiene, dentro del diccionario, la referencia al escriptorio abierto*/
			/* Leo desde el DUMMY y lo copio en el DTB que estoy por mandar a READY */
			//char* estado_actual;
			//t_dtb* dummy_copia = planificador_encontrar_dtb(0, &estado_actual);
			//free(estado_actual);

			//t_dtb* dtb_mandar_ready = list_get(cola_new, 0);
			//void dictionary_copy_element(char* key, void* data){
			//	dictionary_put(dtb_mandar_ready->archivos_abiertos, key, (int*) data);
			//}
			//dictionary_iterator(dummy_copia->archivos_abiertos, dictionary_copy_element);
			//plp_mover_dtb(dtb_mandar_ready->gdt_id, "READY");
			//dtb_destroy(dummy_copia);

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
