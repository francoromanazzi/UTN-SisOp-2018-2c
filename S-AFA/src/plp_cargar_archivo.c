#include "plp_cargar_archivo.h"

void plp_cargar_recurso_iniciar(){
	int ok;
	unsigned int id;
	char* path;
	int base;
	t_dtb* dtb_a_actualizar;

	bool _mismo_id(void* dtb){
		return ((t_dtb*) dtb)->gdt_id == id;
	}

	while(1){
		sem_wait(&sem_bin_plp_cargar_archivo);

		desempaquetar_resultado_abrir(msg_resultado_abrir, &ok, &id, &path, &base);

		pthread_mutex_lock(&sem_mutex_cola_new);
		t_dtb* dtb_a_actualizar = list_find(cola_new, _mismo_id);
		pthread_mutex_unlock(&sem_mutex_cola_new);

		if(dtb_a_actualizar == NULL){ // No encontre al DTB en NEW (es responsabilidad de pcp_cargar_recurso)
			free(path);
			sem_post(&sem_bin_pcp_cargar_archivo);
			continue;
		}

		/* Encontre al DTB en NEW. Me fijo si DAM pudo cargar el archivo, si es asi, le cargo la base al DTB */
		log_info(logger,"Soy PLP. OK: %d, ID: %d, PATH: %s, BASE: %d",ok,id,path,base);
		msg_free(&msg_resultado_abrir);

		if(!ok){ // No se encontro el recurso, asi que lo aborto
			planificador_finalizar_dtb(dtb_a_actualizar->gdt_id);
			free(path);
		}
		else{ // Toodo ok, asi que cargo la ruta del escriptorio y lo muevo a READY
			dictionary_remove(dtb_a_actualizar->archivos_abiertos, path);
			dictionary_put(dtb_a_actualizar->archivos_abiertos, path, (void*) base);

			// De paso, reseteo al dummy


			free(path);
			plp_mover_dtb(dtb_a_actualizar->gdt_id, "READY");
		}

		sem_post(&sem_cont_puedo_iniciar_op_dummy); // Finalizo la op dummy
	}
}
