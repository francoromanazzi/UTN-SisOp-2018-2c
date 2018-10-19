#include "protocol.h"

t_msg* empaquetar_int(int i){
	t_msg* ret = malloc(sizeof(t_msg));
	ret->header = malloc(sizeof(t_header));
	ret->header->payload_size = sizeof(int);
	ret->payload=malloc(ret->header->payload_size);

	memcpy(ret->payload, (void*) &i, sizeof(int));
	return ret;
}

int desempaquetar_int(t_msg* msg){
	int ret;
	memcpy((void*) &ret, msg->payload, sizeof(int));
	return ret;
}

t_msg* empaquetar_string(char* str){
	t_msg* ret = malloc(sizeof(t_msg));
	unsigned int str_len = strlen(str);

	ret->header = malloc(sizeof(t_header));
	ret->header->payload_size = sizeof(unsigned int) + str_len;
	ret->payload=malloc(ret->header->payload_size);

	memcpy(ret->payload , (void*) &str_len, sizeof(unsigned int));
	memcpy(ret->payload + sizeof(unsigned int), (void*) str, str_len);
	return ret;
}

char* desempaquetar_string(t_msg* msg){
	char* ret;
	int ret_len;

	memcpy((void*) &ret_len, msg->payload, sizeof(unsigned int));

	ret = malloc(ret_len + 1);
	memcpy(ret, msg->payload + sizeof(unsigned int), ret_len);
	ret[ret_len] = '\0';
	return ret;
}

t_msg* empaquetar_void_ptr(void* data, int data_size){
	t_msg* ret = malloc(sizeof(t_msg));
	ret->header = malloc(sizeof(t_header));
	ret->header->payload_size = sizeof(int) + data_size;
	ret->payload = malloc(ret->header->payload_size);

	memcpy(ret->payload, (void*) &data_size, sizeof(int));
	memcpy(ret->payload + sizeof(int), data, data_size);
	return ret;
}

void desempaquetar_void_ptr(t_msg* msg, void** ret, int* data_size){
	memcpy((void*) data_size, msg->payload, sizeof(int));
	*ret = malloc(*data_size);
	memcpy(*ret, msg->payload + sizeof(int), *data_size);
}

