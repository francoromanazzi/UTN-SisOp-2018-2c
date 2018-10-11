#include "MDJ.h"

//void mdj_enviar_datos_HARDCODEADO(int dam_socket, char* path, int offset, int size);
static void _hardcodear_archivos();
static int _mkpath(char* file_path, mode_t mode);

int main(void) {

	void config_create_fixed(){
		config = config_create(CONFIG_PATH);
		util_config_fix_comillas(&config, "PUNTO_MONTAJE");


		char* punto_montaje_un_directorio_atras = string_new();
		string_append(&punto_montaje_un_directorio_atras, "..");
		string_append(&punto_montaje_un_directorio_atras, config_get_string_value(config, "PUNTO_MONTAJE"));
		config_set_value(config, "PUNTO_MONTAJE", punto_montaje_un_directorio_atras);
		free(punto_montaje_un_directorio_atras);


		char* retardo_microsegundos_str = string_itoa(1000 * config_get_int_value(config, "RETARDO"));
		config_set_value(config, "RETARDO", retardo_microsegundos_str); // Milisegundos a microsegundos
		free(retardo_microsegundos_str);
	}

	config_create_fixed();

	mkdir(LOG_DIRECTORY_PATH,0777);
	logger = log_create(LOG_PATH, "MDJ", false, LOG_LEVEL_TRACE);

	crearEstructuras();
	_hardcodear_archivos();

	if((listenning_socket = socket_create_listener(IP, config_get_string_value(config, "PUERTO"))) == -1){
		log_error(logger, "No pude crear el socket de escucha");
		mdj_exit();
		exit(EXIT_FAILURE);
	}
	log_info(logger,"Escucho en el socket %d", listenning_socket);

	pthread_t thread_consola;
	if(pthread_create(&thread_consola,NULL,(void*) mdj_consola_init, NULL)){
		log_error(logger,"No se pudo crear el hilo para la consola");
		return -1;
	}
	log_info(logger,"Se creo el hilo para la consola");

	socket_start_listening_select(listenning_socket, mdj_manejador_de_eventos);

	mdj_exit();
	return EXIT_SUCCESS;
}

static int _mkpath(char* file_path, mode_t mode) {
	log_info(logger, "%s",file_path);
  assert(file_path && *file_path);
  char* p;
  for (p=strchr(file_path+1, '/'); p; p=strchr(p+1, '/')) {
    *p='\0';
    if (mkdir(file_path, mode)==-1) {
      if (errno!=EEXIST) { *p='/'; return -1; }
    }
    *p='/';
  }
  return 0;
}

int mdj_send(int socket, e_tipo_msg tipo_msg, ...){
	t_msg* mensaje_a_enviar;
	int ret, ok, size;
	char* str;
	void* buffer;

 	va_list arguments;
	va_start(arguments, tipo_msg);

	switch(tipo_msg){
		case RESULTADO_VALIDAR:
			ok = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_int(ok);
		break;

		case RESULTADO_CREAR_MDJ:
			ok = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_int(ok);
		break;

		case RESULTADO_GET_MDJ:
			buffer = va_arg(arguments, void*);
			size = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_void_ptr(buffer, size);
		break;

		case RESULTADO_ESCRIBIR_MDJ:
			// TODO
		break;

		case RESULTADO_BORRAR:
		break;
	}

	mensaje_a_enviar->header->emisor = MDJ;
	mensaje_a_enviar->header->tipo_mensaje = tipo_msg;
	ret = msg_send(socket, *mensaje_a_enviar);
	msg_free(&mensaje_a_enviar);
	va_end(arguments);
	return ret;
}

