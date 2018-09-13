#include "DTB.h"

t_dtb* dtb_create(char* path_escriptorio){
	t_dtb* ret = malloc(sizeof(t_dtb));
	ret -> gdt_id = dtb_get_gdt_id_count();
	ret -> ruta_escriptorio= strdup(path_escriptorio);
	ret -> pc = 0;
	ret -> flags.inicializado = 0;
	ret -> archivos_abiertos = dictionary_create();


	if(ret -> gdt_id != 0){ // Si no es el DUMMY
		dictionary_put(ret->archivos_abiertos,strdup(ret->ruta_escriptorio), (void*) -1);
	}

	return ret;
}

t_dtb* dtb_create_dummy(){
	return dtb_create("");
}

int dtb_get_gdt_id_count(){
	static int count = 0;
	return count++;
}

void dtb_destroy(t_dtb* dtb){
	if(dtb == NULL) return;

	dictionary_destroy(dtb->archivos_abiertos);
	free(dtb->ruta_escriptorio);
	free(dtb);
}

void dtb_mostrar(t_dtb* dtb, char* estado_actual){
	void dictionary_print_element(char* key, void* data){
		printf("\t%s: %d\n", key, (int) data);
	}

	if(dtb->gdt_id == 0)
		printf("ID: DUMMY (0)\n");
	else
		printf("ID: %d\n",dtb->gdt_id);
	printf("Estado actual: %s\n", estado_actual);
	printf("Escriptorio: %s\n",dtb->ruta_escriptorio);
	printf("PC: %d\n",dtb->pc);
	printf("Inicializado: %d\n",dtb->flags.inicializado);
	printf("Archivos abiertos:\n");
	dictionary_iterator(dtb->archivos_abiertos,(void*) dictionary_print_element);
	printf("\n");
}

t_dtb* dtb_copiar(t_dtb* otro_dtb){
	if(otro_dtb == NULL) return NULL;
	t_dtb* ret_dtb = malloc(sizeof(t_dtb));

	void dictionary_copy_element(char* key, void* data){
		dictionary_put(ret_dtb->archivos_abiertos, key, data);
	}

	ret_dtb -> gdt_id = otro_dtb -> gdt_id;
	ret_dtb -> ruta_escriptorio = strdup(otro_dtb->ruta_escriptorio);
	ret_dtb -> pc = otro_dtb -> pc;
	ret_dtb -> flags.inicializado = otro_dtb -> flags.inicializado;
	ret_dtb -> archivos_abiertos = dictionary_create();
	dictionary_iterator(otro_dtb->archivos_abiertos, (void*) dictionary_copy_element);
	return ret_dtb;
}

