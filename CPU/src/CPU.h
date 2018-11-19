#ifndef CPU_H_
#define CPU_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <stdarg.h>
	#include <errno.h>
	#include <sys/stat.h>
 	#include <sys/types.h>
	#include <string.h>
	#include <stdbool.h>
	#include <pthread.h>

	#include <commons/log.h>
	#include <commons/process.h>
	#include <commons/config.h>
	#include <shared/socket.h>
	#include <shared/msg.h>
	#include <shared/protocol.h>
	#include <shared/DTB.h>
	#include <shared/util.h>

	#include "parser.h"
	#include "operacion.h"

	#define CONFIG_PATH "../configs/CPU.txt"
	#define LOG_DIRECTORY_PATH "../logs/"

	t_config* config;
	t_log* logger;

	pthread_t thread_interrupt_listenner; // Solo para algoritmos con desalojo
	bool interrupcion;
	pthread_mutex_t sem_mutex_interrupcion;

	int retardo_ejecucion;

	int safa_socket, safa_socket_interrupciones;
	int dam_socket;
	int fm9_socket;

	int cpu_initialize();
	int cpu_connect_to_safa();
	int cpu_connect_to_dam();
	int cpu_connect_to_fm9();

	int cpu_send(int socket, e_tipo_msg tipo_msg, ...);

	int cpu_esperar_orden_safa();
	void cpu_ejecutar_dtb(t_dtb* dtb);

	char* cpu_fetch(t_dtb* dtb, int base_escriptorio);
	t_operacion* cpu_decodificar(char* instruccion);
	int cpu_ejecutar_operacion(t_dtb* dtb, t_operacion* operacion);

	void cpu_interrupt_listenner_start();
	bool cpu_chequear_interrupciones(bool fifo, bool cancel_thread);

	void cpu_exit();

#endif /* CPU_H_ */
