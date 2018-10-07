#ifndef SHARED_PROTOCOL_H_
#define SHARED_PROTOCOL_H_
	#include "_common_includes.h"
#include "msg.h"
	
	t_msg* empaquetar_int(int);
	int desempaquetar_int(t_msg*);

	t_msg* empaquetar_string(char*);
	char* desempaquetar_string(t_msg*);

	t_msg* empaquetar_dtb(t_dtb*);
	t_dtb* desempaquetar_dtb(t_msg*);

	t_msg* empaquetar_resultado_abrir(int ok, unsigned int id, char* path, int base);
	void desempaquetar_resultado_abrir(t_msg*, int* ok, unsigned int* id, char** path, int* base);

	t_msg* empaquetar_abrir(char* path, unsigned int id);
	void desempaquetar_abrir(t_msg*, char** path, unsigned int* id);

	t_msg* empaquetar_get_fm9(unsigned int id, int base, int offset);
	void desempaquetar_get_fm9(t_msg*, unsigned int* id, int* base, int* offset);

	t_msg* empaquetar_resultado_get_fm9(int ok, char* datos);
	void desempaquetar_resultado_get_fm9(t_msg* msg, int* ok, char** datos);

	t_msg* empaquetar_tiempo_respuesta(unsigned int id, struct timespec ms);
	void desempaquetar_tiempo_respuesta(t_msg*, unsigned int* id, struct timespec* ms);

	t_msg* empaquetar_escribir_fm9(unsigned int id, int base, int offset, char* datos);
	void desempaquetar_escribir_fm9(t_msg*, unsigned int* id, int* base, int* offset, char** datos);

	t_msg* empaquetar_flush(unsigned int id, char* path, int base);
	void desempaquetar_flush(t_msg*, unsigned int* id, char** path, int* base);

	t_msg* empaquetar_crear(char* path, int cant_lineas);
	void desempaquetar_crear(t_msg*, char** path, int* cant_lineas);

	t_msg* empaquetar_get_mdj(char* path, int offset, int size);
	void desempaquetar_get_mdj(t_msg*, char** path, int* offset, int* size);

	t_msg* empaquetar_close(unsigned int id, int base);
	void desempaquetar_close(t_msg* msg, unsigned int* id, int* base);

#endif /* SHARED_PROTOCOL_H_ */
