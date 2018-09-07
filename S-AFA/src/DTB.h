#ifndef DTB_H_
#define DTB_H_
	#include <string.h>
	#include <stdlib.h>
	#include <stdio.h>

	typedef struct dtb_flags {
		int inicializado;
	} t_dtb_flags;

	typedef struct dtb {
		unsigned int gdt_id;
		char* ruta_escriptorio;
		unsigned int pc;
		struct dtb_flags flags;
		/* TODO: Tabla de direcciones de archivos abiertos */
	} t_dtb ;


	int dtb_get_gdt_id_count();

	t_dtb* dtb_create(char* path_escriptorio);
	t_dtb* dtb_create_dummy();

	void dtb_mostrar(t_dtb* dtb, char* estado_actual);
	t_dtb* dtb_copiar(t_dtb* otro_dtb);
	void dtb_destroy(t_dtb* dtb);


#endif /* DTB_H_ */