int mdj_manejador_de_eventos(int socket, t_msg* msg){
	log_info(logger, "EVENTO: Emisor: %d, Tipo: %d, Tamanio: %d",msg->header->emisor,msg->header->tipo_mensaje,msg->header->payload_size);

	char* path;
	void* buffer;
	int ok, offset, size, cant_lineas, buffer_size;

	if(msg->header->emisor == DAM){
		switch(msg->header->tipo_mensaje){
			case CONEXION:
				log_info(logger, "Se conecto un nuevo hilo de DAM");
			break;

			case DESCONEXION:
				log_info(logger, "Se desconecto un hilo de DAM");
				return -1;
			break;

			case VALIDAR: // Puede ser por ABRIR o por FLUSH
				log_info(logger, "Recibi ordenes de DAM de validar un archivo");
				path = desempaquetar_string(msg);

				validarArchivo(path, &ok);
				if(ok == MDJ_ERROR_PATH_INEXISTENTE) ok = MDJ_ERROR_PATH_INEXISTENTE; // TODO cambiar el codigo de error?
				free(path);

				usleep(config_get_int_value(config, "RETARDO"));
				mdj_send(socket, RESULTADO_VALIDAR, ok);
			break;

			case CREAR_MDJ:
				log_info(logger, "Recibi ordenes de DAM de crear un archivo");
				desempaquetar_crear(msg, &path, &cant_lineas);

				crearArchivo(path, cant_lineas, &ok);
				free(path);
				if(ok == MDJ_ERROR_ARCHIVO_YA_EXISTENTE){
					ok = ERROR_CREAR_ARCHIVO_YA_EXISTENTE;
					log_error(logger, "El archivo ya existe");
				}
				else if(ok == MDJ_ERROR_ESPACIO_INSUFICIENTE){
					ok = ERROR_CREAR_ESPACIO_INSUFICIENTE_MDJ;
					log_error(logger, "No hay espacio");
				}
				else {
					log_info(logger, "Pude crear el archivo %s con %d lineas", path, cant_lineas);
				}

				usleep(config_get_int_value(config, "RETARDO"));
				mdj_send(socket, RESULTADO_CREAR_MDJ, ok);
			break;

			case GET_MDJ:
				log_info(logger, "Recibi ordenes de DAM de obtener datos");
				desempaquetar_get_mdj(msg, &path, &offset, &size);
				obtenerDatos(path, offset, size, &buffer, &buffer_size);
				//mdj_enviar_datos_HARDCODEADO(socket, path, offset, size);
				free(path);

				char* buffer_str = malloc(buffer_size + 1);
				memcpy((void*) buffer_str, buffer, buffer_size);
				buffer_str[buffer_size] = '\0';
				log_info(logger, "Le envio %d bytes con %s a DAM", buffer_size, buffer_str);
				free(buffer_str);

				usleep(config_get_int_value(config, "RETARDO"));
				mdj_send(socket, RESULTADO_GET_MDJ, buffer, buffer_size);
				free(buffer);
			break;

			case ESCRIBIR_MDJ:
				log_info(logger, "Recibi ordenes de DAM de escribir en un archivo");
				//desempaquetar_
			break;

			case BORRAR:
				log_info(logger, "Recibi ordenes de DAM de borrar un archivo");
			break;

			default:
				log_error(logger, "No entendi el mensaje de DAM");
		}
	}
	else if(msg->header->emisor == DESCONOCIDO){
		log_info(logger, "Me hablo alguien desconocido");
	}
	return 1;
}

void crearEstructuras(){
	cant_bloques = 32;
	char* cantidad_bloques_str = string_itoa(cant_bloques);
	tam_bloques = 64;
	char* tamanio_bloques_str = string_itoa(tam_bloques);
	char* dir_metadata = string_new();
	char* dir_archivos = string_new();
	char* dir_bloques = string_new();
	char* bin_metadata = string_new();
	char* bin_bitmap = string_new();
	char* data_inicial_bin_metadata = string_new();
	char* bitarray_str = calloc(cant_bloques / 8, sizeof(char));
	t_bitarray* bitarray_bin_bitmap = bitarray_create_with_mode(bitarray_str, cant_bloques / 8, MSB_FIRST);
	FILE* f_metadata;

	if(_mkpath(config_get_string_value(config, "PUNTO_MONTAJE"), 0755) == -1){
		log_info(logger, "AAA");
	}

	string_append(&dir_metadata, config_get_string_value(config, "PUNTO_MONTAJE"));
	string_append(&dir_metadata, "Metadata/");
	mkdir(dir_metadata, 0777);
	log_info(logger, "Creada carpeta Metadata");

	string_append(&dir_archivos, config_get_string_value(config, "PUNTO_MONTAJE"));
	string_append(&dir_archivos, "Archivos/");
	mkdir(dir_archivos, 0777);
	log_info(logger, "Creada carpeta Archivos");

	string_append(&dir_bloques, config_get_string_value(config, "PUNTO_MONTAJE"));
	string_append(&dir_bloques, "Bloques/");
	mkdir(dir_bloques, 0777);
	log_info(logger, "Creada carpeta Bloques");

	string_append(&bin_metadata, dir_metadata);
	string_append(&bin_metadata, "/Metadata.bin");
	if((f_metadata = fopen(bin_metadata, "r")) == NULL){ // Si no existe el archivo metadata
		f_metadata = fopen(bin_metadata, "wb+");
		config_metadata = config_create(bin_metadata);
		config_set_value(config_metadata, "TAMANIO_BLOQUES", tamanio_bloques_str);
		config_set_value(config_metadata, "CANTIDAD_BLOQUES", cantidad_bloques_str);
		config_set_value(config_metadata, "MAGIC_NUMBER", "FIFA");
		config_save(config_metadata);
	}
	else
		config_metadata = config_create(bin_metadata);

	fclose(f_metadata);
	log_info(logger, "Creado el archivo Metadata.bin");

	string_append(&bin_bitmap, dir_metadata);
	string_append(&bin_bitmap, "/Bitmap.bin");
	f_bitmap = fopen(bin_bitmap, "wb+");
	/* Serializo la estructura bitarray y la escribo */
	fwrite((void*) &(bitarray_bin_bitmap->size), sizeof(size_t), 1, f_bitmap);
	fwrite((void*) bitarray_bin_bitmap->bitarray, sizeof(char), bitarray_bin_bitmap->size, f_bitmap);
	fwrite((void*) &(bitarray_bin_bitmap->mode), sizeof(bit_numbering_t), 1, f_bitmap);
	fflush(f_bitmap);
	log_info(logger, "Creado el archivo Bitmap.bin");

	paths_estructuras[METADATA] = dir_metadata;
	paths_estructuras[ARCHIVOS] = dir_archivos;
	paths_estructuras[BLOQUES] = dir_bloques;

	bitarray_destroy(bitarray_bin_bitmap);
	free(cantidad_bloques_str);
	free(tamanio_bloques_str);
	free(bin_metadata);
	free(bin_bitmap);
	free(data_inicial_bin_metadata);
	free(bitarray_str);
}

