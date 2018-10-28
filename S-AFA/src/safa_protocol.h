#ifndef SAFA_PROTOCOL_H_
#define SAFA_PROTOCOL_H_
	#include <stdlib.h>
	#include <string.h>
	#include <stdarg.h>
	#include <pthread.h>
	#include <semaphore.h>

	#include <commons/collections/list.h>

	#include <shared/DTB.h>
	#include <shared/msg.h>
	#include <shared/protocol.h>

	typedef enum {S_AFA, CONSOLA, PLP, PCP} e_safa_modulo;

	typedef enum {
		READY_DTB, // SAFA -> PCP
		READY_DTB_ID, // SAFA -> PCP
		BLOCK_DTB, // SAFA -> PCP
		BLOCK_DTB_ID, // SAFA -> PCP
		EXIT_DTB, // SAFA -> PCP
		EXIT_DTB_CONSOLA, // CONSOLA -> PLP || PLP -> PCP
		CREAR_DTB, // CONSOLA -> PLP
		NUEVO_CPU_DISPONIBLE, // SAFA -> PCP || PCP -> PCP
		CPU_DESCONECTADO, // SAFA -> PCP
		GRADO_MULTIPROGRAMACION_AUMENTADO, // SAFA -> PLP
		DESBLOQUEAR_DUMMY, // PLP -> PCP
		FIN_OP_DUMMY, // PLP -> PLP
		NUEVO_DTB_EN_READY, // (PLP || PCP) -> PCP
		RESULTADO_ABRIR_DAM, // SAFA -> PLP || PLP -> PCP
		RESULTADO_IO_DAM, // SAFA -> PCP
		PROCESO_FINALIZADO, // (PLP || PCP) -> PLP
		STATUS, // CONSOLA -> PLP || PLP -> PCP
		STATUS_PCB, // CONSOLA -> PLP || PLP -> PCP
		DUMMY_DISPONIBLE // PCP -> PLP
	} e_safa_tipo_msg;

	typedef struct {
		e_safa_modulo emisor;
		e_safa_modulo receptor;
		e_safa_tipo_msg tipo_msg;
		void* data;
	} t_safa_msg;

	typedef struct{
		int cant_procesos_activos;
		t_list* new;
		t_list* ready;
		t_list* exec;
		t_list* block;
		t_list* exit;
	}t_status; // Para el comando STATUS en la consola


	t_list* cola_msg_plp;
	sem_t sem_cont_cola_msg_plp;
	pthread_mutex_t sem_mutex_cola_msg_plp;

	t_list* cola_msg_pcp;
	sem_t sem_cont_cola_msg_pcp;
	pthread_mutex_t sem_mutex_cola_msg_pcp;

	t_list* cola_msg_consola;
	sem_t sem_cont_cola_msg_consola;
	pthread_mutex_t sem_mutex_cola_msg_consola;


	void safa_protocol_initialize();
	void safa_protocol_encolar_msg_y_avisar(e_safa_modulo emisor, e_safa_modulo receptor, e_safa_tipo_msg tipo_msg, ...);
	t_safa_msg* safa_protocol_desencolar_msg(e_safa_modulo receptor);
	void safa_protocol_msg_free(t_safa_msg* msg);

	void* safa_protocol_empaquetar_desbloquear_dummy(unsigned int id, char* path);
	void safa_protocol_desempaquetar_desbloquear_dummy(void* data, unsigned int* id, char** path);

	void* safa_protocol_empaquetar_resultado_abrir(void* data);
	void safa_protocol_desempaquetar_resultado_abrir(void* data, int* ok, unsigned int* id, char** path, int* base);

	void* safa_protocol_empaquetar_resultado_io(void* data);
	void safa_protocol_desempaquetar_resultado_io(void* data, int* ok, unsigned int* id);

#endif /* SAFA_PROTOCOL_H_ */
