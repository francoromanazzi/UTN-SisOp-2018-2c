#include "MDJ.h"

static void _hardcodear_archivos();
static int _mkpath(char* file_path, mode_t mode);


int main(void) {

	void config_create_fixed(){
		config = config_create(CONFIG_PATH);
		util_config_fix_comillas(&config, "PUNTO_MONTAJE");

		/* DEPRACATED:
		char* punto_montaje_un_directorio_atras = string_new();
		string_append(&punto_montaje_un_directorio_atras, "..");
		string_append(&punto_montaje_un_directorio_atras, config_get_string_value(config, "PUNTO_MONTAJE"));
		config_set_value(config, "PUNTO_MONTAJE", punto_montaje_un_directorio_atras);
		free(punto_montaje_un_directorio_atras);
		*/

		char* retardo_microsegundos_str = string_itoa(1000 * config_get_int_value(config, "RETARDO"));
		config_set_value(config, "RETARDO", retardo_microsegundos_str); // Milisegundos a microsegundos
		free(retardo_microsegundos_str);
	}

	config_create_fixed();

	mkdir(LOG_DIRECTORY_PATH,0777);
	logger = log_create(LOG_PATH, "MDJ", false, LOG_LEVEL_TRACE);

	crearEstructuras();
	_hardcodear_archivos();

	char* local_ip = get_local_ip();
	if((listenning_socket = socket_create_listener(local_ip, config_get_string_value(config, "PUERTO"))) == -1){
		log_error(logger, "No pude crear el socket de escucha");
		free(local_ip);
		mdj_exit();
		exit(EXIT_FAILURE);
	}
	log_info(logger,"Escucho en el socket %d. Mi IP es: %s", listenning_socket, local_ip);
	free(local_ip);

	pthread_t thread_consola;
	if(pthread_create(&thread_consola,NULL,(void*) mdj_consola_init, NULL)){
		log_error(logger,"No se pudo crear el hilo para la consola");
		return -1;
	}
	log_info(logger,"Se creo el hilo para la consola");

	socket_start_listening_select(listenning_socket, mdj_manejador_de_eventos, 0);

	mdj_exit();
	return EXIT_SUCCESS;
}

