#include "DTB.h"

t_dtb* dtb_create(char* path_escriptorio){
	t_dtb* ret = malloc(sizeof(t_dtb));
	ret -> gdt_id = dtb_get_gdt_id_count(true);
	ret -> pc = 0;
	ret -> quantum_restante = 0;
	ret -> archivos_abiertos = dictionary_create();
	ret -> flags.error_nro = OK;
	ret -> metricas.cant_sentencias_ejecutadas_en_new = 0;
	ret -> metricas.cant_sentencias_DAM = 0;
	ret -> metricas.cant_sentencias_totales = 0;

	if(path_escriptorio == NULL){ // DUMMY
		ret -> ruta_escriptorio = strdup("");
		ret -> estado_actual = ESTADO_BLOCK;
		ret -> flags.inicializado = 0;
	}
	else{ // NO DUMMY
		ret -> ruta_escriptorio = strdup(path_escriptorio);
		ret -> estado_actual = ESTADO_NEW;
		ret -> flags.inicializado = 1;
		dictionary_put(ret->archivos_abiertos, ret->ruta_escriptorio, (void*) -1);
	}

	return ret;
}

t_dtb* dtb_create_dummy(){
	return dtb_create((char*) NULL);
}

bool dtb_es_dummy(void* dtb){
	return ((t_dtb*) dtb)->flags.inicializado == 0;
}

bool dtb_le_queda_quantum(void* dtb){
	return ((t_dtb*) dtb)->quantum_restante > 0;
}

int dtb_get_gdt_id_count(bool modify){
	static int count = 0;
	if(modify) return count++;
	else return count;
}

void dtb_destroy(t_dtb* dtb){
	if(dtb == NULL) return;

	dictionary_destroy(dtb->archivos_abiertos);
	free(dtb->ruta_escriptorio);
	free(dtb);
}

void dtb_destroy_v2(void* dtb){
	dtb_destroy((t_dtb*) dtb);
}

void dtb_mostrar(t_dtb* dtb){

	void dictionary_print_element(char* key, void* data){
		printf("\t%s: %d\n", key, (int) data);
	}

	if(dtb->flags.inicializado == 0)
		printf("ID: DUMMY, ID_SOLICITANTE: %d \n",dtb->gdt_id);
	else
		printf("ID: %d\n",dtb->gdt_id);
	printf("Estado actual: %s\n", string_from_estado(dtb->estado_actual));
	printf("Escriptorio: %s\n",dtb->ruta_escriptorio);
	printf("Inicializado: %d\n",dtb->flags.inicializado);

	if(dtb->flags.error_nro != OK){
		printf("ERROR: %d\n",dtb->flags.error_nro);
	}

	if(dtb->flags.inicializado == 1){ // No dummy
		printf("PC: %d\n",dtb->pc);
		printf("Quantum restante: %d\n",dtb->quantum_restante);
		printf("Archivos abiertos:\n");
		dictionary_iterator(dtb->archivos_abiertos, dictionary_print_element);
	}

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
	ret_dtb -> quantum_restante = otro_dtb -> quantum_restante;
	ret_dtb -> estado_actual = otro_dtb -> estado_actual;
	ret_dtb -> flags.inicializado = otro_dtb -> flags.inicializado;
	ret_dtb -> flags.error_nro = otro_dtb -> flags.error_nro;
	ret_dtb -> archivos_abiertos = dictionary_create();
	dictionary_iterator(otro_dtb->archivos_abiertos, dictionary_copy_element);
	return ret_dtb;
}

void dtb_copiar_sobreescribir(void* dtb){
	t_dtb* nuevo_dtb = dtb_copiar((t_dtb*) dtb);
	dtb = (void*) nuevo_dtb;
}


