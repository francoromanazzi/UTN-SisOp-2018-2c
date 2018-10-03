#include "MDJ.h"

int main(void) {
	config_create_fixed("/home/utnso/workspace/tp-2018-2c-RegorDTs/configs/MDJ.txt");
	mkdir("/home/utnso/workspace/tp-2018-2c-RegorDTs/logs",0777);
	logger = log_create("/home/utnso/workspace/tp-2018-2c-RegorDTs/logs/MDJ.log", "MDJ", false, LOG_LEVEL_TRACE);

	//creo estructuras administrativas
	datosConfigMDJ[PUNTO_MONTAJE] = config_get_string_value(config, "PUNTO_MONTAJE");
	datosConfigMDJ[TAMANIO_BLOQUES] = config_get_string_value(config, "TAMANIO_BLOQUES");
	datosConfigMDJ[CANTIDAD_BLOQUES] = config_get_string_value(config, "CANTIDAD_BLOQUES");
	crearEstructuras();

	if((listenning_socket = socket_create_listener(IP, config_get_string_value(config, "PUERTO"))) == -1){
		log_error(logger, "No pude crear el socket de escucha");
		mdj_exit();
		exit(EXIT_FAILURE);
	}
	log_info(logger,"Escucho en el socket %d", listenning_socket);
	dam_socket = socket_aceptar_conexion(listenning_socket);
	log_info(logger,"Se me conecto DAM, en el socket %d", dam_socket);

	while(1)
		mdj_esperar_ordenes_dam();

	mdj_exit();
	return EXIT_SUCCESS;
}

void mdj_esperar_ordenes_dam(){
	t_msg* msg = malloc(sizeof(t_msg));
	msg_await(dam_socket, msg);
	if(msg->header->tipo_mensaje == VALIDAR){
		log_info(logger, "Recibi ordenes de DAM de validar un archivo");
		char* path_a_validar = desempaquetar_string(msg);
		int archivo_existe = 1;

		// TODO: Validar que el archivo exista

		/* Le comunico a Diego el resultado (1->OK, 0->NO_OK) */
		t_msg* mensaje_a_enviar = msg_create(MDJ, RESULTADO_VALIDAR, (void**) archivo_existe, sizeof(int));
		int resultado_send = msg_send(dam_socket, *mensaje_a_enviar);
		msg_free(&mensaje_a_enviar);

		free(path_a_validar);
		msg_free(&msg);
	}
	else if(msg->header->tipo_mensaje == CREAR){
		log_info(logger, "Recibi ordenes de DAM de crear un archivo");
		msg_free(&msg);
	}
	else if(msg->header->tipo_mensaje == GET){
		log_info(logger, "Recibi ordenes de DAM de obtener datos");
		msg_free(&msg);
	}
	else if(msg->header->tipo_mensaje == ESCRIBIR){
		log_info(logger, "Recibi ordenes de DAM de guardar datos");
		msg_free(&msg);
	}
	else if(msg->header->tipo_mensaje == DESCONEXION){
		log_info(logger, "Se desconecto DAM");
		msg_free(&msg);
	}
	else{
		log_error(logger, "No entendi el mensaje de DAM");
		msg_free(&msg);
	}
}

void config_create_fixed(char* path){
	config = config_create(path);
	util_config_fix_comillas(&config, "PUNTO_MONTAJE");
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

bool validarArchivo(char* path){
	return !(fopen(path,"r") == NULL);
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