static void _hardcodear_archivos(){
	char* buffer_str;
	void* buffer;
	int ok;

	/* Escriptorio: test1.bin */
	crearArchivo("/Escriptorios/test1.bin", 3, &ok);
	buffer_str = strdup("concentrar\nconcentrar\nconcentrar\n\n");
	buffer = malloc(strlen(buffer_str));
	memcpy(buffer, (void*) buffer_str, strlen(buffer_str));
	guardarDatos("/Escriptorios/test1.bin", 0, strlen(buffer_str), buffer, &ok);
	free(buffer);
	free(buffer_str);
}

t_bitarray* mdj_bitmap_abrir(){
	t_bitarray* ret;
	size_t size;
	char* bitarray_str;
	bit_numbering_t mode;

	fseek(f_bitmap, 0, SEEK_SET);
	fread((void*) &size, sizeof(size_t), 1, f_bitmap);
	bitarray_str = malloc(size);
	fread((void*) bitarray_str, sizeof(char), size, f_bitmap);
	fread((void*) &mode, sizeof(bit_numbering_t), 1, f_bitmap);
	ret = bitarray_create_with_mode(bitarray_str, size, mode);
	return ret;
}

void mdj_bitmap_save(t_bitarray* bitarray){
	fseek(f_bitmap, 0, SEEK_SET);
	fwrite((void*) &(bitarray->size), sizeof(size_t), 1, f_bitmap);
	fwrite((void*) bitarray->bitarray, sizeof(char), bitarray->size, f_bitmap);
	fwrite((void*) &(bitarray->mode), sizeof(bit_numbering_t), 1, f_bitmap);
	fflush(f_bitmap);
}

void validarArchivo(char* path, int* ok){
	FILE* f;
	char* rutaFinal = string_new();
	string_append(&rutaFinal, paths_estructuras[ARCHIVOS]);
	string_append(&rutaFinal, path);



	if((f = fopen(rutaFinal, "rb")) != NULL){ // Encontro el archivo
		log_info(logger, "Encontre el archivo %s", rutaFinal);
		fclose(f);
		*ok = OK;
	}
	else{
		log_error(logger, "No encontre el archivo %s", rutaFinal);
		*ok = MDJ_ERROR_PATH_INEXISTENTE;
	}
	free(rutaFinal);
}

