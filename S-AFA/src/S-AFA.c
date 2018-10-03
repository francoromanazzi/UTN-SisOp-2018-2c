 #include "S-AFA.h"

int main(void){
	if(safa_initialize() == -1){
		exit(EXIT_FAILURE);
	}

	/* Creo el socket de escucha y comienzo el select */
	if((listening_socket = socket_create_listener(IP, safa_config_get_string_value("PUERTO"))) == -1){
		log_error(logger, "[S-AFA] No pude crear el socket de escucha");
		safa_exit();
		exit(EXIT_FAILURE);
	}
	log_info(logger, "[S-AFA] Escucho en el socket %d",listening_socket);

	socket_start_listening_select(listening_socket, safa_manejador_de_eventos);

	safa_exit();
	return EXIT_SUCCESS;
}

int safa_initialize(){

	safa_protocol_initialize();
	safa_util_initialize();
	metricas_initialize();

	estado_operatorio = false;
	cpu_conectado = false;
	dam_conectado = false;
	op_dummy_en_progreso = false;

	cpu_conexiones = list_create();
	pthread_mutex_init(&sem_mutex_cpu_conexiones, NULL);

	/* Creo el hilo inotify, que se encarga de toodo lo relacionado al archivo de config */
	pthread_t thread_safa_config_inotify;
	if(pthread_create( &thread_safa_config_inotify, NULL, (void*) safa_config_inotify_iniciar, NULL) ){
		log_error(logger,"[S-AFA] No pude crear el hilo inotify");
		return -1;
	}
	log_info(logger, "[S-AFA] Creo el hilo inotify");
	pthread_detach(thread_safa_config_inotify);

	return 1;
}

void safa_iniciar_estado_operatorio(){
	estado_operatorio = true;

	cola_new = list_create();
	cola_ready = list_create(); pthread_mutex_init(&sem_mutex_cola_ready, NULL);
	cola_exec = list_create();
	cola_block = list_create(); list_add(cola_block, dtb_create_dummy());
	cola_exit = list_create(); pthread_mutex_init(&sem_mutex_cola_exit, NULL);

	/* Creo el hilo consola */
	pthread_t thread_consola;
	if(pthread_create( &thread_consola, NULL, (void*) consola_iniciar, NULL) ){
		log_error(logger,"[S-AFA] No pude crear el hilo para la consola");
		safa_exit();
		exit(EXIT_FAILURE);
	}
	log_info(logger, "[S-AFA] Creo el hilo para la consola");
	pthread_detach(thread_consola);

	/* Creo el hilo PLP */
	pthread_t thread_plp;
	if(pthread_create( &thread_plp, NULL, (void*) plp_iniciar, NULL) ){
		log_error(logger,"[S-AFA] No pude crear el hilo PLP");
		exit(EXIT_FAILURE);
	}
	log_info(logger, "[S-AFA] Creo el hilo PLP");
	pthread_detach(thread_plp);

	/* Creo el hilo PCP */
	pthread_t thread_pcp;
	if(pthread_create( &thread_pcp, NULL, (void*) pcp_iniciar, NULL) ){
		log_error(logger,"[S-AFA] No pude crear el hilo PCP");
		exit(EXIT_FAILURE);
	}
	log_info(logger, "[S-AFA] Creo el hilo PCP");
	pthread_detach(thread_pcp);
}

int safa_send(int socket, e_tipo_msg tipo_msg, ...){
	t_msg* mensaje_a_enviar;
	int ret;
 	t_dtb* dtb;
 	va_list arguments;
	va_start(arguments, tipo_msg);

 	switch(tipo_msg){
		case HANDSHAKE: // A CPU
			mensaje_a_enviar = msg_create(SAFA, HANDSHAKE, (void**) 1, sizeof(int)); // Una formalidad, no tiene info relevante
		break;

 		case EXEC: // A CPU
			dtb = va_arg(arguments, t_dtb*);
			mensaje_a_enviar = empaquetar_dtb(dtb);
		break;
	}

	mensaje_a_enviar->header->emisor = SAFA;
	mensaje_a_enviar->header->tipo_mensaje = tipo_msg;
	ret = msg_send(socket, *mensaje_a_enviar);
	msg_free(&mensaje_a_enviar);
 	va_end(arguments);
	return ret;
}

