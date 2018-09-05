#ifndef DTB_H_
#define DTB_H_
	#include <string.h>

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


	t_dtb* dtb_create(char* path_escriptorio);


#endif /* DTB_H_ */