t_msg* empaquetar_dtb(t_dtb* dtb){
	t_msg* ret = malloc(sizeof(t_msg));
	ret->header = malloc(sizeof(t_header));

	int offset = 0;
	int dictionary_len = 0;
	int i;

	void incrementar_dictionary_len(char* key, void* data){

		void _incrementar_nueva_direccion(void* data){
			dictionary_len += sizeof(int); // direccion_logica
		}

		t_list* lista_direcciones = (t_list*) data;
		dictionary_len += sizeof(unsigned int); // key_len
		dictionary_len += strlen(key); // key
		dictionary_len += sizeof(int); // list_len
		list_iterate(lista_direcciones, _incrementar_nueva_direccion);
	}

	dictionary_iterator(dtb->archivos_abiertos, incrementar_dictionary_len);

	ret->header->payload_size =
			sizeof(unsigned int) + // GDT_ID
			sizeof(unsigned int) + // RUTA_ESCRIPTORIO_LEN
			strlen(dtb->ruta_escriptorio) + // RUTA_ESCRIPTORIO
			sizeof(unsigned int) + // PC
			sizeof(int) + // QUANTUM RESTANTE
			sizeof(e_estado) + // ESTADO ACTUAL
			sizeof(int) + // FLAG INICIALIZADO
			sizeof(int) + // ERRNO
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

	memcpy(ret->payload + offset, (void*) &(dtb->quantum_restante), sizeof(int));
	offset += sizeof(int);

	memcpy(ret->payload + offset, (void*) &(dtb->estado_actual), sizeof(e_estado));
	offset += sizeof(e_estado);

	memcpy(ret->payload + offset, (void*) &(dtb->flags.inicializado), sizeof(int));
	offset += sizeof(int);

	memcpy(ret->payload + offset, (void*) &(dtb->flags.error_nro), sizeof(int));
	offset += sizeof(int);

	memcpy(ret->payload + offset, (void*) &(dtb->archivos_abiertos->elements_amount), sizeof(int));
	offset += sizeof(int);

	void copiar_memoria_elemento_diccionario(char* key, void* data){

		void _copiar_memoria_direccion_logica(void* _direccion){
			int direccion = (int) _direccion;
			memcpy(ret->payload + offset, (void*) &direccion, sizeof(int));
			offset += sizeof(int);
		}

		t_list* lista_direcciones = (t_list*) data;
		int key_len = strlen(key);
		memcpy(ret->payload + offset, (void*) &key_len, sizeof(unsigned int));
		offset += sizeof(unsigned int);
		memcpy(ret->payload + offset, (void*) key, key_len);
		offset += key_len;
		memcpy(ret->payload + offset, (void*) &(lista_direcciones->elements_count), sizeof(int));
		offset += sizeof(int);
		list_iterate(lista_direcciones, _copiar_memoria_direccion_logica);
	}

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

	memcpy((void*) &(ret->quantum_restante), msg->payload + offset, sizeof(int));
	offset += sizeof(int);

	memcpy((void*) &(ret->estado_actual), msg->payload + offset, sizeof(e_estado));
	offset += sizeof(e_estado);

	memcpy((void*) &(ret->flags.inicializado), msg->payload + offset, sizeof(int));
	offset += sizeof(int);

	memcpy((void*) &(ret->flags.error_nro), msg->payload + offset, sizeof(int));
	offset += sizeof(int);

	int elementos_diccionario;
	int i;
	memcpy((void*) &elementos_diccionario, msg->payload + offset, sizeof(int));
	offset += sizeof(int);

	for(i = 0; i< elementos_diccionario; i++){
		char* key;
		int key_len, data, lista_direcciones_len, j;
		t_list* lista_direcciones = list_create();

		memcpy((void*) &key_len, msg->payload + offset, sizeof(unsigned int));
		offset += sizeof(unsigned int);

		key = malloc(key_len + 1);
		memcpy(key, msg->payload + offset, key_len);
		offset += key_len;
		key[key_len] = '\0';

		memcpy((void*) &lista_direcciones_len, msg->payload + offset, sizeof(int));
		offset += sizeof(int);

		for(j = 0; j < lista_direcciones_len; j++){
			int direccion;

			memcpy((void*) &direccion, msg->payload + offset, sizeof(int));
			offset += sizeof(int);

			list_add(lista_direcciones, (void*) direccion);
		}

		dictionary_put(ret->archivos_abiertos, key, (void*) lista_direcciones);
		free(key);
	}
	return ret;
}

t_msg* empaquetar_resultado_abrir(int ok, unsigned int id, char* path, t_list* lista_direcciones){
	int offset = 0;
	t_msg* ret = malloc(sizeof(t_msg));

	void _copiar_memoria_direccion_logica(void* _direccion){
		int direccion = (int) _direccion;
		memcpy(ret->payload + offset, &direccion, sizeof(int));
		offset += sizeof(int);
	}

	ret->header = malloc(sizeof(t_header));
	ret->header->payload_size =
			sizeof(int) + // OK
			sizeof(unsigned int) + // id
			sizeof(unsigned int) + // path_len
			strlen(path) + // path
			sizeof(int) + // lista_direcciones_len
			sizeof(int) * lista_direcciones->elements_count // direcciones
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

	memcpy(ret->payload + offset, (void*) &(lista_direcciones->elements_count), sizeof(int));
	offset += sizeof(int);

	list_iterate(lista_direcciones, _copiar_memoria_direccion_logica);

	return ret;
}

