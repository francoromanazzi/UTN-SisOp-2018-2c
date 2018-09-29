1#include "DAM.h"

int main(void) {
	if(dam_initialize() == -1){
		dam_exit(); return EXIT_FAILURE;
	}

	/* Me quedo esperando conexiones de CPU */
	while(1){
		int nuevo_cliente = socket_aceptar_conexion(listenning_socket);
		if( !dam_crear_nuevo_hilo(nuevo_cliente)){
			log_error(logger, "No pude crear un nuevo hilo para atender una nueva conexion");
			dam_exit();
			return EXIT_FAILURE;
		}
		log_info(logger, "Creo un nuevo hilo para atender los pedidos de un nuevo cliente");
	}

	dam_exit();
	return EXIT_SUCCESS;
}

int dam_initialize(){
	config_create_fixed("/home/utnso/workspace/tp-2018-2c-RegorDTs/configs/DAM.txt");
	mkdir("/home/utnso/workspace/tp-2018-2c-RegorDTs/logs",0777);
	logger = log_create("/home/utnso/workspace/tp-2018-2c-RegorDTs/logs/DAM.log", "DAM", false, LOG_LEVEL_TRACE);

	/* Me conecto a S-AFA, FM9 y MDJ */
	if(!dam_connect_to_safa()){
		log_error(logger, "No pude conectarme a SAFA"); return -1;
	}
	else log_info(logger, "Me conecte a SAFA");

	if(!dam_connect_to_mdj()){
		log_error(logger, "No pude conectarme a MDJ"); return -1;
	}
	else log_info(logger, "Me conecte a MDJ");

	if(!dam_connect_to_fm9()){
		log_error(logger, "No pude conectarme a FM9"); return -1;
	}
	else log_info(logger, "Me conecte a FM9");

	if((listenning_socket = socket_create_listener(IP, config_get_string_value(config, "PUERTO"))) == -1){
		log_error(logger, "No pude crear el socket de escucha");
		return -1;
	}
	log_info(logger, "Escucho en el socket %d", listenning_socket);
	return 1;
}

int dam_send(int socket, e_tipo_msg tipo_msg, ...){
	t_msg* mensaje_a_enviar;
	int ret;

	int ok;
	unsigned int id;
	char* path;
	int base;

	va_list arguments;
	va_start(arguments, tipo_msg);

	switch(tipo_msg){
		case CONEXION:
			mensaje_a_enviar = msg_create(DAM, CONEXION, (void**) 1, sizeof(int));
		break;

		case RESULTADO_ABRIR:
			ok = va_arg(arguments, int);
			id = va_arg(arguments, unsigned int);
			path = va_arg(arguments, char*);
			base = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_resultado_abrir(ok, id, path, base);
		break;
	}
	mensaje_a_enviar->header->emisor = DAM;
	mensaje_a_enviar->header->tipo_mensaje = tipo_msg;
	ret = msg_send(socket, *mensaje_a_enviar);
	msg_free(&mensaje_a_enviar);

	va_end(arguments);
	return ret;
}

bool dam_crear_nuevo_hilo(int socket_nuevo_cliente){
	pthread_t nuevo_thread;
	if(pthread_create( &nuevo_thread, NULL, (void*) dam_nuevo_cliente_iniciar, (void*) socket_nuevo_cliente) ){
		return false;
	}
	pthread_detach(nuevo_thread);
	return true;
}

void dam_nuevo_cliente_iniciar(int socket){
	while(1){
		t_msg* nuevo_mensaje = malloc(sizeof(t_msg));
		if(msg_await(socket, nuevo_mensaje) == -1){
			log_info(logger, "Cierro el hilo que atendia a este cliente");
			msg_free(&nuevo_mensaje);
			pthread_exit();
			return;
		}
		if(dam_manejar_nuevo_mensaje(socket, nuevo_mensaje) == -1){
			log_info(logger, "Cierro el hilo que atendia a este cliente");
			msg_free(&nuevo_mensaje);
			pthread_exit();
			return;
		}
		msg_free(&nuevo_mensaje);
	} // Fin while(1)
}

