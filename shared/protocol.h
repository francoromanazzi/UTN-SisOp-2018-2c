#ifndef SHARED_PROTOCOL_H_
#define SHARED_PROTOCOL_H_
	#include "DTB.h"
	#include "msg.h"

	#define OK 10
	#define ERROR_PATH_INEXISTENTE 10001
	#define ERROR_ESPACIO_INSUFICIENTE_FM9 10002

	#define ERROR_ARCHIVO_NO_ABIERTO = 20001
	#define ERROR_FALLO_SEGMENTO = 20002

	#define ERROR_ESPACIO_INSUFICIENTE_MDJ = 30003
	#define ERROR_ARCHIVO_NO_EXISTE_MDJ = 30003


	t_msg* empaquetar_string(char* str);
	char* desempaquetar_string(t_msg* msg);

	t_msg* empaquetar_dtb(t_dtb* dtb);
	t_dtb* desempaquetar_dtb(t_msg* msg);

	t_msg* empaquetar_resultado_abrir(int ok, unsigned int id, char* path, int base);
	void desempaquetar_resultado_abrir(t_msg* msg, int* ok, unsigned int* id, char** path, int* base);

	t_msg* empaquetar_abrir(char* path, unsigned int id);
	void desempaquetar_abrir(t_msg* msg, char** path, unsigned int* id);

#endif /* SHARED_PROTOCOL_H_ */
