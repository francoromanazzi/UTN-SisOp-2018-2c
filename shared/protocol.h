#ifndef SHARED_PROTOCOL_H_
#define SHARED_PROTOCOL_H_

	#include "DTB.h"
	#include "msg.h"


	t_msg* empaquetar_string(char* str);
	char* desempaquetar_string(t_msg* msg);

	t_msg* empaquetar_dtb(t_dtb* dtb);
	t_dtb* desempaquetar_dtb(t_msg* msg);

	t_msg* empaquetar_resultado_abrir(int ok, unsigned int id, char* path, int base);
	void desempaquetar_resultado_abrir(t_msg* msg, int* ok, unsigned int* id, char** path, int* base);

	t_msg* empaquetar_abrir(char* path, unsigned int id);
	void desempaquetar_abrir(t_msg* msg, char** path, unsigned int* id);

#endif /* SHARED_PROTOCOL_H_ */
