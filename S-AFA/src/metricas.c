#include "metricas.h"

static double timespec_to_double(struct timespec time);
static int metricas_cantidad_sentencias_ejecutadas_total();

void metricas_initialize(){
	dict_tiempo_respuesta = dictionary_create();
	pthread_mutex_init(&sem_mutex_dict_tiempo_respuesta, NULL);
	cant_procesos_tiempo_promedio = 0;
	tiempo_promedio = -1;
	pthread_mutex_init(&sem_mutex_tiempo_promedio, NULL);

	dict_sentencias_ejecutadas = dictionary_create();
	dictionary_put(dict_sentencias_ejecutadas, string_itoa(SAFA), 0);
	dictionary_put(dict_sentencias_ejecutadas, string_itoa(CPU), 0);
	dictionary_put(dict_sentencias_ejecutadas, string_itoa(DAM), 0);
	dictionary_put(dict_sentencias_ejecutadas, string_itoa(FM9), 0);
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

	char* key = string_itoa(modulo_destinatario);
	dictionary_put(dict_sentencias_ejecutadas, key, (void*) (1 + (int) dictionary_get(dict_sentencias_ejecutadas, key)));
	free(key);
}

static int metricas_cantidad_sentencias_ejecutadas_total(){
	int ret = 0;

	void _acumular_sentencias(char* key, void* data){
		ret += (int) data;
	}

	dictionary_iterator(dict_sentencias_ejecutadas, _acumular_sentencias);

	return ret;
}

double metricas_porcentaje_sentencias_hacia_diego(){
	int cant_sentencias_total = metricas_cantidad_sentencias_ejecutadas_total();
	char* dam_key = string_itoa(DAM);
	int cant_sentencias_diego = (int) dictionary_get(dict_sentencias_ejecutadas, dam_key);
	free(dam_key);

	return cant_sentencias_total > 0 ? cant_sentencias_diego/cant_sentencias_total : (double) 0;
}












