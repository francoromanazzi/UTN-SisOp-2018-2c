#include "plp_crear_dtb.h"

void plp_crear_dtb_iniciar(){

	while(1){
		/* Espero un inicio de operacion dummy */
		sem_wait(&sem_bin_crear_dtb_0);

		/* Agrego a NEW un DTB nuevo */
		pthread_mutex_lock(&sem_mutex_ruta_escriptorio_nuevo_dtb);
		plp_crear_dtb_encolar_new(ruta_escriptorio_nuevo_dtb);
		free(ruta_escriptorio_nuevo_dtb);
		pthread_mutex_unlock(&sem_mutex_ruta_escriptorio_nuevo_dtb);

		sem_post(&sem_bin_crear_dtb_1); // Aviso que ya termine de crear el DTB
	}
}

void plp_crear_dtb_encolar_new(char* path){
	t_dtb* nuevo_dtb = dtb_create(path);
	pthread_mutex_lock(&sem_mutex_cola_new);
	list_add(cola_new, nuevo_dtb);
	pthread_mutex_unlock(&sem_mutex_cola_new);
	sem_post(&sem_cont_cola_new);

	log_info(logger, "Creo el DTB con ID: %d del escriptorio: %s", nuevo_dtb->gdt_id, path);
}