int safa_manejador_de_eventos(int socket, t_msg* msg){
	log_info(logger, "[S-AFA] EVENTO: Emisor: %d, Tipo: %d, Tamanio: %d, Mensaje: %s",msg->header->emisor,msg->header->tipo_mensaje,msg->header->payload_size,(char*) msg->payload);

	void* data;
	struct timespec time;
	unsigned int id;

	if(msg->header->emisor == DAM){
		switch(msg->header->tipo_mensaje){
			case CONEXION:
				log_info(logger, "[S-AFA] Se conecto DAM");
				dam_conectado = true;
				dam_socket = socket;

				if(!estado_operatorio && cpu_conectado){
					safa_iniciar_estado_operatorio();
				}
			break;

			case DESCONEXION:
				log_error(logger, "[S-AFA] Se desconecto DAM");
				return -1;
			break;

			case RESULTADO_ABRIR:
				log_info(logger, "[S-AFA] Recibi el resultado de cargar algo en memoria");
				data = malloc(msg->header->payload_size);
				memcpy(data, msg->payload, msg->header->payload_size);
				safa_protocol_encolar_msg_y_avisar(S_AFA, PLP, RESULTADO_ABRIR_DAM, data);
			break;

			default:
				log_info(logger, "[S-AFA] No entendi el mensaje de DAM");
			break;
		}
	}
	else if(msg->header->emisor == CPU){
		switch(msg->header->tipo_mensaje){
			case CONEXION:
				log_info(logger, "[S-AFA] Se conecto un CPU");
				cpu_conectado = true;

				conexion_cpu_add_new(socket); // Agrego el nuevo socket a la lista de CPUs

				safa_send(socket, HANDSHAKE);

				safa_protocol_encolar_msg_y_avisar(S_AFA, PCP, NUEVO_CPU_DISPONIBLE);

				if(!estado_operatorio && dam_conectado){
					safa_iniciar_estado_operatorio();
				}
			break;

			case DESCONEXION:
				log_info(logger, "[S-AFA] Se desconecto un CPU");
				conexion_cpu_disconnect(socket);

				return -1;
			break;

			case TIEMPO_RESPUESTA:
				desempaquetar_tiempo_respuesta(msg, &id, &time);
				metricas_tiempo_add_finish(id, time);
			break;

			case BLOCK:
				conexion_cpu_set_active(socket); // Esta CPU es seleccionable de nuevo
				safa_protocol_encolar_msg_y_avisar(S_AFA, PCP, BLOCK_DTB, desempaquetar_dtb(msg));
			break;

			case READY:
				conexion_cpu_set_active(socket); // Esta CPU es seleccionable de nuevo
				safa_protocol_encolar_msg_y_avisar(S_AFA, PCP, READY_DTB, desempaquetar_dtb(msg));
			break;

			case EXIT:
				conexion_cpu_set_active(socket); // Esta CPU es seleccionable de nuevo
				safa_protocol_encolar_msg_y_avisar(S_AFA, PCP, EXIT_DTB, desempaquetar_dtb(msg));
			break;

			default:
				log_info(logger, "[S-AFA] No entendi el mensaje de CPU");
		}
	}
	else if(msg->header->emisor == DESCONOCIDO){
		log_info(logger, "[S-AFA] Me hablo alguien desconocido");
	}
	return 1;
}

void safa_exit(){
	close(listening_socket);
	close(dam_socket);
	list_destroy_and_destroy_elements(cpu_conexiones, free);
	log_destroy(logger);
	config_destroy(config);
}

