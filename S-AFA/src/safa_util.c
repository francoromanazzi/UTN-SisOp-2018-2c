#include "safa_util.h"

void safa_util_initialize(){
	pthread_mutex_init(&sem_mutex_config, NULL);

	safa_config_create_fixed();

	mkdir(LOG_DIRECTORY_PATH, 0777);
	logger = log_create(LOG_PATH, "S-AFA", false, LOG_LEVEL_TRACE);
}

void safa_config_create_fixed(){
	config = config_create(CONFIG_PATH);

	util_config_fix_comillas(&config, "ALGORITMO");
	char* retardo_microsegundos_str = string_itoa(1000 * config_get_int_value(config, "RETARDO_PLANIF"));
	config_set_value(config, "RETARDO_PLANIF", retardo_microsegundos_str); // Milisegundos a microsegundos
	free(retardo_microsegundos_str);
}

int safa_config_get_int_value(char* key){
	pthread_mutex_lock(&sem_mutex_config);
	int ret = config_get_int_value(config, key);
	pthread_mutex_unlock(&sem_mutex_config);
	return ret;
}

char* safa_config_get_string_value(char* key){
	pthread_mutex_lock(&sem_mutex_config);
	char* ret = config_get_string_value(config, key);
	pthread_mutex_unlock(&sem_mutex_config);
	return ret;
}

t_status* status_copiar(t_status* otro_status){
	t_status* ret = malloc(sizeof(t_status));

	void dtb_duplicar_y_agregar_a_ret_new(void* dtb_original){
		list_add(ret->new, (void*) dtb_copiar(((t_dtb*) dtb_original)));
	}
	void dtb_duplicar_y_agregar_a_ret_ready(void* dtb_original){
		list_add(ret->ready, (void*) dtb_copiar(((t_dtb*) dtb_original)));
	}
	void dtb_duplicar_y_agregar_a_ret_exec(void* dtb_original){
		list_add(ret->exec, (void*) dtb_copiar(((t_dtb*) dtb_original)));
	}
	void dtb_duplicar_y_agregar_a_ret_block(void* dtb_original){
		list_add(ret->block, (void*) dtb_copiar(((t_dtb*) dtb_original)));
	}
	void dtb_duplicar_y_agregar_a_ret_exit(void* dtb_original){
		list_add(ret->exit, (void*) dtb_copiar(((t_dtb*) dtb_original)));
	}


	ret->cant_procesos_activos = otro_status->cant_procesos_activos;

	ret->new = list_create();
	list_iterate(otro_status->new, dtb_duplicar_y_agregar_a_ret_new);

	ret->ready = list_create();
	list_iterate(otro_status->ready, dtb_duplicar_y_agregar_a_ret_ready);

	ret->exec = list_create();
	list_iterate(otro_status->exec, dtb_duplicar_y_agregar_a_ret_exec);

	ret->block = list_create();
	list_iterate(otro_status->block, dtb_duplicar_y_agregar_a_ret_block);

	ret->exit = list_create();
	list_iterate(otro_status->exit, dtb_duplicar_y_agregar_a_ret_exit);

	return ret;
}

void status_free(t_status* status){
	list_destroy_and_destroy_elements(status->new, dtb_destroy_v2);
	list_destroy_and_destroy_elements(status->block, dtb_destroy_v2);
	list_destroy_and_destroy_elements(status->ready, dtb_destroy_v2);
	list_destroy_and_destroy_elements(status->exec, dtb_destroy_v2);
	list_destroy_and_destroy_elements(status->exit, dtb_destroy_v2);
	free(status);
}



