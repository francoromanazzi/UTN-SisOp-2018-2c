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
	#include <shared/msg.h>
	#include <shared/protocol.h>
	#include "/home/utnso/workspace/tp-2018-2c-RegorDTs/S-AFA/src/DTB.h"

	t_config* config;
	t_log* logger;
	int safa_socket;
	int dam_socket;

	int quantum;

	void config_create_fixed(char* path);
	void cpu_iniciar();
	void cpu_ejecutar_dtb(t_dtb* dtb);
	int cpu_connect_to_safa();
	int cpu_connect_to_dam();
	void cpu_exit();

#endif /* CPU_H_ */