static int _mkpath(char* file_path, mode_t mode) {
	assert(file_path && *file_path);
	char* p;
	for (p=strchr(file_path+1, '/'); p; p=strchr(p+1, '/')) {
		*p='\0';
		if (mkdir(file_path, mode)==-1) {
			if (errno!=EEXIST) {
				*p='/';
				return -1;
			}
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
			ok = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_int(ok);
		break;

		case RESULTADO_BORRAR:
			ok = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_int(ok);
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

	char* path, *buffer_str;
	void* buffer;
	int ok, offset, size, cant_lineas, buffer_size;
	unsigned int _basura;

	if(msg->header->emisor == DAM){
		switch(msg->header->tipo_mensaje){
			case CONEXION:
				log_info(logger, "Se conecto un nuevo hilo de DAM");
			break;

			case DESCONEXION:
				log_info(logger, "Se desconecto un hilo de DAM");
				return -1;
			break;

			case VALIDAR: // Puede ser por ABRIR o por FLUSH o por CREAR
				path = desempaquetar_string(msg);
				log_info(logger, "Recibi ordenes de DAM de validar el archivo %s", path);

				validarArchivo(path, &ok);
				free(path);

				usleep(config_get_int_value(config, "RETARDO"));
				mdj_send(socket, RESULTADO_VALIDAR, ok);
			break;

			case CREAR_MDJ:
				desempaquetar_crear_mdj(msg, &_basura, &path, &cant_lineas);
				log_info(logger, "Recibi ordenes de DAM de crear el archivo %s con %d lineas", path, cant_lineas);

				crearArchivo(path, cant_lineas, &ok);

				if(ok == MDJ_ERROR_ESPACIO_INSUFICIENTE){
					ok = ERROR_CREAR_ESPACIO_INSUFICIENTE_MDJ;
					log_error(logger, "No hay espacio para crear el archivo");
				}

				free(path);
				usleep(config_get_int_value(config, "RETARDO"));
				mdj_send(socket, RESULTADO_CREAR_MDJ, ok);
			break;

			case GET_MDJ:
				desempaquetar_get_mdj(msg, &path, &offset, &size);
				log_info(logger, "Recibi ordenes de DAM de obtener %d bytes del archivo %s, con offset: %d", size, path, offset);

				obtenerDatos(path, offset, size, &buffer, &buffer_size);
				free(path);

				buffer_str = malloc(buffer_size + 1);
				memcpy((void*) buffer_str, buffer, buffer_size);
				buffer_str[buffer_size] = '\0';
				log_info(logger, "Le envio %d bytes con %s a DAM", buffer_size, buffer_str);
				free(buffer_str);

				usleep(config_get_int_value(config, "RETARDO"));
				mdj_send(socket, RESULTADO_GET_MDJ, buffer, buffer_size);
				free(buffer);
			break;

			case ESCRIBIR_MDJ:
				desempaquetar_escribir_mdj(msg, &path, &offset, &buffer_size, &buffer);
				buffer_str = malloc(buffer_size + 1);
				memcpy((void*) buffer_str, buffer, buffer_size);
				buffer_str[buffer_size] = '\0';
				log_info(logger, "Recibi ordenes de DAM de escribir %d bytes con %s en el archivo %s, con offset: %d",
						buffer_size, buffer_str, path, offset);
				free(buffer_str);

				guardarDatos(path, offset, buffer_size, buffer, &ok);
				free(path);
				free(buffer);

				usleep(config_get_int_value(config, "RETARDO"));
				mdj_send(socket, RESULTADO_ESCRIBIR_MDJ, ok);
			break;

			case BORRAR:
				path = desempaquetar_string(msg);
				log_info(logger, "Recibi ordenes de DAM de borrar el archivo %s", path);
				log_info(logger, "Pude borrar el archivo %s", path); // Nunca falla porque DAM le habia pedido validarlo

				borrarArchivo(path, &ok);
				free(path);

				usleep(config_get_int_value(config, "RETARDO"));
				mdj_send(socket, RESULTADO_BORRAR, ok);
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
	char* dir_metadata = string_new();
	char* dir_archivos = string_new();
	char* dir_bloques = string_new();
	char* bin_metadata = string_new();
	char* bin_bitmap = string_new();
	FILE* f_metadata;

	if(_mkpath(config_get_string_value(config, "PUNTO_MONTAJE"), 0755) == -1){
		log_error(logger, "_mkpath");
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
		printf("[[FATAL ERROR]] FILESYSTEM NO ENCONTRADO (se creara uno nuevo)");
		f_metadata = fopen(bin_metadata, "wb+");
		config_metadata = config_create(bin_metadata);
		config_set_value(config_metadata, "TAMANIO_BLOQUES", "64");
		config_set_value(config_metadata, "CANTIDAD_BLOQUES", "256");
		config_set_value(config_metadata, "MAGIC_NUMBER", "FIFA");
		config_save(config_metadata);
	}
	else
		config_metadata = config_create(bin_metadata);

	fclose(f_metadata);
	log_info(logger, "Creado el archivo Metadata.bin");

	string_append(&bin_bitmap, dir_metadata);
	string_append(&bin_bitmap, "/Bitmap.bin");
	if((f_bitmap = fopen(bin_bitmap, "rb+")) == NULL){ // Si no existe el archivo bitmap
		f_bitmap = fopen(bin_bitmap, "wb+");
		char* bitarray_limpio_temp = calloc(1, ceiling(config_get_int_value(config_metadata, "CANTIDAD_BLOQUES"), 8));
		fwrite((void*) bitarray_limpio_temp, ceiling(config_get_int_value(config_metadata, "CANTIDAD_BLOQUES"), 8), 1, f_bitmap);
		fflush(f_bitmap);
		free(bitarray_limpio_temp);
	}
	fseek(f_bitmap, 0, SEEK_END);
	int file_size = ftell(f_bitmap);
	fseek(f_bitmap, 0, SEEK_SET);
	char* bitarray_str = (char*) mmap(NULL, file_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, fileno(f_bitmap), 0);
	if(bitarray_str == (char*) -1) {
		log_error(logger, "Fallo el mmap: %s", strerror(errno));
	}
	fread((void*) bitarray_str, sizeof(char), file_size, f_bitmap);
	bitmap = bitarray_create_with_mode(bitarray_str, file_size, MSB_FIRST);
	log_info(logger, "Creado el archivo Bitmap.bin");

	paths_estructuras[METADATA] = dir_metadata;
	paths_estructuras[ARCHIVOS] = dir_archivos;
	paths_estructuras[BLOQUES] = dir_bloques;

	free(bin_metadata);
	free(bin_bitmap);
}

static void _hardcodear_archivos(){ // PARA TESTEAR

	void __nuevo_escriptorio(char* ruta, char* contenido){

		int ___count_char_occurrences_in_string(const char* str, const char c){
			int i, ret = 0;
			for(i = 0; i<strlen(str); i++){
				if(str[i] == c) ret++;
			}
			return ret;
		}

		int ok, ok_validar, cant_lineas = ___count_char_occurrences_in_string(contenido, '\n');

		validarArchivo(ruta, &ok_validar);
		if(ok_validar == OK){
			log_error(logger, "%s - validar - %d", ruta, ok_validar);
			return;
		}

		crearArchivo(ruta, cant_lineas, &ok);
		if(ok != OK) {
			log_error(logger, "%s - crear - %d", ruta, ok);
			return;
		}

		void* buffer = malloc(strlen(contenido));
		memcpy(buffer, (void*) contenido, strlen(contenido));
		guardarDatos(ruta, 0, strlen(contenido), buffer, &ok);
		free(buffer);
		if(ok != OK) {
			log_error(logger, "%s - guardar - %d", ruta, ok);
			return;
		}
	}

	/*
	__nuevo_escriptorio("/scripts/cpu.escriptorio",
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"concentrar\n"
			"\n");

	__nuevo_escriptorio("/scripts/io_to_cpu.escriptorio",
			"crear /estadisticas/goleadores.txt 10\n"
			"abrir /estadisticas/goleadores.txt\n"
			"asignar /estadisticas/goleadores.txt 1 E. Gigliotti - Independiente - 12\n"
			"flush /estadisticas/goleadores.txt\n"
			"asignar /estadisticas/goleadores.txt 2 L. Lopez - Racing Club - 9\n"
			"asignar /estadisticas/goleadores.txt 3 L. Rodriguez - Atl. Tucuman - 7\n"
			"asignar /estadisticas/goleadores.txt 4 F. Barcelo - Patronato - 6\n"
			"asignar /estadisticas/goleadores.txt 5 M. Rojas - Defensa y Justicia - 6\n"
			"asignar /estadisticas/goleadores.txt 6 D. Cvitanich - Banfield - 5\n"
			"asignar /estadisticas/goleadores.txt 7 F. Zampedri - Rosario Central - 4\n"
			"asignar /estadisticas/goleadores.txt 8 N. Fernandez - Defensa y Justicia - 4\n"
			"asignar /estadisticas/goleadores.txt 9 N. Blandi - San Lorenzo - 4\n"
			"asignar /estadisticas/goleadores.txt 10 R. Aliendro - Atl. Tucuman - 4\n"
			"flush /estadisticas/goleadores.txt\n"
			"\n");

	*/
}

void validarArchivo(char* path, int* ok){

	int isDirectory(const char *path) {
	   struct stat statbuf;
	   if (stat(path, &statbuf) != 0)
	       return 0;
	   return S_ISDIR(statbuf.st_mode);
	}


	FILE* f;
	char* rutaFinal = string_new();
	string_append(&rutaFinal, paths_estructuras[ARCHIVOS]);
	string_append(&rutaFinal, path);

	if((f = fopen(rutaFinal, "rb")) != NULL && !isDirectory(rutaFinal)){ // Encontro el archivo, y no es un directorio
		log_info(logger, "Encontre el archivo %s", rutaFinal);
		fclose(f);
		*ok = OK;
	}
	else{
		log_info(logger, "No encontre el archivo %s", rutaFinal);
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
	int i, bloques_necesarios = (cant_lineas / config_get_int_value(config_metadata, "TAMANIO_BLOQUES")) + 1;
	t_list* lista_nro_bloques = list_create();
	char* rutaFinal = string_new();
	string_append(&rutaFinal, paths_estructuras[ARCHIVOS]);
	string_append(&rutaFinal, path);

	/* Busco en el bitmap la cantidad de bloques necesarios */
	for(i = 0; i < bitmap->size * 8 && lista_nro_bloques->elements_count < bloques_necesarios; i++){
		if(!bitarray_test_bit(bitmap, i)){ // Bloque disponible
			list_add(lista_nro_bloques, (void*) i);
			bitarray_set_bit(bitmap, i);
		}
	}

	if(lista_nro_bloques->elements_count < bloques_necesarios){ // Espacio insuficiente
		*ok = MDJ_ERROR_ESPACIO_INSUFICIENTE;
		free(rutaFinal);
		/* Limpio del bitmap los bloques que reserve */
		for(i = 0; i < list_size(lista_nro_bloques); i++){
			bitarray_clean_bit(bitmap, (int) list_get(lista_nro_bloques, i));
		}
		list_destroy(lista_nro_bloques);
		return;
	}

	/* OK, puedo crear el archivo */
	*ok = OK;
	_mkpath(rutaFinal, 0777); // Creo todos los directorios necesarios del path

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

		void* vacio = malloc(config_get_int_value(config_metadata, "TAMANIO_BLOQUES"));
		memset(vacio, 0, config_get_int_value(config_metadata, "TAMANIO_BLOQUES"));

		int cant_saltos_de_linea = min(cant_lineas - (config_get_int_value(config_metadata, "TAMANIO_BLOQUES") * i), config_get_int_value(config_metadata, "TAMANIO_BLOQUES"));
		void* salto_de_linea = malloc(cant_saltos_de_linea);
		int barra_ene = (int) '\n';
		memset(salto_de_linea, barra_ene, cant_saltos_de_linea);

		string_append(&ruta_bloque, paths_estructuras[BLOQUES]);
		string_append(&ruta_bloque, nro_bloque_str);
		string_append(&ruta_bloque, ".bin");
		archivo = fopen(ruta_bloque, "wb+");
		fwrite(vacio, config_get_int_value(config_metadata, "TAMANIO_BLOQUES"), 1, archivo);
		free(vacio);
		fflush(archivo);

		/* Escribo los \n necesarios */
		fseek(archivo, 0 ,SEEK_SET);
		fwrite(salto_de_linea, cant_saltos_de_linea, 1, archivo);
		free(salto_de_linea);
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
	int i = 0, indice_bloque_inicial;
	char** bloques_strings;
	t_config* config_archivo;

	// Retorna si el archivo se ha terminado
	bool _agregar_bloque_a_buffer(char* nro_bloque, int offset, int indice_bloque){
		bool ret_eof = false;

		char* ruta_bloque = string_new();
		string_append(&ruta_bloque, paths_estructuras[BLOQUES]);
		string_append(&ruta_bloque, nro_bloque);
		string_append(&ruta_bloque, ".bin");
		FILE* bloque = fopen(ruta_bloque, "rb");
		fseek(bloque, offset, SEEK_SET);
		free(ruta_bloque);

		int cant_bytes_a_leer = min(config_get_int_value(config_metadata, "TAMANIO_BLOQUES") - offset, bytes_restantes);
		/* Me fijo si es el ultimo bloque */
		if(split_cant_elem(bloques_strings) - 1 == indice_bloque){
			int tam_ultimo_bloque = config_get_int_value(config_archivo, "TAMANIO") -
					((indice_bloque_inicial + i) * config_get_int_value(config_metadata, "TAMANIO_BLOQUES"));
			ret_eof = tam_ultimo_bloque - offset <= cant_bytes_a_leer; // Llegue al EOF
			cant_bytes_a_leer = min(tam_ultimo_bloque - offset, cant_bytes_a_leer);
		}

		void* data = malloc(cant_bytes_a_leer);
		int bytes_leidos = fread(data, 1, cant_bytes_a_leer, bloque);
		memcpy((char*) (*ret_buffer) + (*ret_buffer_size), data, bytes_leidos);
		*ret_buffer_size += bytes_leidos;
		free(data);
		fclose(bloque);
		return ret_eof;
	}

	char* rutaFinal = string_new();
	string_append(&rutaFinal, paths_estructuras[ARCHIVOS]);
	string_append(&rutaFinal, path);
	config_archivo = config_create(rutaFinal);
	free(rutaFinal);

	bloques_strings = config_get_array_value(config_archivo, "BLOQUES");
	// Agrego el bloque inicial:
	indice_bloque_inicial = offset/config_get_int_value(config_metadata, "TAMANIO_BLOQUES");
	int offset_bloque_inicial = offset - (indice_bloque_inicial * config_get_int_value(config_metadata, "TAMANIO_BLOQUES"));
	bool eof = _agregar_bloque_a_buffer(bloques_strings[indice_bloque_inicial],  offset_bloque_inicial, indice_bloque_inicial);
	if(eof) // No tengo que ir a leer mas bloques
		bytes_restantes = 0;
	else
		bytes_restantes -= (config_get_int_value(config_metadata, "TAMANIO_BLOQUES") - offset_bloque_inicial);

	// Agrego el resto de bloques a abrir
	while(bytes_restantes > 0 && !eof){
		int indice = indice_bloque_inicial + ++i;
		log_debug(logger, "AGREGO BLOQUE INDICE: %d, NRO: %s | BYTES_RESTANTES: %d", indice, bloques_strings[indice], bytes_restantes);
		eof = _agregar_bloque_a_buffer(bloques_strings[indice], 0, indice);
		bytes_restantes -= config_get_int_value(config_metadata, "TAMANIO_BLOQUES");
	}

	config_destroy(config_archivo);
	split_liberar(bloques_strings);
}

void guardarDatos(char* path, int offset, int bytes_restantes, void* buffer, int* ok){
	char* bloques_string;
	int offset_buffer = 0;
	int tamanio_total;
	int nuevo_indice_bloque = 0;

	void _escribir_bloque(char* nro_bloque, int offset){
		char* ruta_bloque = string_new();
		string_append(&ruta_bloque, paths_estructuras[BLOQUES]);
		string_append(&ruta_bloque, nro_bloque);
		string_append(&ruta_bloque, ".bin");
		FILE* bloque = fopen(ruta_bloque, "rb+");
		fseek(bloque, offset, SEEK_SET);
		free(ruta_bloque);

		int cant_bytes_a_escribir = min(config_get_int_value(config_metadata, "TAMANIO_BLOQUES") - offset, bytes_restantes);
		tamanio_total = max(tamanio_total, offset + cant_bytes_a_escribir + (nuevo_indice_bloque * config_get_int_value(config_metadata, "TAMANIO_BLOQUES")));
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

		for(nro_bloque = 0;
			nro_bloque < bitmap->size * 8 && !(bloque_disponible = !(bitarray_test_bit(bitmap, nro_bloque)));
			nro_bloque++);

		if(!bloque_disponible){
			*ok = MDJ_ERROR_ESPACIO_INSUFICIENTE;
			free(ruta_bloque);
			return NULL;
		}
		bitarray_set_bit(bitmap, nro_bloque);
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

	void _borrar_bloques(char* bloques_str){

		void __borrar_bloque_de_fifa(char* nro_bloque){
			char* ruta_bloque = string_new();
			string_append(&ruta_bloque, paths_estructuras[BLOQUES]);
			string_append(&ruta_bloque, nro_bloque);
			string_append(&ruta_bloque, ".bin");
			remove(ruta_bloque);

			bitarray_clean_bit(bitmap, atoi(nro_bloque));

			free(ruta_bloque);
		}

		char** nros_bloques = string_get_string_as_array(bloques_str);
		string_iterate_lines(nros_bloques, __borrar_bloque_de_fifa);
		split_liberar(nros_bloques);
	}

	*ok = OK;

	char* rutaFinal = string_new();
	string_append(&rutaFinal, paths_estructuras[ARCHIVOS]);
	string_append(&rutaFinal, path);
	t_config* config_archivo = config_create(rutaFinal);


	tamanio_total = config_get_int_value(config_archivo, "TAMANIO");
	char** bloques_arr_strings = config_get_array_value(config_archivo, "BLOQUES");
	bloques_string = strdup(config_get_string_value(config_archivo, "BLOQUES"));
	int bloques_arr_strings_len = split_cant_elem(bloques_arr_strings);
	int indice_bloque_inicial = offset/config_get_int_value(config_metadata, "TAMANIO_BLOQUES");
	int offset_bloque_inicial = offset - (indice_bloque_inicial * config_get_int_value(config_metadata, "TAMANIO_BLOQUES"));

	if(bytes_restantes == 0){ // DAM me aviso que en este offset finaliza el archivo
		int i;

		/* Elimino los bloques sobrantes y actualizo el bitmap */
		for(i = bloques_arr_strings_len - 1; i > 0 && i > indice_bloque_inicial; i--){
			char* nro_bloque = bloques_arr_strings[i];

			char* ruta_bloque = string_new();
			string_append(&ruta_bloque, paths_estructuras[BLOQUES]);
			string_append(&ruta_bloque, nro_bloque);
			string_append(&ruta_bloque, ".bin");
			remove(ruta_bloque);
			free(ruta_bloque);

			bitarray_clean_bit(bitmap, atoi(nro_bloque));

			free(bloques_arr_strings[i]);
			bloques_arr_strings[i] = NULL;
		}

		bloques_arr_strings_len = split_cant_elem(bloques_arr_strings);

		/* Actualizo config */
		free(bloques_string);
		bloques_string = string_new();
		string_append(&bloques_string, "[");
		for(i = 0; i < bloques_arr_strings_len; i++){
			string_append(&bloques_string, bloques_arr_strings[i]);
			string_append(&bloques_string, ",");
		}
		bloques_string[strlen(bloques_string) - 1] = ']';
		config_set_value(config_archivo, "BLOQUES", bloques_string);
		free(bloques_string);

		tamanio_total = offset + nuevo_indice_bloque * config_get_int_value(config_metadata, "TAMANIO_BLOQUES");
		char* tamanio_str = string_itoa(tamanio_total);
		config_set_value(config_archivo, "TAMANIO", tamanio_str);
		config_save(config_archivo);
		free(tamanio_str);

		split_liberar(bloques_arr_strings);
		config_destroy(config_archivo);
		free(rutaFinal);
		return;
	}

	// Guardo lo del buffer, creando los bloques necesarios
	int bytes_guardados_bloque_actual, offset_bloque_actual;
	for(nuevo_indice_bloque = indice_bloque_inicial;
			bytes_restantes > 0;
			nuevo_indice_bloque++, bytes_restantes -= bytes_guardados_bloque_actual){

		if(nuevo_indice_bloque == indice_bloque_inicial){ // Primer bloque{
			bytes_guardados_bloque_actual = config_get_int_value(config_metadata, "TAMANIO_BLOQUES") - offset_bloque_inicial;
			offset_bloque_actual = offset_bloque_inicial;
		}
		else{
			bytes_guardados_bloque_actual = config_get_int_value(config_metadata, "TAMANIO_BLOQUES");
			offset_bloque_actual = 0;
		}

		if(nuevo_indice_bloque > bloques_arr_strings_len - 1){
			char* nro_bloque = _crear_bloque();
			if(*ok != OK){ // No hay espacio
				_borrar_bloques(bloques_string);
				remove(rutaFinal);

				free(bloques_string);
				free(rutaFinal);
				config_destroy(config_archivo);
				split_liberar(bloques_arr_strings);
				return;
			}

			_escribir_bloque(nro_bloque, offset_bloque_actual);
			free(nro_bloque);
		}
		else{
			_escribir_bloque(bloques_arr_strings[nuevo_indice_bloque], offset_bloque_actual);
		}
	}


	config_set_value(config_archivo, "BLOQUES", bloques_string);
	free(bloques_string);

	char* tamanio_str = string_itoa(tamanio_total);
	config_set_value(config_archivo, "TAMANIO", tamanio_str);
	config_save(config_archivo);
	free(tamanio_str);

	free(rutaFinal);
	split_liberar(bloques_arr_strings);
	config_destroy(config_archivo);
}

void borrarArchivo(char* path, int* ok){

	void _borrar_bloque(char* nro_bloque){
		char* ruta_bloque = string_new();
		string_append(&ruta_bloque, paths_estructuras[BLOQUES]);
		string_append(&ruta_bloque, nro_bloque);
		string_append(&ruta_bloque, ".bin");
		remove(ruta_bloque);

		bitarray_clean_bit(bitmap, atoi(nro_bloque));

		free(ruta_bloque);
	}

	*ok = OK;

	char* rutaFinal = string_new();
	string_append(&rutaFinal, paths_estructuras[ARCHIVOS]);
	string_append(&rutaFinal, path);
	t_config* config_archivo = config_create(rutaFinal);


	char** bloques_strings = config_get_array_value(config_archivo, "BLOQUES");
	string_iterate_lines(bloques_strings, _borrar_bloque);

	remove(rutaFinal);
	free(rutaFinal);

	config_destroy(config_archivo);
	split_liberar(bloques_strings);
}

void mdj_exit(){
	log_destroy(logger);
	config_destroy(config);
	free(bitmap->bitarray);
	bitarray_destroy(bitmap);
}








