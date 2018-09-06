#include "plp.h"

void plp_iniciar(){
	/* Tengo que comparar constantemente la cantidad de procesos y el grado de multiprogramacion */

}



void plp_crear_dtb_y_encolar(char* path){
	t_dtb* nuevo_dtb = dtb_create(path);
	list_add(cola_new, nuevo_dtb);
	log_info(logger, "Creo el DTB con ID: %d del escriptorio: %s", nuevo_dtb->gdt_id, path);
}

void plp_desencolar(unsigned int id){
	bool _tiene_mismo_id(void* data){
		return  ((t_dtb*) data) -> gdt_id == id;
	}
	log_info(logger, "Finalizo el DTB con ID: %d", id);
	list_remove_and_destroy_by_condition(cola_new, _tiene_mismo_id, (void*) dtb_destroy);
}
