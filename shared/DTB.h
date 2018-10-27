#ifndef SHARED_DTB_H_
#define SHARED_DTB_H_
	#include <string.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <commons/collections/dictionary.h>
	#include <commons/collections/list.h>
	#include <commons/string.h>

	#include "_common_includes.h"

	typedef enum {ESTADO_NEW, ESTADO_READY, ESTADO_EXEC, ESTADO_BLOCK, ESTADO_EXIT} e_estado;

	static inline char* string_from_estado(e_estado estado)
	{
	    static char* strings[] = { "NEW", "READY", "EXEC", "BLOCK", "EXIT"};
	    return strings[estado];
	}

	typedef struct dtb_flags {
		int inicializado;
		int error_nro;
	} t_dtb_flags;

	typedef struct dtb {
		unsigned int gdt_id;
		char* ruta_escriptorio;
		unsigned int pc;
		int quantum_restante;
		e_estado estado_actual;
		struct dtb_flags flags;
		t_dictionary* archivos_abiertos; // KEY->ruta, VALUE->base
	} t_dtb ;


	t_dtb* dtb_create(char* path_escriptorio);
	t_dtb* dtb_create_dummy();
	int dtb_get_gdt_id_count(bool modify);

	bool dtb_es_dummy(void* dtb);
	bool dtb_le_queda_quantum(void* dtb);

	void dtb_mostrar(t_dtb* dtb);
	t_dtb* dtb_copiar(t_dtb* otro_dtb);
	void dtb_copiar_sobreescribir(void* dtb);
	void dtb_destroy(t_dtb* dtb);
	void dtb_destroy_v2(void* dtb);

#endif /* SHARED_DTB_H_ */
