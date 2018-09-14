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
	#include <shared/DTB.h>

	t_config* config;
	t_log* logger;
	int safa_socket;
	int dam_socket;

	int quantum;

	typedef enum {OP_ABRIR, OP_CONCENTRAR, OP_ASIGNAR, OP_WAIT, OP_SIGNAL, OP_FLUSH, OP_CLOSE, OP_CREAR, OP_BORRAR} e_tipo_operacion;

	typedef struct{
		e_tipo_operacion tipo_operacion;
		t_dictionary* operandos;
	}t_operacion;



	void config_create_fixed(char* path);
	int cpu_connect_to_safa();
	int cpu_connect_to_dam();

	int cpu_send(int socket, e_tipo_msg tipo_msg, void* data);

	void cpu_esperar_dtb();
	void cpu_ejecutar_dtb(t_dtb* dtb);

	char* cpu_fetch(t_dtb* dtb, int base_escriptorio);
	t_operacion* cpu_decodificar(char* instruccion);
	int cpu_buscar_operandos(t_operacion* operacion);
	int cpu_ejecutar_operacion(t_operacion* operacion);

	void cpu_exit();

#endif /* CPU_H_ */
