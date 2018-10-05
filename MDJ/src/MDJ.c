#include "MDJ.h"

void mdj_enviar_datos_HARDCODEADO(int dam_socket, char* path, int offset, int size);

int main(void) {

	void config_create_fixed(){
		config = config_create(CONFIG_PATH);
		util_config_fix_comillas(&config, "PUNTO_MONTAJE");
	}

	config_create_fixed();
	mkdir(LOG_DIRECTORY_PATH,0777);
	logger = log_create(LOG_PATH, "MDJ", false, LOG_LEVEL_TRACE);

	//creo estructuras administrativas
	datosConfigMDJ[PUNTO_MONTAJE] = config_get_string_value(config, "PUNTO_MONTAJE");
	datosConfigMDJ[TAMANIO_BLOQUES] = config_get_string_value(config, "TAMANIO_BLOQUES");
	datosConfigMDJ[CANTIDAD_BLOQUES] = config_get_string_value(config, "CANTIDAD_BLOQUES");
	retardo_microsegundos = config_get_int_value(config, "RETARDO") * 1000;
	crearEstructuras();

	if((listenning_socket = socket_create_listener(IP, config_get_string_value(config, "PUERTO"))) == -1){
		log_error(logger, "No pude crear el socket de escucha");
		mdj_exit();
		exit(EXIT_FAILURE);
	}
	log_info(logger,"Escucho en el socket %d", listenning_socket);

	socket_start_listening_select(listenning_socket, mdj_manejador_de_eventos);

	mdj_exit();
	return EXIT_SUCCESS;
}