void crearArchivo(char* path, int cant_lineas, int* ok){

	char* _list_to_string(t_list* list){
		int i;
		char* ret = string_new();
		char* aux;
		string_append(&ret, "[");
		for(i = 0; i<list->elements_count; i++){
			aux = string_itoa((int) list_get(list, i));
			string_append(&ret, aux);
			free(aux);
			if(i < list->elements_count - 1){
				string_append(&ret, ",");
			}
		}
		string_append(&ret, "]");
		return ret;
	}

	FILE *archivo;
	int i, bloques_necesarios = (cant_lineas / tam_bloques) + 1;
	t_list* lista_nro_bloques = list_create();
	char* rutaFinal = string_new();
	string_append(&rutaFinal, paths_estructuras[ARCHIVOS]);
	string_append(&rutaFinal, path);

	if((archivo = fopen(rutaFinal, "rb")) != NULL){ // El archivo a crear ya existia
		fclose(archivo);
		*ok = MDJ_ERROR_ARCHIVO_YA_EXISTENTE;
		free(rutaFinal);
		list_destroy(lista_nro_bloques);
		return;
	}

	/* Busco en el bitmap la cantidad de bloques necesarios */
	t_bitarray* bitmap = mdj_bitmap_abrir();
	for(i = 0; i < bitmap->size && lista_nro_bloques->elements_count < bloques_necesarios; i++){
		if(!bitarray_test_bit(bitmap, i)){ // Bloque disponible
			list_add(lista_nro_bloques, (void*) i);
			bitarray_set_bit(bitmap, i);
		}
	}

	if(lista_nro_bloques->elements_count < bloques_necesarios){ // Espacio insuficiente
		*ok = MDJ_ERROR_ESPACIO_INSUFICIENTE;
		free(rutaFinal);
		list_destroy(lista_nro_bloques);
		free(bitmap->bitarray);
		bitarray_destroy(bitmap);
		return;
	}

	/* OK, puedo crear el archivo */
	*ok = OK;
	_mkpath(rutaFinal, 0777); // Creo todos los directorios necesarios del path

	mdj_bitmap_save(bitmap); // Actualizo archivo bitmap
	free(bitmap->bitarray);
	bitarray_destroy(bitmap);

	/* Creo el archivo */
	archivo = fopen(rutaFinal, "wb+");
	fclose(archivo);
	t_config* config_archivo = config_create(rutaFinal);
	char* cant_lineas_str = string_itoa(cant_lineas);
	config_set_value(config_archivo, "TAMANIO", cant_lineas_str);
	free(cant_lineas_str);
	char* bloques_str = _list_to_string(lista_nro_bloques);
	config_set_value(config_archivo, "BLOQUES", bloques_str);
	config_save(config_archivo);

	/* Creo los bloques vacios y les escribo los /n necesarios */
	for(i = 0; i<lista_nro_bloques->elements_count; i++){
		char* ruta_bloque = string_new();
		char* nro_bloque_str = string_itoa((int) list_get(lista_nro_bloques, i));
		char vacio = ' ';
		char salto_de_linea = '\n';

		string_append(&ruta_bloque, paths_estructuras[BLOQUES]);
		string_append(&ruta_bloque, nro_bloque_str);
		string_append(&ruta_bloque, ".bin");
		archivo = fopen(ruta_bloque, "wb+");
		fwrite((void*) &vacio, sizeof(char), config_get_int_value(config_metadata, "TAMANIO_BLOQUES"), archivo);
		fflush(archivo);

		/* Escribo los \n necesarios */
		fseek(archivo, 0 ,SEEK_SET);
		int cant_saltos_de_linea = ((cant_lineas - (tam_bloques * i)) - tam_bloques) < 0 ? cant_lineas - (tam_bloques * i) : tam_bloques;
		fwrite((void*) &salto_de_linea, sizeof(char), cant_saltos_de_linea, archivo);
		fflush(archivo);

		fclose(archivo);
		free(ruta_bloque);
		free(nro_bloque_str);
	}


	log_info(logger, "Creo el archivo %s con %d bloques. Los mismos son: %s", path, lista_nro_bloques->elements_count, bloques_str);
	list_destroy(lista_nro_bloques);
	config_destroy(config_archivo);
	free(bloques_str);
	free(rutaFinal);
}

