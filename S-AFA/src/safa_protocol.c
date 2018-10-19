#include "safa_protocol.h"

void safa_protocol_initialize(){
	cola_msg_plp = list_create();
	pthread_mutex_init(&sem_mutex_cola_msg_plp, NULL);
	sem_init(&sem_cont_cola_msg_plp, 0, 0);

	cola_msg_pcp = list_create();
	pthread_mutex_init(&sem_mutex_cola_msg_pcp, NULL);
	sem_init(&sem_cont_cola_msg_pcp, 0, 0);

	cola_msg_consola = list_create();
	pthread_mutex_init(&sem_mutex_cola_msg_consola, NULL);
	sem_init(&sem_cont_cola_msg_consola, 0, 0);
}

void safa_protocol_encolar_msg_y_avisar(e_safa_modulo emisor, e_safa_modulo receptor, e_safa_tipo_msg tipo_msg, ...){
	t_safa_msg* msg = malloc(sizeof(t_safa_msg));
	msg->emisor = emisor;
	msg->receptor = receptor;
	msg->tipo_msg = tipo_msg;
	msg->data = NULL;

 	va_list arguments;
	va_start(arguments, tipo_msg);

	unsigned int id;
	int ok;
	char* path;
	t_dtb* dtb;
	void* data;
	t_status* status;

	switch(receptor){
		case PLP:
			switch(tipo_msg){
				case CREAR_DTB:
					path = va_arg(arguments, char*);
					msg->data = (void*) path;
				break;

				case RESULTADO_ABRIR_DAM:
					data = va_arg(arguments, void*);
					msg->data = data;
				break;

				case GRADO_MULTIPROGRAMACION_AUMENTADO:
				break;

				case DUMMY_DISPONIBLE:
				break;

				case PROCESO_FINALIZADO:
				break;

				case EXIT_DTB_CONSOLA:
					id = va_arg(arguments, unsigned int);
					msg->data = (void*) id;
				break;

				case FIN_OP_DUMMY:
				break;

				case STATUS:
				break;

				case STATUS_PCB:
					id = va_arg(arguments, unsigned int);
					msg->data = (void*) id;
				break;
			}
			pthread_mutex_lock(&sem_mutex_cola_msg_plp);
			list_add(cola_msg_plp, msg);
			pthread_mutex_unlock(&sem_mutex_cola_msg_plp);
			sem_post(&sem_cont_cola_msg_plp);
		break;

		case PCP:
			switch(tipo_msg){
				case NUEVO_CPU_DISPONIBLE:
				break;

				case CPU_DESCONECTADO:
				break;

				case BLOCK_DTB:
				case READY_DTB:
				case EXIT_DTB:
					dtb = va_arg(arguments, t_dtb*);
					msg->data = (void*) dtb;
				break;

				case EXIT_DTB_CONSOLA:
					id = va_arg(arguments, unsigned int);
					msg->data = (void*) id;
				break;

				case RESULTADO_ABRIR_DAM:
					data = va_arg(arguments, void*);
					msg->data = safa_protocol_empaquetar_resultado_abrir(data);
				break;

				case RESULTADO_IO_DAM:
					data = va_arg(arguments, void*);
					msg->data = data;
				break;

				case DESBLOQUEAR_DUMMY:
					id = va_arg(arguments, unsigned int);
					path = va_arg(arguments, char*);
					msg->data = safa_protocol_empaquetar_desbloquear_dummy(id, path);
				break;

				case STATUS:
					status = va_arg(arguments, t_status*);
					msg->data = (void*) status;
				break;

				case STATUS_PCB:
					id = va_arg(arguments, unsigned int);
					msg->data = (void*) id;
				break;
			}
			pthread_mutex_lock(&sem_mutex_cola_msg_pcp);
			list_add(cola_msg_pcp, msg);
			pthread_mutex_unlock(&sem_mutex_cola_msg_pcp);
			sem_post(&sem_cont_cola_msg_pcp);
		break;

		case CONSOLA:
			switch(tipo_msg){
				case STATUS:
					status = va_arg(arguments, t_status*);
					msg->data = (void*) status;
				break;

				case STATUS_PCB:
					dtb = va_arg(arguments, t_dtb*);
					msg->data = (void*) dtb;
				break;

				case EXIT_DTB_CONSOLA:
					id = va_arg(arguments, unsigned int);
					ok = va_arg(arguments, int);

					data = malloc(sizeof(unsigned int) + sizeof(int));
					memcpy(data, (void*) &id, sizeof(unsigned int));
					memcpy(data + sizeof(unsigned int), (void*) &ok, sizeof(int));
					msg->data = data;
				break;
			}
			pthread_mutex_lock(&sem_mutex_cola_msg_consola);
			list_add(cola_msg_consola, msg);
			pthread_mutex_unlock(&sem_mutex_cola_msg_consola);
			sem_post(&sem_cont_cola_msg_consola);
		break;
	}
	va_end(arguments);
}

