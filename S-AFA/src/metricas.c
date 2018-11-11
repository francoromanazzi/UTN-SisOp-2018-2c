#include "metricas.h"

static double timespec_to_double(struct timespec time);

void metricas_initialize(){
	dict_tiempo_respuesta = dictionary_create();
	pthread_mutex_init(&sem_mutex_dict_tiempo_respuesta, NULL);

	cant_procesos_tiempo_promedio = 0;
	tiempo_promedio = -1;
	pthread_mutex_init(&sem_mutex_tiempo_promedio, NULL);
}

void metricas_tiempo_add_start(unsigned int id){
	char* key = string_itoa((int) id);

	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);

	double* tiempo_inicio = malloc(sizeof(double));
	*tiempo_inicio = timespec_to_double(time);

	pthread_mutex_lock(&sem_mutex_dict_tiempo_respuesta);
	dictionary_put(dict_tiempo_respuesta, key, (void*) tiempo_inicio);
	pthread_mutex_unlock(&sem_mutex_dict_tiempo_respuesta);

	free(key);
}

void metricas_tiempo_add_finish(unsigned int id, struct timespec time){
	char* key = string_itoa((int) id);
    double tiempo_respuesta_proceso_actual;  // Segundos

	pthread_mutex_lock(&sem_mutex_dict_tiempo_respuesta);
	double* tiempo_inicio = (double*) dictionary_remove(dict_tiempo_respuesta, key);
	pthread_mutex_unlock(&sem_mutex_dict_tiempo_respuesta);

	tiempo_respuesta_proceso_actual  = timespec_to_double(time) - *tiempo_inicio;
	log_info(logger, "{METRICAS} TIEMPO_RESPUESTA_ID_%d: %f", id, tiempo_respuesta_proceso_actual);

	pthread_mutex_lock(&sem_mutex_tiempo_promedio);
	tiempo_promedio = ((tiempo_promedio * cant_procesos_tiempo_promedio) + tiempo_respuesta_proceso_actual);
	tiempo_promedio /= ++cant_procesos_tiempo_promedio;
	pthread_mutex_unlock(&sem_mutex_tiempo_promedio);

	free(tiempo_inicio);
	free(key);
}

double metricas_tiempo_get_promedio(){
	double ret;
	pthread_mutex_lock(&sem_mutex_tiempo_promedio);
	ret = tiempo_promedio;
	pthread_mutex_unlock(&sem_mutex_tiempo_promedio);
	return ret;
}

static double timespec_to_double(struct timespec time){
	return ((double) time.tv_sec + (((double) time.tv_nsec) / 1000000000));
}


void metricas_nueva_sentencia_ejecutada(e_emisor modulo_destinatario){
	safa_protocol_encolar_msg_y_avisar(S_AFA, PLP, SENTENCIA_EJECUTADA_);
}










