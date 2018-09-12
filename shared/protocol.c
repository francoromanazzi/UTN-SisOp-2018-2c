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
		dictionary_len += sizeof(*data);
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
		memcpy(ret->payload + offset, (void*) &key_len, sizeof(unsigned int));
		offset += sizeof(unsigned int);
		memcpy(ret->payload + offset, (void*) key, key_len);
		offset += key_len;
		memcpy(ret->payload + offset, data, sizeof(*data));
		offset += sizeof(*data);
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

		key = malloc(key_len + 1);
		memcpy(key, msg->payload + offset, key_len);
		offset += key_len;
		key[key_len] = '\0';

		memcpy((void*) &data, msg->payload + offset, sizeof(int));
		offset += sizeof(int);

		dictionary_put(ret->archivos_abiertos, key, (void*) &data);
	}
	return ret;
}


t_msg* empaquetar_resultado_abrir(int ok, unsigned int id, char* path, int base){
	int offset = 0;
	t_msg* ret = malloc(sizeof(t_msg));
	ret->header = malloc(sizeof(t_header));
	ret->header->payload_size =
			sizeof(int) + // OK
			sizeof(unsigned int) + // id
			sizeof(unsigned int) + // path_len
			strlen(path) + // path
			sizeof(int) // base
			;
	ret->payload = malloc(ret->header->payload_size);

	memcpy(ret->payload, (void*) &ok, sizeof(int));
	offset += sizeof(int);

	memcpy(ret->payload + offset, (void*) &id, sizeof(unsigned int));
	offset += sizeof(unsigned int);

	int path_len = strlen(path);
	memcpy(ret->payload + offset, (void*) &path_len, sizeof(unsigned int));
	offset += sizeof(unsigned int);

	memcpy(ret->payload + offset, (void*) path, path_len);
	offset += path_len;

	memcpy(ret->payload + offset, (void*) &base, sizeof(int));

	return ret;
}

void desempaquetar_resultado_abrir(t_msg* msg, int* ok, unsigned int* id, char** path, int* base){
	int offset = 0;

	memcpy((void*) ok, msg->payload, sizeof(int));
	offset += sizeof(int);

	memcpy((void*) id, msg->payload + offset, sizeof(unsigned int));
	offset += sizeof(unsigned int);

	int path_len;
	memcpy((void*) &path_len, msg->payload + offset, sizeof(unsigned int));
	offset += sizeof(unsigned int);

	*path = malloc(path_len + 1);
	memcpy((void*) *path, msg->payload + offset, path_len);
	offset += path_len;
	(*path)[path_len] = '\0';

	memcpy((void*) base,msg->payload + offset, sizeof(int));
	return;
}


t_msg* empaquetar_abrir(char* path, unsigned int id){
	int offset = 0;
	t_msg* ret = malloc(sizeof(t_msg));
	ret->header = malloc(sizeof(t_header));
	ret->header->payload_size =
			sizeof(unsigned int) + //path_len
			strlen(path) + // path
			sizeof(unsigned int) // id
			;
	ret->payload = malloc(ret->header->payload_size);

	int path_len = strlen(path);
	memcpy(ret->payload, (void*) &path_len , sizeof(unsigned int));
	offset += sizeof(unsigned int);

	memcpy(ret->payload + offset, (void*) path, path_len);
	offset += path_len;

	memcpy(ret->payload + offset, (void*) &id, sizeof(unsigned int));
	return ret;
}

void desempaquetar_abrir(t_msg* msg, char** path, unsigned int* id){
	int offset = 0;
	int path_len;

	memcpy((void*) &path_len, msg->payload, sizeof(unsigned int));
	offset += sizeof(unsigned int);

	*path = malloc(path_len + 1);
	memcpy((void*) *path, msg->payload + offset, path_len);
	offset += path_len;
	(*path)[path_len] = '\0';

	memcpy((void*) id, msg->payload + offset, sizeof(unsigned int));
	return;
}









