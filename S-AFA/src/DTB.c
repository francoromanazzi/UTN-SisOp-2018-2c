#include "DTB.h"

t_dtb* dtb_create(char* path_escriptorio){
	t_dtb* ret = malloc(sizeof(t_dtb));
	ret -> gdt_id = 0;
	strcpy(ret -> ruta_escriptorio, path_escriptorio);
	ret -> pc = 0;
	ret -> flags.inicializado = 0;
	return ret;
}