void obtenerDatos(char* path, int offset, int bytes_restantes, void** ret_buffer, int* ret_buffer_size){
	*ret_buffer = malloc(bytes_restantes);
	*ret_buffer_size = 0;
	int i = 0;
	char** bloques_strings;
	t_config* config_archivo;

	void _agregar_bloque_a_buffer(char* nro_bloque, int offset, int indice_bloque){
		char* ruta_bloque = string_new();
		string_append(&ruta_bloque, paths_estructuras[BLOQUES]);
		string_append(&ruta_bloque, nro_bloque);
		string_append(&ruta_bloque, ".bin");
		FILE* bloque = fopen(ruta_bloque, "rb");
		fseek(bloque, offset, SEEK_SET);
		free(ruta_bloque);

		int cant_bytes_a_leer = (config_get_int_value(config_metadata, "TAMANIO_BLOQUES") - offset) > bytes_restantes
				? bytes_restantes : config_get_int_value(config_metadata, "TAMANIO_BLOQUES") - offset;

		/* Me fijo si es el ultimo bloque */
		if(split_cant_elem(bloques_strings) - 1 == indice_bloque){
			int tam_ultimo_bloque = config_get_int_value(config_archivo, "TAMANIO") -
					(i * config_get_int_value(config_metadata, "TAMANIO_BLOQUES"));
			cant_bytes_a_leer = (cant_bytes_a_leer + offset > tam_ultimo_bloque) ? tam_ultimo_bloque - offset : cant_bytes_a_leer;
		}

		void* data = malloc(cant_bytes_a_leer);
		int bytes_leidos = fread(data, 1, cant_bytes_a_leer, bloque);
		memcpy(*ret_buffer, data, bytes_leidos);
		*ret_buffer_size += bytes_leidos;
		free(data);
		fclose(bloque);
	}

	char* rutaFinal = string_new();
	string_append(&rutaFinal, paths_estructuras[ARCHIVOS]);
	string_append(&rutaFinal, path);
	config_archivo = config_create(rutaFinal);
	free(rutaFinal);

	bloques_strings = config_get_array_value(config_archivo, "BLOQUES");
	// Agrego el bloque inicial:
	int indice_bloque_inicial = offset/config_get_int_value(config_metadata, "TAMANIO_BLOQUES");
	int offset_bloque_inicial = offset - (indice_bloque_inicial * config_get_int_value(config_metadata, "TAMANIO_BLOQUES"));
	_agregar_bloque_a_buffer(bloques_strings[indice_bloque_inicial],  offset_bloque_inicial, indice_bloque_inicial);
	bytes_restantes -= (config_get_int_value(config_metadata, "TAMANIO_BLOQUES") - offset_bloque_inicial);

	// Agrego el resto de bloques a abrir

	while(bytes_restantes > 0){
		int indice = indice_bloque_inicial + ++i;
		_agregar_bloque_a_buffer(bloques_strings[indice], 0, indice);
		bytes_restantes -= config_get_int_value(config_metadata, "TAMANIO_BLOQUES");
	}

	config_destroy(config_archivo);
	split_liberar(bloques_strings);
}

