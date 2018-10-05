#include "FM9.h"

int main(void) {
	if(fm9_initialize() == -1){
		fm9_exit(); return EXIT_FAILURE;
	}

	while(1){
		int nuevo_cliente = socket_aceptar_conexion(listening_socket);
		if( !fm9_crear_nuevo_hilo(nuevo_cliente)){
			log_error(logger, "No pude crear un nuevo hilo para atender una nueva conexion");
			fm9_exit();
			return EXIT_FAILURE;
		}
		log_info(logger, "Creo un nuevo hilo para atender los pedidos de un nuevo cliente");
	}

	fm9_exit();
	return EXIT_SUCCESS;
}

int fm9_initialize(){
	config = config_create(CONFIG_PATH);
	mkdir(LOG_DIRECTORY_PATH,0777);
	logger = log_create(LOG_PATH, "FM9", false, LOG_LEVEL_TRACE);

	modo=config_get_string_value(config, "MODO");
	tamanio = config_get_int_value(config, "TAMANIO");
	max_linea = config_get_int_value(config, "MAX_LINEA");
	tam_pagina = config_get_int_value(config, "TAM_PAGINA");
	log_info(logger,"Se realiza la inicializacion del Storage");
	marca=0;
	storage = calloc(1, tamanio);

	if(pthread_create(&thread_consola,NULL,(void*) fm9_consola_init,NULL)){
		log_error(logger,"No se pudo crear el hilo para la consola");
		return -1;
	}
	log_info(logger,"Se creo el hilo para la consola");

	if((listening_socket = socket_create_listener(IP, config_get_string_value(config, "PUERTO"))) == -1){
		log_error(logger,"No se pudo crear socket de escucha");
		return -1;
	}
	log_info(logger, "Comienzo a escuchar  por el socket %d", listening_socket);
	return 1;
}

int fm9_send(int socket, e_tipo_msg tipo_msg, ...){
	t_msg* mensaje_a_enviar;
	int ret, ok;
	char* str;

 	va_list arguments;
	va_start(arguments, tipo_msg);

	switch(tipo_msg){
		case RESULTADO_CREAR_FM9:
			ok = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_int(ok);
		break;

		case RESULTADO_GET_FM9:
			str = va_arg(arguments, char*);
			mensaje_a_enviar = empaquetar_string(str);
		break;

		case RESULTADO_ESCRIBIR_FM9:
			ok = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_int(ok);
		break;

	}
	mensaje_a_enviar->header->emisor = FM9;
	mensaje_a_enviar->header->tipo_mensaje = tipo_msg;
	ret = msg_send(socket, *mensaje_a_enviar);
	msg_free(&mensaje_a_enviar);
	va_end(arguments);
	return ret;
}

bool fm9_crear_nuevo_hilo(int socket_nuevo_cliente){
	pthread_t nuevo_thread;
	if(pthread_create( &nuevo_thread, NULL, (void*) fm9_nuevo_cliente_iniciar, (void*) socket_nuevo_cliente) ){
		return false;
	}
	pthread_detach(nuevo_thread);
	return true;
}

void fm9_nuevo_cliente_iniciar(int socket){
	while(1){
		t_msg* nuevo_mensaje = malloc(sizeof(t_msg));
		if(msg_await(socket, nuevo_mensaje) == -1 || fm9_manejar_nuevo_mensaje(socket, nuevo_mensaje) == -1){
			log_info(logger, "Cierro el hilo que atendia a este cliente");
			free(nuevo_mensaje);
			close(socket);
			pthread_exit(NULL);
			return;
		}
		msg_free(&nuevo_mensaje);
	} // Fin while(1)
}

int fm9_manejar_nuevo_mensaje(int socket, t_msg* msg){
	log_info(logger, "EVENTO: Emisor: %d, Tipo: %d, Tamanio: %d",msg->header->emisor,msg->header->tipo_mensaje,msg->header->payload_size);

	int base, offset;
	char* datos;

	if(msg->header->emisor == DAM){
		switch(msg->header->tipo_mensaje){
			case CONEXION:
				log_info(logger,"Se me conecto DAM");
			break;

			case DESCONEXION:
				log_info(logger,"Se me conecto DAM");
				return -1;
			break;

			case CREAR_FM9:
				log_info(logger,"DAM me pidio reservar memoria para un nuevo archivo");
				fm9_send(socket, RESULTADO_CREAR_FM9, marca);
			break;

			case ESCRIBIR_FM9:
				log_info(logger,"DAM me pidio la operacion ESCRIBIR");
				desempaquetar_escribir_fm9(msg, &base, &offset, &datos);

				fm9_storage_escribir(base, offset, datos);
				marca++;
				fm9_send(socket, RESULTADO_ESCRIBIR_FM9, OK); // El OK esta hardcodeado, puede no haber espacio
				//escribirEnMemoria(largo,datos);
			break;

			case GET_FM9:
				log_info(logger,"DAM me pidio la operacion GET");
				/*
				buscar en la memoria y armarlo para en enviar al DAM
				*/
				fm9_send(socket, RESULTADO_GET_FM9, (void*)"datosApasaralDAM");
			break;

			default:
				log_info(logger,"No entendi el mensaje de DAM");
		}

	}
	else if(msg->header->emisor == CPU){
		switch(msg->header->tipo_mensaje){
			case CONEXION:
				log_info(logger,"Se me conecto CPU");
			break;

			case DESCONEXION:
				log_info(logger,"Se me desconecto CPU");
				return -1;
			break;

			case ESCRIBIR_FM9:
				log_info(logger,"CPU me pidio la operacion ESCRIBIR (asignar)");
				desempaquetar_escribir_fm9(msg, &base, &offset, &datos);

				fm9_storage_escribir(base, offset, datos);
				fm9_send(socket, RESULTADO_ESCRIBIR_FM9, OK); // El OK esta hardcodeado, puede no haber espacio
				//escribirEnMemoria(largo,datos);
			break;

			case GET_FM9:
				log_info(logger,"CPU me pidio la operacion GET");

				int base, offset;
				desempaquetar_get(msg, &base, &offset);

				datos = fm9_storage_leer(offset);
				fm9_send(socket, RESULTADO_GET_FM9, (void*) datos); // Hardcodeado
				free(datos);
			break;

			default:
				log_info(logger,"No entendi el mensaje de CPU");
		}
	}
	else if(msg->header->emisor == DESCONOCIDO){
		log_info(logger, "Me hablo alguien desconocido");
	}
	return 1;
}

int escribirEnMemoria(int tam,char* contenido){

	memcpy(storage+marca,contenido,tam);

	marca=marca+tam;
	return tam;
}

void fm9_storage_escribir(int base, int offset, char* str){
	int direccion_fisica = base + offset;
	log_info(logger, "Escribo %s en %d", str, direccion_fisica);
	void* lugar_a_escribir = (void*) (storage + (direccion_fisica*max_linea));

	memcpy(lugar_a_escribir, (void*) str, strlen(str));
	memset(lugar_a_escribir + strlen(str), 0, sizeof(char));
	free(str);
}

char* fm9_storage_leer(int direccion_fisica){
	char* ret = malloc(max_linea);
	memcpy((void*) ret, (void*) (storage + (direccion_fisica*max_linea)), max_linea);
	return ret;
}

int fm9_storage_get_marca(){
	return marca < (tamanio/max_linea) ? marca : ERROR_ABRIR_ESPACIO_INSUFICIENTE_FM9;
}

void fm9_exit(){
	free(storage);
	close(listening_socket);
	log_destroy(logger);
	config_destroy(config);
}
