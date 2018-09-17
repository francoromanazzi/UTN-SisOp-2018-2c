#include "S-AFA.h"

int main(void){
	safa_initialize();

	/* Empiezo en estado corrupto */
	/* Como salgo? Conexion con DAM y por lo menos un CPU */
	if((listening_socket = socket_create_listener(IP, config_get_string_value(config, "PUERTO"))) == -1){
		log_error(logger, "No pude crear el socket de escucha");
		safa_exit();
		exit(EXIT_FAILURE);
	}
	log_info(logger, "Escucho en el socket %d",listening_socket);

	socket_start_listening_select(listening_socket, safa_manejador_de_eventos);

	safa_exit();
	return EXIT_SUCCESS;
}

void safa_initialize(){
	estado_operatorio = false;
	cpu_conectado = false;
	dam_conectado = false;

	config = config_create("/home/utnso/workspace/tp-2018-2c-RegorDTs/configs/S-AFA.txt");
	retardo_planificacion = config_get_int_value(config, "RETARDO_PLANIF");
	mkdir("/home/utnso/workspace/tp-2018-2c-RegorDTs/logs",0777);
	logger = log_create("/home/utnso/workspace/tp-2018-2c-RegorDTs/logs/S-AFA.log", "S-AFA", false, LOG_LEVEL_TRACE);

	cpu_conexiones = list_create();
	cola_mensajes = list_create();
	sem_init(&sem_cont_cola_mensajes, 0, 0);
	pthread_mutex_init(&sem_mutex_cola_mensajes, NULL);
	sem_init(&sem_cont_cpu_conexiones, 0, 0);
	pthread_mutex_init(&sem_mutex_cpu_conexiones, NULL);
}

void safa_iniciar_estado_operatorio(){
	estado_operatorio = true;

	/* Creo el hilo gestor de programas */
	if(pthread_create( &thread_gestor, NULL, (void*) gestor_iniciar, NULL) ){
		log_error(logger,"No pude crear el hilo para el gestor");
			safa_exit();
		exit(EXIT_FAILURE);
	}
	log_info(logger, "Creo el hilo para el gestor");
	pthread_detach(thread_gestor);

	/* Creo el hilo para el planificador */
	if(pthread_create( &thread_planificador, NULL, (void*) planificador_iniciar, NULL) ){
		log_error(logger,"No pude crear el hilo para el planificador");
		safa_exit();
		exit(EXIT_FAILURE);
	}
	log_info(logger, "Creo el hilo para el planificador");
	pthread_detach(thread_planificador);
}

int safa_manejador_de_eventos(int socket, t_msg* msg){
	log_info(logger, "EVENTO: Emisor: %d, Tipo: %d, Tamanio: %d, Mensaje: %s",msg->header->emisor,msg->header->tipo_mensaje,msg->header->payload_size,(char*) msg->payload);

	if(msg->header->emisor == DAM){
		switch(msg->header->tipo_mensaje){
			case CONEXION:
				log_info(logger, "Se conecto DAM");
				dam_conectado = true;
				dam_socket = socket;

				if(!estado_operatorio && cpu_conectado){
					safa_iniciar_estado_operatorio();
				}
			break;

			case DESCONEXION:
				log_error(logger, "Se desconecto DAM");
				return -1;
			break;

			case RESULTADO_ABRIR:
				log_info(logger, "Recibi el resultado de cargar algo en memoria");
				safa_encolar_mensaje(msg);
			break;

			default:
				log_info(logger, "No entendi el mensaje de DAM");
			break;
		}
	}
	else if(msg->header->emisor == CPU){
		switch(msg->header->tipo_mensaje){
			case CONEXION:
				log_info(logger, "Se conecto un CPU");
				cpu_conectado = true;

				// Agrego el nuevo socket a la lista de CPUs
				conexion_cpu_add_new(socket);

				// TODO: No enviarle el quantum al principio, sino junto al DTB cuando tenga que ejecutar algo.
				// Otra opcion: manejar toodo desde SAFA, y CPU no se entera del quantum (hacer esto)
				// Le mando el quantum
				int quantum = config_get_int_value(config, "QUANTUM");
				t_msg* mensaje_a_enviar = msg_create(SAFA, HANDSHAKE, (void*) quantum, sizeof(int));
				msg_send(socket, *mensaje_a_enviar);
				msg_free(&mensaje_a_enviar);

				if(!estado_operatorio && dam_conectado){
					safa_iniciar_estado_operatorio();
				}
			break;

			case DESCONEXION:
				log_info(logger, "Se desconecto un CPU");
				conexion_cpu_disconnect(socket);

				return -1;
			break;

			case BLOCK:
			case READY:
			case EXIT:
				conexion_cpu_set_active(socket); // Esta CPU es seleccionable de nuevo
				safa_encolar_mensaje(msg);
			break;

			default:
				log_info(logger, "No entendi el mensaje de CPU");
		}
	}
	else if(msg->header->emisor == DESCONOCIDO){
		log_info(logger, "Me hablo alguien desconocido");
	}
	return 1;
}

void safa_encolar_mensaje(t_msg* msg){
	t_msg* msg_dup = msg_duplicar(msg);

	pthread_mutex_lock(&sem_mutex_cola_mensajes);
	list_add(cola_mensajes, msg_dup);
	pthread_mutex_unlock(&sem_mutex_cola_mensajes);
	sem_post(&sem_cont_cola_mensajes);
}

void safa_exit(){
	close(listening_socket);
	close(dam_socket);
	list_destroy_and_destroy_elements(cpu_conexiones, free);
	log_destroy(logger);
	config_destroy(config);
}

