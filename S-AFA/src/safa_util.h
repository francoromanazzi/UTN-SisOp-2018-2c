#ifndef SAFA_UTIL_H_
#define SAFA_UTIL_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <sys/inotify.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <pthread.h>
	#include <string.h>
	#include <errno.h>
    #include <poll.h>
	#include <unistd.h>

	#include <commons/config.h>
	#include <shared/util.h>
	#include "S-AFA.h"


	t_config* config;
	pthread_mutex_t sem_mutex_config; // Por ahi no es necesario, pero por las dudas...

	t_log* logger; // Lei en el foro que no necesita mutex

	void safa_util_initialize();

	void safa_config_create_fixed();
	int safa_config_get_int_value(char* key);
	char* safa_config_get_string_value(char* key);

	void status_free(t_status* status);
	t_status* status_copiar(t_status* otro_status);

#endif /* SAFA_UTIL_H_ */