void guardarDatos(char* path, int offset, int bytes_restantes, void* buffer, int* ok){
	char* bloques_string;
	int offset_buffer = 0;
	int tamanio_total = 0;

	void _escribir_bloque(char* nro_bloque, int offset){
		char* ruta_bloque = string_new();
		string_append(&ruta_bloque, paths_estructuras[BLOQUES]);
		string_append(&ruta_bloque, nro_bloque);
		string_append(&ruta_bloque, ".bin");
		FILE* bloque = fopen(ruta_bloque, "rb+");
		fseek(bloque, offset, SEEK_SET);
		free(ruta_bloque);

		int cant_bytes_a_escribir = (config_get_int_value(config_metadata, "TAMANIO_BLOQUES") - offset) > bytes_restantes ?
				bytes_restantes : config_get_int_value(config_metadata, "TAMANIO_BLOQUES") - offset;
		tamanio_total += cant_bytes_a_escribir;
		fwrite(buffer + offset_buffer, 1, cant_bytes_a_escribir, bloque);
		offset_buffer += cant_bytes_a_escribir;

		fclose(bloque);
	}

	char* _crear_bloque(){
		char* ruta_bloque = string_new();
		char vacio = '\0';
		char* nro_bloque_str;
		int nro_bloque;
		bool bloque_disponible;
		t_bitarray* bitarray = mdj_bitmap_abrir();

		for(nro_bloque = 0; nro_bloque < bitarray->size && !(bloque_disponible = !(bitarray_test_bit(bitarray, nro_bloque))); nro_bloque++);

		if(!bloque_disponible){
			*ok = MDJ_ERROR_ESPACIO_INSUFICIENTE;
			return NULL;
		}
		bitarray_set_bit(bitarray, nro_bloque);
		mdj_bitmap_save(bitarray);
		free(bitarray->bitarray);
		bitarray_destroy(bitarray);
		nro_bloque_str = string_itoa(nro_bloque);

		string_append(&ruta_bloque, paths_estructuras[BLOQUES]);
		string_append(&ruta_bloque, nro_bloque_str);
		string_append(&ruta_bloque, ".bin");
		FILE* archivo = fopen(ruta_bloque, "wb+");
		fwrite((void*) &vacio, sizeof(char), config_get_int_value(config_metadata, "TAMANIO_BLOQUES"), archivo);
		fflush(archivo);
		fclose(archivo);

		bloques_string[strlen(bloques_string) - 1] = '\0';
		string_append(&bloques_string, ",");
		string_append(&bloques_string, nro_bloque_str);
		string_append(&bloques_string, "]");
		free(ruta_bloque);
		return nro_bloque_str;
	}

	*ok = OK;

	char* rutaFinal = string_new();
	string_append(&rutaFinal, paths_estructuras[ARCHIVOS]);
	string_append(&rutaFinal, path);
	t_config* config_archivo = config_create(rutaFinal);
	free(rutaFinal);

	char** bloques_arr_strings = config_get_array_value(config_archivo, "BLOQUES");
	bloques_string = strdup(config_get_string_value(config_archivo, "BLOQUES"));
	int bloques_strings_len = split_cant_elem(bloques_arr_strings);
	int indice_bloque_inicial = offset/config_get_int_value(config_metadata, "TAMANIO_BLOQUES");
	int offset_bloque_inicial = offset - (indice_bloque_inicial * config_get_int_value(config_metadata, "TAMANIO_BLOQUES"));
	_escribir_bloque(bloques_arr_strings[indice_bloque_inicial],  offset_bloque_inicial);
	bytes_restantes -= (config_get_int_value(config_metadata, "TAMANIO_BLOQUES") - offset_bloque_inicial);

	int i;
	// Agrego el resto de bloques a abrir
	for(i = 0; i < bytes_restantes; i+= config_get_int_value(config_metadata, "TAMANIO_BLOQUES")){
		if(i + 1 < bloques_strings_len){
			char* nro_bloque = _crear_bloque();
			if(*ok != OK){
				free(bloques_string);
				config_destroy(config_archivo);
				split_liberar(bloques_arr_strings);
				return;
			}

			_escribir_bloque(nro_bloque, 0);
			free(nro_bloque);
		}
		else{
			_escribir_bloque(bloques_arr_strings[indice_bloque_inicial + i + 1], 0);
		}

	}

	config_remove_key(config_archivo, "BLOQUES");
	config_set_value(config_archivo, "BLOQUES", bloques_string);
	split_liberar(bloques_arr_strings);
	bloques_arr_strings = config_get_array_value(config_archivo, "BLOQUES");

	char* tamanio_str = string_itoa(tamanio_total);
	config_set_value(config_archivo, "TAMANIO", tamanio_str);
	config_save(config_archivo);
	free(tamanio_str);

	free(bloques_string);
	config_destroy(config_archivo);
	split_liberar(bloques_arr_strings);
}

void borrarArchivo(char* path, int* ok){
	char* rutaFinal = string_new();
	string_append(&rutaFinal, "../");
	string_append(&rutaFinal, path);

	validarArchivo(rutaFinal, ok);
	if(*ok == OK){
		remove(rutaFinal);

		//PENDIENTE actualizar bitmap

		log_info(logger, "Archivo borrado correctamente");
	}else{
		log_error(logger,"El archivo no puede ser borrado ya que no existe");
	}
	free(rutaFinal);
}

void mdj_exit(){
	log_destroy(logger);
	config_destroy(config);
}

/*
void mdj_enviar_datos_HARDCODEADO(int dam_socket, char* path, int offset, int size){
	FILE* f = fopen(path, "r");
	void* buffer = malloc(size);

	if(f == NULL){
		free(buffer);
		return;
	}

	fseek(f, offset, SEEK_SET);
	int caracteres_leidos = fread(buffer, 1, size, f);

	usleep(config_get_int_value(config, "RETARDO"));

	char* buffer_str = malloc(caracteres_leidos + 1);
	memcpy((void*) buffer_str, buffer, caracteres_leidos);
	buffer_str[caracteres_leidos] = '\0';
	log_info(logger, "Le envio %s a DAM", buffer_str);
	free(buffer_str);

	mdj_send(dam_socket, RESULTADO_GET_MDJ, buffer, caracteres_leidos);

	free(buffer);
	fclose(f);
}
*/







