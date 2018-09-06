#include "DTB.h"

t_dtb* dtb_create(char* path_escriptorio){
	t_dtb* ret = malloc(sizeof(t_dtb));
	ret -> gdt_id = dtb_get_gdt_id_count();
	ret -> ruta_escriptorio = strdup(path_escriptorio);
	ret -> pc = 0;
	ret -> flags.inicializado = 0;
	return ret;
}

int dtb_get_gdt_id_count(){
	static int count = 1;
	return count++;
}

void dtb_destroy(t_dtb* dtb){
	if(dtb == NULL) return;

	free(dtb->ruta_escriptorio);
	free(dtb);
}

void dtb_mostrar(t_dtb* dtb, char* estado_actual){
	printf("ID: %d\n",dtb->gdt_id);
	printf("Estado actual: %s\n", estado_actual);
	printf("Escriptorio: %s\n",dtb->ruta_escriptorio);
	printf("PC: %d\n",dtb->pc);
	printf("Inicializado: %d\n",dtb->flags.inicializado);
	printf("\n");
}

t_dtb* dtb_copiar(t_dtb* otro_dtb){
	if(otro_dtb == NULL) return NULL;

	t_dtb* ret_dtb = malloc(sizeof(t_dtb));
	ret_dtb -> gdt_id = otro_dtb -> gdt_id;
	ret_dtb -> ruta_escriptorio = strdup(otro_dtb->ruta_escriptorio);
	ret_dtb -> pc = otro_dtb -> pc;
	ret_dtb -> flags.inicializado = otro_dtb -> flags.inicializado;
	return ret_dtb;
}

