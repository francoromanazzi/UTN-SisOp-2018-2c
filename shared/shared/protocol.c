#include "protocol.h"

t_msg* empaquetar_dtb(t_dtb* dtb){
	t_msg* ret = malloc(sizeof(t_msg));
	ret->header = malloc(sizeof(t_header));

	int offset = 0;
	int dictionary_len = 0;
	int i;

	void incrementar_dictionary_len(char* key, void* data){
		dictionary_len += sizeof(unsigned int);
		dictionary_len += strlen(key);
		dictionary_len += sizeof(data);
	}

	for(i=0; i<dtb->archivos_abiertos->elements_amount; i++){
		dictionary_iterator(dtb->archivos_abiertos, incrementar_dictionary_len);
	}

	ret->header->payload_size =
			sizeof(unsigned int) + // GDT_ID
			sizeof(unsigned int) + // RUTA_ESCRIPTORIO_LEN
			strlen(dtb->ruta_escriptorio) + // RUTA_ESCRIPTORIO
			sizeof(unsigned int) + // PC
			sizeof(int) + // FLAG INICIALIZADO
			sizeof(int) + // ELEMENTS_AMOUNT
			dictionary_len // LONGITUD TOTAL DE LOS DATOS DEL DICCIONARIO DE ARCHIVOS ABIERTOS
			;

	ret->payload=malloc(ret->header->payload_size);

	memcpy(ret->payload, (void*) &(dtb->gdt_id), sizeof(unsigned int));
	offset += sizeof(unsigned int);

	int ruta_escriptorio_len = strlen(dtb->ruta_escriptorio);
	memcpy(ret->payload + offset, (void*) &ruta_escriptorio_len, sizeof(unsigned int));
	offset += sizeof(unsigned int);

	memcpy(ret->payload + offset, (void*) dtb->ruta_escriptorio, ruta_escriptorio_len);
	offset += ruta_escriptorio_len;

	memcpy(ret->payload + offset, (void*) &(dtb->pc), sizeof(unsigned int));
	offset += sizeof(unsigned int);

	memcpy(ret->payload + offset, (void*) &(dtb->flags.inicializado), sizeof(int));
	offset += sizeof(int);

	memcpy(ret->payload + offset, (void*) &(dtb->archivos_abiertos->elements_amount), sizeof(int));
	offset += sizeof(int);

	void copiar_memoria_elemento_diccionario(char* key, void* data){
		int key_len = strlen(key);
		memcpy(ret->payload + offset, (void*) &key, sizeof(unsigned int));
		offset += sizeof(unsigned int);
		memcpy(ret->payload + offset, (char*) key, key_len);
		offset += key_len;
		memcpy(ret->payload + offset, data, sizeof(int));
		offset += sizeof(int);
	}

	for(i=0;i<dtb->archivos_abiertos->elements_amount; i++)
		dictionary_iterator(dtb->archivos_abiertos, copiar_memoria_elemento_diccionario);

	return ret;
}


t_dtb* desempaquetar_dtb(t_msg* msg){
	t_dtb* ret = malloc(sizeof(t_dtb));
	ret->archivos_abiertos = dictionary_create();
	int offset = 0;

	memcpy((void*) &(ret->gdt_id), msg->payload, sizeof(unsigned int));
	offset += sizeof(unsigned int);

	int ruta_escriptorio_len;
	memcpy(&ruta_escriptorio_len, msg->payload + offset, sizeof(unsigned int));
	offset += sizeof(unsigned int);

	ret->ruta_escriptorio = malloc(ruta_escriptorio_len + 1);
	memcpy(ret->ruta_escriptorio, msg->payload + offset, ruta_escriptorio_len);
	offset += ruta_escriptorio_len;
	ret->ruta_escriptorio[ruta_escriptorio_len] = '\0';

	memcpy((void*) &(ret->pc), msg->payload + offset, sizeof(unsigned int));
	offset += sizeof(unsigned int);

	memcpy((void*) &(ret->flags.inicializado), msg->payload + offset, sizeof(int));
	offset += sizeof(int);

	int elementos_diccionario;
	int i;
	memcpy((void*) &elementos_diccionario, msg->payload + offset, sizeof(int));
	offset += sizeof(int);

	for(i = 0; i< elementos_diccionario; i++){
		char* key;
		int key_len;
		int data;

		memcpy((void*) &key_len, msg->payload + offset, sizeof(unsigned int));
		offset += sizeof(unsigned int);

		key = malloc(key_len);
		memcpy(key, msg->payload + offset, key_len);
		offset += key_len;
		key[key_len] = '\0';

		memcpy((void*) &data, msg->payload + offset, sizeof(int));
		offset += sizeof(int);

		dictionary_put(ret->archivos_abiertos, key, (void*) &data);
	}
	return ret;
}















