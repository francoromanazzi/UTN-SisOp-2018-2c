#ifndef CPU_H_
#define CPU_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <sys/stat.h>
 	#include <sys/types.h>
	#include <commons/log.h>
	#include <commons/config.h>
	#include <string.h>
	#include <stdbool.h>
	#include <pthread.h>
	#include <shared/socket.h>
	#include <shared/util.h>
	#include <shared/msg.h>

	t_config* config;
	t_log* logger;
	int safa_socket;
	int dam_socket;

	int quantum;

	void config_create_fixed(char* path);
	int cpu_connect_to_safa();
	void cpu_exit();

#endif /* CPU_H_ */