void desempaquetar_resultado_abrir(t_msg* msg, int* ok, unsigned int* id, char** path, t_list** lista_direcciones){
	int i, offset = 0, lista_direcciones_len;
	*lista_direcciones = list_create();

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

	memcpy((void*) &lista_direcciones_len, msg->payload + offset, sizeof(int));
	offset += sizeof(int);

	for(i = 0; i < lista_direcciones_len; i++){
		int direccion;
		memcpy((void*) &direccion, msg->payload + offset, sizeof(int));
		offset += sizeof(int);
		list_add(*lista_direcciones, (void*) direccion);
	}

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

t_msg* empaquetar_get_fm9(unsigned int id, int direccion_logica){
	t_msg* ret = malloc(sizeof(t_msg));
	ret->header = malloc(sizeof(t_header));
	ret->header->payload_size =
		sizeof(unsigned int) + // id
		sizeof(int) // direccion
		;
	ret->payload = malloc(ret->header->payload_size);

	memcpy(ret->payload, (void*) &id, sizeof(unsigned int));
	memcpy(ret->payload + sizeof(unsigned int), (void*) &direccion_logica, sizeof(int));
	return ret;
}

void desempaquetar_get_fm9(t_msg* msg, unsigned int* id, int* direccion_logica){
	memcpy((void*) id, msg->payload, sizeof(unsigned int));
	memcpy((void*) direccion_logica, msg->payload + sizeof(unsigned int), sizeof(int));
}

t_msg* empaquetar_resultado_get_fm9(int ok, char* datos){
	t_msg* ret = malloc(sizeof(t_msg));
	ret->header = malloc(sizeof(t_header));

	int datos_len = strlen(datos);
	ret->header->payload_size =
			sizeof(int) +
			sizeof(int) + // datos_len
			datos_len // datos
			;
	ret->payload = malloc(ret->header->payload_size);

	memcpy(ret->payload, (void*) &ok, sizeof(int));
	memcpy(ret->payload + sizeof(int), (void*) &datos_len, sizeof(int));
	memcpy(ret->payload + sizeof(int) + sizeof(int), datos, datos_len);

	return ret;
}

void desempaquetar_resultado_get_fm9(t_msg* msg, int* ok, char** datos){
	int datos_len;

	memcpy((void*) ok, msg->payload, sizeof(int));

	memcpy((void*) &datos_len, msg->payload + sizeof(int), sizeof(int));

	*datos = malloc(datos_len + 1);
	memcpy((void*) *datos, msg->payload + sizeof(int) + sizeof(int), datos_len);
	(*datos)[datos_len] = '\0';
}

t_msg* empaquetar_tiempo_respuesta(unsigned int id, struct timespec time){
	t_msg* ret = malloc(sizeof(t_msg));
	ret->header = malloc(sizeof(t_header));
	ret->header->payload_size = sizeof(unsigned int) + sizeof(time.tv_sec) + sizeof(time.tv_nsec);
	ret->payload = malloc(ret->header->payload_size);

	memcpy(ret->payload, (void*) &id, sizeof(unsigned int));
	memcpy(ret->payload + sizeof(unsigned int), (void*) &(time.tv_sec), sizeof(time.tv_sec));
	memcpy(ret->payload + sizeof(unsigned int) + sizeof(time.tv_sec), (void*) &(time.tv_nsec), sizeof(time.tv_nsec));
	return ret;
}

void desempaquetar_tiempo_respuesta(t_msg* msg, unsigned int* id, struct timespec* time){
	memcpy((void*) id, msg->payload, sizeof(unsigned int));
	memcpy((void*) &(time->tv_sec), msg->payload + sizeof(unsigned int), sizeof(time->tv_sec));
	memcpy((void*) &(time->tv_nsec), msg->payload + sizeof(unsigned int) + sizeof(time->tv_sec), sizeof(time->tv_nsec));
}

t_msg* empaquetar_reservar_linea_fm9(unsigned int id, int direccion_logica){
	return empaquetar_get_fm9(id, direccion_logica);
}

void desempaquetar_reservar_linea_fm9(t_msg* msg, unsigned int* id, int* direccion_logica){
	desempaquetar_get_fm9(msg, id, direccion_logica);
}

t_msg* empaquetar_resultado_reservar_linea_fm9(int ok, int direccion_logica){
	t_msg* ret = malloc(sizeof(t_msg));
	ret->header = malloc(sizeof(t_header));
	ret->header->payload_size = sizeof(int) + sizeof(int);
	ret->payload = malloc(ret->header->payload_size);

	memcpy(ret->payload, (void*) &ok, sizeof(int));
	memcpy(ret->payload + sizeof(int), (void*) &direccion_logica, sizeof(int));
	return ret;
}

void desempaquetar_resultado_reservar_linea_fm9(t_msg* msg, int* ok, int* direccion_logica){
	memcpy((void*) ok, msg->payload, sizeof(int));
	memcpy((void*) direccion_logica, msg->payload + sizeof(int), sizeof(int));
}

t_msg* empaquetar_resultado_crear_fm9(int ok, int direccion_logica){
	return empaquetar_resultado_reservar_linea_fm9(ok, direccion_logica);
}

void desempaquetar_resultado_crear_fm9(t_msg* msg, int* ok, int* direccion_logica){
	desempaquetar_resultado_reservar_linea_fm9(msg, ok, direccion_logica);
}

t_msg* empaquetar_escribir_fm9(unsigned int id, int direccion_logica, char* datos){
	t_msg* ret = malloc(sizeof(t_msg));
	ret->header = malloc(sizeof(t_header));

	int datos_len = strlen(datos);
	int payload_offset = 0;

	ret->header->payload_size = sizeof(unsigned int) + sizeof(int) + sizeof(int) + datos_len;
	ret->payload = malloc(ret->header->payload_size);

	memcpy(ret->payload, (void*) &id, sizeof(unsigned int));
	payload_offset += sizeof(unsigned int);

	memcpy(ret->payload + payload_offset, (void*) &direccion_logica, sizeof(int));
	payload_offset += sizeof(int);

	memcpy(ret->payload + payload_offset, (void*) &datos_len, sizeof(int));
	payload_offset += sizeof(int);

	memcpy(ret->payload + payload_offset, (void*) datos, datos_len);

	return ret;
}

void desempaquetar_escribir_fm9(t_msg* msg, unsigned int* id, int* direccion_logica, char** datos){
	int payload_offset = 0;
	int datos_len;

	memcpy((void*) id, msg->payload, sizeof(unsigned int));
	payload_offset += sizeof(unsigned int);
	memcpy((void*) direccion_logica, msg->payload + payload_offset, sizeof(int));
	payload_offset += sizeof(int);
	memcpy((void*) &datos_len, msg->payload + payload_offset, sizeof(int));
	payload_offset += sizeof(int);

	*datos = malloc(datos_len + 1);
	memcpy((void*) *datos, msg->payload + payload_offset, datos_len);
	payload_offset += datos_len;
	(*datos)[datos_len] = '\0';
}

t_msg* empaquetar_flush(unsigned int id, char* path, t_list* lista_direcciones){
	t_msg* ret = malloc(sizeof(t_msg));
	int offset = 0;

	void _copiar_memoria_direccion_logica(void* _direccion){
		int direccion = (int) _direccion;
		memcpy(ret->payload + offset, (void*) &direccion, sizeof(int));
		offset += sizeof(int);
	}
	ret->header = malloc(sizeof(t_header));

	int datos_len = strlen(path);
	ret->header->payload_size = sizeof(unsigned int) + sizeof(int) + datos_len + sizeof(int) + sizeof(int)*lista_direcciones->elements_count;
	ret->payload = malloc(ret->header->payload_size);

	memcpy(ret->payload, (void*) &id, sizeof(unsigned int));
	offset += sizeof(unsigned int);
	memcpy(ret->payload + offset, (void*) &datos_len, sizeof(int));
	offset += sizeof(int);
	memcpy(ret->payload + offset, (void*) path, datos_len);
	offset += datos_len;
	memcpy(ret->payload + offset, (void*) &(lista_direcciones->elements_count), sizeof(int));
	offset += sizeof(int);

	list_iterate(lista_direcciones, _copiar_memoria_direccion_logica);

	return ret;
}

void desempaquetar_flush(t_msg* msg, unsigned int* id, char** path, t_list** lista_direcciones){
	int lista_direcciones_len, offset = 0, i;
	*lista_direcciones = list_create();

	memcpy((void*) id, msg->payload, sizeof(unsigned int));
	offset += sizeof(unsigned int);

	int path_len;
	memcpy((void*) &path_len, msg->payload + offset, sizeof(int));
	offset += sizeof(int);

	*path = malloc(path_len + 1);
	memcpy((void*) *path, msg->payload + offset, path_len);
	offset += path_len;
	(*path)[path_len] = '\0';

	memcpy((void*) &lista_direcciones_len, msg->payload + offset, sizeof(int));
	offset += sizeof(int);

	for(i = 0; i < lista_direcciones_len; i++){
		int direccion;
		memcpy((void*) &direccion, msg->payload + offset, sizeof(int));
		offset += sizeof(int);
		list_add(*lista_direcciones, (void*) direccion);
	}
}

t_msg* empaquetar_crear_mdj(unsigned int id, char* path, int cant_lineas){
	t_msg* ret = malloc(sizeof(t_msg));
	ret->header = malloc(sizeof(t_header));

	int path_len = strlen(path);
	ret->header->payload_size = sizeof(unsigned int) + sizeof(int) + path_len + sizeof(int);
	ret->payload = malloc(ret->header->payload_size);

	memcpy(ret->payload, (void*) &id, sizeof(unsigned int));
	memcpy(ret->payload + sizeof(unsigned int), (void*) &path_len, sizeof(int));
	memcpy(ret->payload + sizeof(unsigned int) + sizeof(int), (void*) path, path_len);
	memcpy(ret->payload + sizeof(unsigned int) + sizeof(int) + path_len, (void*) &cant_lineas, sizeof(int));
	return ret;
}

void desempaquetar_crear_mdj(t_msg* msg, unsigned int* id, char** path, int* cant_lineas){
	memcpy((void*) id, msg->payload, sizeof(unsigned int));

	int path_len;
	memcpy((void*) &path_len, msg->payload + sizeof(unsigned int), sizeof(int));

	*path = malloc(path_len + 1);
	memcpy((void*) *path, msg->payload + sizeof(unsigned int) + sizeof(int), path_len);
	(*path)[path_len] = '\0';

	memcpy((void*) cant_lineas, msg->payload + sizeof(unsigned int) + sizeof(int) + path_len, sizeof(int));
}

t_msg* empaquetar_get_mdj(char* path, int offset, int size){
	t_msg* ret = malloc(sizeof(t_msg));
	ret->header = malloc(sizeof(t_header));

	int path_len = strlen(path);
	ret->header->payload_size = sizeof(int) + path_len + sizeof(int) + sizeof(int);
	ret->payload = malloc(ret->header->payload_size);

	memcpy(ret->payload, (void*) &path_len, sizeof(int));
	memcpy(ret->payload + sizeof(int), (void*) path, path_len);
	memcpy(ret->payload + sizeof(int) + path_len, (void*) &offset, sizeof(int));
	memcpy(ret->payload + sizeof(int) + path_len + sizeof(int), (void*) &size, sizeof(int));
	return ret;
}

void desempaquetar_get_mdj(t_msg* msg, char** path, int* offset, int* size){
	int path_len;
	memcpy((void*) &path_len, msg->payload, sizeof(int));

	*path = malloc(path_len + 1);
	memcpy((void*) *path, msg->payload + sizeof(int), path_len);
	(*path)[path_len] = '\0';

	memcpy((void*) offset, msg->payload + sizeof(int) + path_len, sizeof(int));
	memcpy((void*) size, msg->payload + sizeof(int) + path_len + sizeof(int), sizeof(int));
}

t_msg* empaquetar_escribir_mdj(char* path, int offset, int size, void* buffer){
	int payload_offset = 0;
	t_msg* ret = malloc(sizeof(t_msg));
	ret->header = malloc(sizeof(t_header));

	int path_len = strlen(path);
	ret->header->payload_size = sizeof(int) + path_len + sizeof(int) + sizeof(int) + size;
	ret->payload = malloc(ret->header->payload_size);

	memcpy(ret->payload , (void*) &path_len, sizeof(int));
	payload_offset += sizeof(int);
	memcpy(ret->payload + payload_offset, (void*) path, path_len);
	payload_offset += path_len;
	memcpy(ret->payload + payload_offset, (void*) &offset, sizeof(int));
	payload_offset += sizeof(int);
	memcpy(ret->payload + payload_offset, (void*) &size, sizeof(int));
	payload_offset += sizeof(int);
	memcpy(ret->payload + payload_offset, buffer, size);
	return ret;
}

void desempaquetar_escribir_mdj(t_msg* msg, char** path, int* offset, int* size, void** buffer){
	int payload_offset = 0;
	int path_len;
	memcpy((void*) &path_len, msg->payload, sizeof(int));
	payload_offset += sizeof(int);

	*path = malloc(path_len + 1);
	memcpy((void*) *path, msg->payload + payload_offset, path_len);
	payload_offset += path_len;
	(*path)[path_len] = '\0';

	memcpy((void*) offset, msg->payload + payload_offset, sizeof(int));
	payload_offset += sizeof(int);
	memcpy((void*) size, msg->payload + payload_offset, sizeof(int));
	payload_offset += sizeof(int);

	*buffer = malloc(*size);
	memcpy(*buffer, msg->payload + payload_offset, *size);
}

t_msg* empaquetar_borrar(unsigned int id, char* path){
	return empaquetar_abrir(path, id);
}

void desempaquetar_borrar(t_msg* msg, unsigned int* id, char** path){
	desempaquetar_abrir(msg, path, id);
}

t_msg* empaquetar_resultado_io(int ok, unsigned int id){
	return empaquetar_get_fm9(id, ok);
}

void desempaquetar_resultado_io(t_msg* msg, int* ok, unsigned int* id){
	desempaquetar_get_fm9(msg, id, ok);
}

t_msg* empaquetar_close(unsigned int id, t_list* lista_direcciones){
	int offset = 0;
	t_msg* ret = malloc(sizeof(t_msg));

	void _copiar_memoria_direccion_logica(void* _direccion){
		int direccion = (int) _direccion;
		memcpy(ret->payload + offset, (void*) &direccion, sizeof(int));
		offset += sizeof(int);
	}

	ret->header = malloc(sizeof(t_header));

	ret->header->payload_size = sizeof(unsigned int) + sizeof(int) + sizeof(int)*lista_direcciones->elements_count;
	ret->payload = malloc(ret->header->payload_size);

	memcpy(ret->payload, (void*) &id, sizeof(unsigned int));
	offset += sizeof(unsigned int);

	memcpy(ret->payload + offset, (void*) &(lista_direcciones->elements_count), sizeof(int));
	offset += sizeof(int);

	list_iterate(lista_direcciones, _copiar_memoria_direccion_logica);

	return ret;
}

void desempaquetar_close(t_msg* msg, unsigned int* id, t_list** lista_direcciones){
	*lista_direcciones = list_create();
	int lista_direcciones_len, offset = 0, i;

	memcpy((void*) id, msg->payload, sizeof(unsigned int));
	offset += sizeof(unsigned int);

	memcpy((void*) &lista_direcciones_len, msg->payload + offset, sizeof(int));
	offset += sizeof(int);

	for(i = 0; i < lista_direcciones_len; i++){
		int direccion;
		memcpy((void*) &direccion, msg->payload + offset, sizeof(int));
		offset += sizeof(int);
		list_add(*lista_direcciones, (void*) direccion);
	}
}




