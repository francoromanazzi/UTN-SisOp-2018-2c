#ifndef SHARED_PROTOCOL_H_
#define SHARED_PROTOCOL_H_
	#include <commons/collections/list.h>
	#include "_common_includes.h"
	#include "msg.h"
	

	t_msg* empaquetar_int(int);
	int desempaquetar_int(t_msg*);

	t_msg* empaquetar_string(char*);
	char* desempaquetar_string(t_msg*);

	t_msg* empaquetar_void_ptr(void* data, int data_size);
	void desempaquetar_void_ptr(t_msg*, void** data, int* data_size);

	t_msg* empaquetar_dtb(t_dtb*);
	t_dtb* desempaquetar_dtb(t_msg*);

	t_msg* empaquetar_resultado_abrir(int ok, unsigned int id, char* path, t_list* lista_direcciones);
	void desempaquetar_resultado_abrir(t_msg*, int* ok, unsigned int* id, char** path, t_list** lista_direcciones);

	t_msg* empaquetar_abrir(char* path, unsigned int id);
	void desempaquetar_abrir(t_msg*, char** path, unsigned int* id);

	t_msg* empaquetar_get_fm9(unsigned int id, int direccion_logica);
	void desempaquetar_get_fm9(t_msg*, unsigned int* id, int* direccion_logica);

	t_msg* empaquetar_resultado_get_fm9(int ok, char* datos);
	void desempaquetar_resultado_get_fm9(t_msg* msg, int* ok, char** datos);

	t_msg* empaquetar_tiempo_respuesta(unsigned int id, struct timespec ms);
	void desempaquetar_tiempo_respuesta(t_msg*, unsigned int* id, struct timespec* ms);

	t_msg* empaquetar_reservar_linea_fm9(unsigned int id, int direccion_logica);
	void desempaquetar_reservar_linea_fm9(t_msg*, unsigned int* id, int* direccion_logica);

	t_msg* empaquetar_resultado_reservar_linea_fm9(int ok, int direccion_logica);
	void desempaquetar_resultado_reservar_linea_fm9(t_msg*, int* ok, int* direccion_logica);

	t_msg* empaquetar_resultado_crear_fm9(int ok, int direccion_logica);
	void desempaquetar_resultado_crear_fm9(t_msg*, int* ok, int* direccion_logica);

	t_msg* empaquetar_escribir_fm9(unsigned int id, int direccion_logica, char* datos);
	void desempaquetar_escribir_fm9(t_msg*, unsigned int* id, int* direccion_logica, char** datos);

	t_msg* empaquetar_flush(unsigned int id, char* path, t_list* lista_direcciones);
	void desempaquetar_flush(t_msg*, unsigned int* id, char** path, t_list** lista_direcciones);

	t_msg* empaquetar_crear_mdj(unsigned int id, char* path, int cant_lineas);
	void desempaquetar_crear_mdj(t_msg*, unsigned int* id, char** path, int* cant_lineas);

	t_msg* empaquetar_get_mdj(char* path, int offset, int size);
	void desempaquetar_get_mdj(t_msg*, char** path, int* offset, int* size);

	t_msg* empaquetar_escribir_mdj(char* path, int offset, int size, void* buffer);
	void desempaquetar_escribir_mdj(t_msg*, char** path, int* offset, int* size, void** buffer);

	t_msg* empaquetar_borrar(unsigned int id, char* path);
	void desempaquetar_borrar(t_msg*, unsigned int* id, char** path);

	t_msg* empaquetar_resultado_io(int ok, unsigned int id);
	void desempaquetar_resultado_io(t_msg*, int* ok, unsigned int* id);

	t_msg* empaquetar_close(unsigned int id, t_list* lista_direcciones);
	void desempaquetar_close(t_msg* msg, unsigned int* id, t_list** lista_direcciones);

#endif /* SHARED_PROTOCOL_H_ */