int dam_manejar_nuevo_mensaje(int socket, t_msg* msg){
	log_info(logger, "EVENTO: Emisor: %d, Tipo: %d, Tamanio: %d, Mensaje: %s",msg->header->emisor,msg->header->tipo_mensaje,msg->header->payload_size,(char*) msg->payload);

	int ok,resultadoManejar;
	unsigned int id;
	char* path;
	int base;

	if(msg->header->emisor == CPU){
		switch(msg->header->tipo_mensaje){
			case CONEXION:
				log_info(logger, "Se conecto CPU");
			break;

			case DESCONEXION:
				log_info(logger, "Se desconecto CPU");
				resultadoManejar = -1;
			break;

			case ABRIR:
				desempaquetar_abrir(msg,&path,&id);
				log_info(logger, "Ehhh, voy a busar %s para %d", path, id);

				// TODO: solicitud de archivo desde MDJ
				//que pasa si no lo tiene?
				t_msg* mensajeAbrir = msg_create(DAM,ABRIR,(void**) path, strlen(path));
				if((msg_send(mdj_socket, mensajeAbrir)) == -1){
					log_info(logger, "Problema con solicitud de archivo");
					msg_free(&mensajeAbrir);
					resultadoManejar = -1;
					break;
				}

				resultadoManejar = 1;
				break;


				//TODO: recibir datos de FM9 para G.DT

				/* Le mando a SAFA el resultado de abrir el archivo */
				/* HARDCODEO*/ ok = 1; base = 21198;
				dam_send(safa_socket, RESULTADO_ABRIR, ok, id, path, base);
				free(path);
				resultadoManejar = 1;
			break;

			case FLUSH:
				log_info(logger, "Iniciando operacion FLUSH");

				//si es OK
				resultadoManejar = 1;
			break;

			default:
				log_info(logger, "No entendi el mensaje de CPU");
				//no deberia ser 1, sino otro numero, para identificar el "ERROR"
				resultadoManejar = 1;

		}
	}
	else if(msg->header->emisor == MDJ){
				//TODO: recibir archivo de MDJ
				//TODO: cargar en FM9 el archivo
				//hace la recepción del resto del archivo desde MDJ y envia To.do el archivo a FM9
		t_msg* avisoApertura = msg_create(DAM,ABRIR,path, strlen(path));
		if (msg_send(SAFA, avisoApertura) != -1){
			resultadoManejar = enviarArchivo((void**) msg->payload, msg->header->payload_size);
		}else{
			resultadoManejar = -1;
		}

	}
	else if(msg->header->emisor == DESCONOCIDO){
		log_info(logger, "Me hablo alguien desconocido");
		//si esto es OK
		resultadoManejar = 1;
	}

	return resultadoManejar;
}

int enviarArchivo(void** payload, int tamPayload){
	int resultadoTransferencia = 1;

	if(tamPayload != -1){

		t_msg* mensajeResultado = msg_create(DAM,RESULTADO_ABRIR,payload, tamPayload);

		if(msg_send(fm9_socket, mensajeResultado) == -1){
			log_info(logger, "Problemas al enviar linea de archivo a FM9");
			msg_free(mensajeResultado);
			resultadoTransferencia = -1;
		}

		if (msg_await(MDJ,mensajeResultado) == -1){
			log_info(logger, "Problema al recibir linea de archivo de MDJ");
			resultadoTransferencia = -1;
		}else{
			resultadoTransferencia = enviarArchivo(mensajeResultado->payload, mensajeResultado->header->payload_size);
		}

		msg_free(mensajeResultado);
	}
	return resultadoTransferencia;
}

int dam_connect_to_safa(){
	safa_socket = socket_connect_to_server(config_get_string_value(config, "IP_SAFA"), config_get_string_value(config, "PUERTO_SAFA"));
	int resultado_send = dam_send(safa_socket, CONEXION, NULL);

	dam_crear_nuevo_hilo(safa_socket);

	return safa_socket > 0 && resultado_send > 0;
}

int dam_connect_to_mdj(){
	mdj_socket  = socket_connect_to_server(config_get_string_value(config, "IP_MDJ"), config_get_string_value(config, "PUERTO_MDJ"));
	int resultado_send = dam_send(mdj_socket, CONEXION, NULL);

	dam_crear_nuevo_hilo(mdj_socket);

	return mdj_socket > 0 && resultado_send > 0;
}

int dam_connect_to_fm9(){
	fm9_socket  = socket_connect_to_server(config_get_string_value(config, "IP_FM9"), config_get_string_value(config, "PUERTO_FM9"));
	int resultado_send = dam_send(fm9_socket, CONEXION, NULL);

	dam_crear_nuevo_hilo(fm9_socket);

	return fm9_socket > 0 && resultado_send > 0;
}

void config_create_fixed(char* path){
	config = config_create(path);
	util_config_fix_comillas(&config, "IP_SAFA");
	util_config_fix_comillas(&config, "IP_FM9");
	util_config_fix_comillas(&config, "IP_MDJ");
}

void dam_exit(){
	close(safa_socket);
	close(fm9_socket);
	close(mdj_socket);
	close(listenning_socket);
	log_destroy(logger);
	config_destroy(config);
}
