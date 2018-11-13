#ifndef METRICAS_H_
#define METRICAS_H_
	#include <time.h>
	#include <stdlib.h>
	#include <commons/collections/dictionary.h>
	#include <commons/string.h>
	#include <pthread.h>
	#include <inttypes.h>
	#include <math.h>
	#include "safa_util.h"

	#define _POSIX_C_SOURCE 200809L

	t_dictionary* dict_tiempo_respuesta;
	pthread_mutex_t sem_mutex_dict_tiempo_respuesta;
	double tiempo_promedio;
	unsigned int cant_procesos_tiempo_promedio;
	pthread_mutex_t sem_mutex_tiempo_promedio;

	t_dictionary* dict_sentencias_ejecutadas;

	void metricas_initialize();

	double metricas_tiempo_get_promedio();
	void metricas_tiempo_add_start(unsigned int id);
	void metricas_tiempo_add_finish(unsigned int id, struct timespec time);

	void metricas_nueva_sentencia_ejecutada(e_emisor modulo_destinatario);
	double metricas_porcentaje_sentencias_hacia_diego();

#endif /* METRICAS_H_ */