t_safa_msg* safa_protocol_desencolar_msg(e_safa_modulo receptor){
	t_safa_msg* ret;
	switch(receptor){
		case PLP:
			pthread_mutex_lock(&sem_mutex_cola_msg_plp);
			ret = list_remove(cola_msg_plp, 0);
			pthread_mutex_unlock(&sem_mutex_cola_msg_plp);
		break;

		case PCP:
			pthread_mutex_lock(&sem_mutex_cola_msg_pcp);
			ret = list_remove(cola_msg_pcp, 0);
			pthread_mutex_unlock(&sem_mutex_cola_msg_pcp);
		break;

		case CONSOLA:
			pthread_mutex_lock(&sem_mutex_cola_msg_consola);
			ret = list_remove(cola_msg_consola, 0);
			pthread_mutex_unlock(&sem_mutex_cola_msg_consola);
		break;
	}
	return ret;
}

void safa_protocol_msg_free(t_safa_msg* msg){
	if(msg->data != NULL) free(msg->data);
	free(msg);
}

void* safa_protocol_empaquetar_desbloquear_dummy(unsigned int id, char* path){
	int path_len = strlen(path);
	void* ret = malloc(sizeof(unsigned int) + sizeof(int) + path_len);
	memcpy(ret, (void*) &id, sizeof(unsigned int));
	memcpy(ret + sizeof(unsigned int), (void*) &path_len, sizeof(int));
	memcpy(ret + sizeof(unsigned int) + sizeof(int), (void*) path, path_len);
	return ret;
}

void safa_protocol_desempaquetar_desbloquear_dummy(void* data, unsigned int* id, char** path){
	int path_len;
	memcpy((void*) id, data, sizeof(unsigned int));
	memcpy((void*) &path_len, data + sizeof(unsigned int), sizeof(int));
	*path = malloc(path_len + 1);
	memcpy((void*) *path, data + sizeof(unsigned int) + sizeof(int), path_len);
	(*path)[path_len] = '\0';
}

void* safa_protocol_empaquetar_resultado_abrir(void* data){
	int ok, base;
	unsigned int id;
	char* path;
	t_list* lista_direcciones;
	void* ret;

	safa_protocol_desempaquetar_resultado_abrir(data, &ok, &id, &path, &lista_direcciones);
	t_msg* msg = empaquetar_resultado_abrir(ok, id, path, lista_direcciones);
	ret = malloc(msg->header->payload_size);
	memcpy(ret, msg->payload, msg->header->payload_size);
	msg_free(&msg);
	free(path);
	list_destroy(lista_direcciones);
	return ret;
}

void safa_protocol_desempaquetar_resultado_abrir(void* data, int* ok, unsigned int* id, char** path, t_list** lista_direcciones){
	t_msg* msg = malloc(sizeof(t_msg));
	msg->payload = data;
	desempaquetar_resultado_abrir(msg, ok, id, path, lista_direcciones);
	free(msg);
}

void* safa_protocol_empaquetar_resultado_io(void* data){
	int ok;
	unsigned int id;
	void* ret;

	safa_protocol_desempaquetar_resultado_io(data, &ok, &id);
	t_msg* msg = empaquetar_resultado_io(ok, id);
	ret = malloc(msg->header->payload_size);
	memcpy(ret, msg->payload, msg->header->payload_size);
	msg_free(&msg);
	return ret;
}

void safa_protocol_desempaquetar_resultado_io(void* data, int* ok, unsigned int* id){
	t_msg* msg = malloc(sizeof(t_msg));
	msg->payload = data;
	desempaquetar_resultado_io(msg, ok, id);
	free(msg);
}