int mdj_send(int socket, e_tipo_msg tipo_msg, ...){
	t_msg* mensaje_a_enviar;
	int ret, ok;
	char* str;

 	va_list arguments;
	va_start(arguments, tipo_msg);

	switch(tipo_msg){
		case RESULTADO_VALIDAR:
			ok = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_int(ok);
		break;

		case RESULTADO_GET_MDJ:
			str = va_arg(arguments, char*);
			mensaje_a_enviar = empaquetar_string(str);
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
	int ok, offset, size;

	if(msg->header->emisor == DAM){
		switch(msg->header->tipo_mensaje){
			case CONEXION:
				log_info(logger, "Se conecto un nuevo hilo de DAM");
			break;

			case DESCONEXION:
				log_info(logger, "Se desconecto un hilo de DAM");
				return -1;
			break;

			case VALIDAR:
				log_info(logger, "Recibi ordenes de DAM de validar un archivo");
				path = desempaquetar_string(msg);

				/* Valido que el archivo exista y se lo comunico a diego */
				ok = validarArchivo(path);
				free(path);

				usleep(retardo_microsegundos);
				mdj_send(socket, RESULTADO_VALIDAR, ok);
			break;

			case CREAR_MDJ:
				log_info(logger, "Recibi ordenes de DAM de crear un archivo");
			break;

			case GET_MDJ:
				log_info(logger, "Recibi ordenes de DAM de obtener datos");
				desempaquetar_get_mdj(msg, &path, &offset, &size);

				mdj_enviar_datos_HARDCODEADO(socket, path, offset, size);

				free(path);
			break;

			case ESCRIBIR_MDJ:
				log_info(logger, "Recibi ordenes de DAM de escribir en un archivo");
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
	char* puntoMontaje = string_new();
	char* metadata = string_new();
	char* archivos = string_new();
	char* bloques = string_new();
	char* metadataBin = string_new();
	char* bitmapBin = string_new();

	string_append(&puntoMontaje, "..");
	string_append(&puntoMontaje, datosConfigMDJ[PUNTO_MONTAJE]);
	mkdir(puntoMontaje,0777);

	string_append(&metadata, puntoMontaje);
	string_append(&metadata, "Metadata");
	mkdir(metadata,0777);
	log_info(logger, "Creada carpeta Metadata");

	string_append(&archivos, puntoMontaje);
	string_append(&archivos, "Archivos");
	mkdir(archivos,0777);
	log_info(logger, "Creada carpeta Archivos");

	string_append(&bloques, puntoMontaje);
	string_append(&bloques, "Bloques");
	mkdir(bloques,0777);
	log_info(logger, "Creada carpeta Bloques");

	string_append(&metadataBin, metadata);
	string_append(&metadataBin, "/Metadata.bin");
	FILE *archivoMetadataBin = fopen(metadataBin,"wb+");
	fwrite("TAMANIO_BLOQUES=\n",1,17,archivoMetadataBin);
	//fwrite(datosConfigMDJ[TAMANIO_BLOQUES],1,strlen(datosConfigMDJ[TAMANIO_BLOQUES])*sizeof(char*),archivoMetadataBin);


	//fputc('\n', archivoMetadataBin);
	fwrite("CANTIDAD_BLOQUES=",1,17,archivoMetadataBin);
	fwrite(datosConfigMDJ[CANTIDAD_BLOQUES],1,strlen(datosConfigMDJ[CANTIDAD_BLOQUES])*sizeof(char*),archivoMetadataBin);
	//fseek(archivoMetadataBin,17,0);

	fclose(archivoMetadataBin);
	log_info(logger, "Creado el archivo Metadata.bin");

	string_append(&bitmapBin, metadata);
	string_append(&bitmapBin, "/Bitmap.bin");
	FILE *bitmap = fopen(bitmapBin,"wb+");
	fclose(bitmap);
	log_info(logger, "Creado el archivo Bitmap.bin");
	puts("Creadas las estructuras administrativas");

	free(puntoMontaje);
	free(metadata);
	free(archivos);
	free(bloques);
	free(metadataBin);
	free(bitmapBin);

}

int validarArchivo(char* path){ // Despues rehacer bien esto (sin archivos de texto)
	FILE* f;
	if((f = fopen(path, "r")) != NULL){ // Encontro el archivo de texto
		fclose(f);
		return OK;
	}
	return ERROR_ABRIR_PATH_INEXISTENTE;
}

void crearArchivo(char* path,int cantidadLineas){
	char* rutaFinal = string_new();
	string_append(&rutaFinal, "../");
	string_append(&rutaFinal, path);

	if(validarArchivo(rutaFinal)){
		puts("Fallo al intentar crear un archivo existente");
		log_error(logger, "Fallo al intentar crear un archivo existente");
	}else{
		FILE *archivo = fopen(rutaFinal,"a");
		int n;
		for(n = 0; n < cantidadLineas -1; n++){
			fwrite("\n",1,1,archivo);
		}
		fclose(archivo);

		//PENDIENTE actualizar bitmap

		puts("Archivo creado correctamente");
		log_info(logger, "Archivo creado correctamente");
	}
	free(rutaFinal);
}

void borrarArchivo(char* path){
	char* rutaFinal = string_new();
	string_append(&rutaFinal, "../");
	string_append(&rutaFinal, path);

	if(validarArchivo(rutaFinal)){
		remove(rutaFinal);

		//PENDIENTE actualizar bitmap

		puts("Archivo borrado correctamente");
		log_info(logger, "Archivo borrado correctamente");
	}else{
		puts("El archivo no puede ser borrado ya que no existe");
		log_error(logger,"El archivo no puede ser borrado ya que no existe");
	}
	free(rutaFinal);
}

void mdj_exit(){
	log_destroy(logger);
	config_destroy(config);
}

void mdj_enviar_datos_HARDCODEADO(int dam_socket, char* path, int offset, int size){
	FILE* f = fopen(path, "r");
	char* buffer = malloc(size + 1);

	if(f == NULL){
		free(buffer);
		return;
	}

	fseek(f, offset, SEEK_SET);
	int caracteres_leidos = fread((void*) buffer, sizeof(char), size, f);
	memset(buffer + caracteres_leidos, 0, size - caracteres_leidos);

	usleep(retardo_microsegundos);
	log_info(logger, "Le envio %s a DAM", buffer);
	mdj_send(dam_socket, RESULTADO_GET_MDJ, buffer);

	free(buffer);
	fclose(f);
}








