#include "S-AFA.h"

int main(void){
	if(safa_initialize() == -1){
		exit(EXIT_FAILURE);
	}

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

void safa_inotify_config_iniciar(){
	int buffer_len = ((sizeof(struct inotify_event)) + 16) * 32;
	char buffer[buffer_len];
	int length, i;
	 /* Inotify */
	int inotify_fd = inotify_init();
	int inotify_wd = inotify_add_watch(inotify_fd, CONFIG_PATH, IN_MODIFY | IN_DELETE );
	/* Espero eventos del file descriptor de inotify */
	while(1){
		i = 0;
		length = read(inotify_fd, buffer, buffer_len);
		log_info(logger, "[INOTIFY] Lei %d", length);
		if ( length < 0 ) {
			log_error(logger, "[INOTIFY] Fallo el read"); continue;
		}
		while(i < length){
			struct inotify_event* event = (struct inotify_event*) &buffer[i];
			if(event->len){
				if(event->mask & IN_DELETE){
					log_error(logger, "[INOTIFY] El archivo %s ha sido borrado\n", event->name );
				}
				else if(event->mask & IN_MODIFY){
					log_info(logger, "[INOTIFY] El archivo %s ha sido modificado\n", event->name );

					pthread_mutex_lock(&sem_mutex_config_algoritmo);
					pthread_mutex_lock(&sem_mutex_config_multiprogramacion);
					pthread_mutex_lock(&sem_mutex_config_quantum);
					pthread_mutex_lock(&sem_mutex_config_retardo);

					config_destroy(config);
					config = config_create(CONFIG_PATH);
					util_config_fix_comillas(&config, "ALGORITMO");

					/* ~~~~~~~~~~~~~~~~~~~ACTUALIZO ALGORITMO ~~~~~~~~~~~~~~~~~~~ */
					algoritmo = config_get_string_value(config, "ALGORITMO");
					pthread_mutex_unlock(&sem_mutex_config_algoritmo);

					/* ~~~~~~~~~~~~~~~~~~~ACTUALIZO QUANTUM ~~~~~~~~~~~~~~~~~~~ */
					quantum = config_get_int_value(config, "QUANTUM");
					log_info(logger, "[INOTIFY] Nuevo quantum: %d\n", quantum);
					/* Le aviso a todas las CPUs*/
					int j = 0;
					pthread_mutex_lock(&sem_mutex_cpu_conexiones);
					for(; j<cpu_conexiones->elements_count; j++){
						int socket_cpu = ((t_conexion_cpu*) list_get(cpu_conexiones, j))->socket;
						safa_send(socket_cpu, QUANTUM);
					}
					pthread_mutex_unlock(&sem_mutex_cpu_conexiones);
					pthread_mutex_unlock(&sem_mutex_config_quantum);

					/* ~~~~~~~~~~~~~~~~~~~ACTUALIZO MULTIPROGRAMACION ~~~~~~~~~~~~~~~~~~~ */
					int nueva_multiprogramacion = config_get_int_value(config, "MULTIPROGRAMACION");
					log_info(logger, "[INOTIFY] Nueva multiprogramacion: %d\n", nueva_multiprogramacion);
					/* Modifico el semaforo sem_cont_procesos*/
					while(nueva_multiprogramacion < multiprogramacion){ // Tengo que esperar la finalizacion de procesos
						sem_wait(&sem_cont_procesos);
						multiprogramacion--;
					}
					while(multiprogramacion < nueva_multiprogramacion){ // Hago signal de los nuevos procesos permitidos
						sem_post(&sem_cont_procesos);
						multiprogramacion++;
					}
					pthread_mutex_unlock(&sem_mutex_config_multiprogramacion);

					/* ~~~~~~~~~~~~~~~~~~~ACTUALIZO RETARDO ~~~~~~~~~~~~~~~~~~~ */
					retardo_planificacion = config_get_int_value(config, "RETARDO_PLANIF");
					log_info(logger, "[INOTIFY] Nuevo retardo: %d\n", retardo_planificacion);
					pthread_mutex_unlock(&sem_mutex_config_retardo);
				}
			}
			i += (sizeof(struct inotify_event)) + event->len;
		} // Fin while(i < length)
	} // Fin while(1)
}

int safa_initialize(){

	estado_operatorio = false;
	cpu_conectado = false;
	dam_conectado = false;

	config = config_create(CONFIG_PATH);
	util_config_fix_comillas(&config, "ALGORITMO");

	mkdir("/home/utnso/workspace/tp-2018-2c-RegorDTs/logs",0777);
	logger = log_create("/home/utnso/workspace/tp-2018-2c-RegorDTs/logs/S-AFA.log", "S-AFA", false, LOG_LEVEL_TRACE);

	cpu_conexiones = list_create();
	cola_mensajes = list_create();
	sem_init(&sem_cont_cola_mensajes, 0, 0);
	pthread_mutex_init(&sem_mutex_cola_mensajes, NULL);
	sem_init(&sem_cont_cpu_conexiones, 0, 0);
	pthread_mutex_init(&sem_mutex_cpu_conexiones, NULL);

	pthread_mutex_init(&sem_mutex_config_algoritmo, NULL);
	pthread_mutex_init(&sem_mutex_config_quantum, NULL);
	pthread_mutex_init(&sem_mutex_config_multiprogramacion, NULL);
	pthread_mutex_init(&sem_mutex_config_retardo, NULL);

	pthread_mutex_lock(&sem_mutex_config_algoritmo);
	algoritmo = config_get_string_value(config, "ALGORITMO");
	pthread_mutex_unlock(&sem_mutex_config_algoritmo);

	pthread_mutex_lock(&sem_mutex_config_quantum);
	quantum = config_get_int_value(config, "QUANTUM");
	pthread_mutex_unlock(&sem_mutex_config_quantum);

	pthread_mutex_lock(&sem_mutex_config_multiprogramacion);
	multiprogramacion = config_get_int_value(config, "MULTIPROGRAMACION");
	pthread_mutex_unlock(&sem_mutex_config_multiprogramacion);

	pthread_mutex_lock(&sem_mutex_config_retardo);
	retardo_planificacion = config_get_int_value(config, "RETARDO_PLANIF");
	pthread_mutex_unlock(&sem_mutex_config_retardo);

	/* Creo el hilo inotify, que se encarga de toodo lo relacionado al archivo de config */
	if(pthread_create( &thread_inotify_config, NULL, (void*) safa_inotify_config_iniciar, NULL) ){
		log_error(logger,"No pude crear el hilo inotify");
		return -1;
	}
	log_info(logger, "Creo el hilo inotify");
	pthread_detach(thread_inotify_config);

	return 1;
}

void safa_iniciar_estado_operatorio(){
	estado_operatorio = true;
	sem_init(&sem_bin_plp_cargar_archivo, 0, 0);
	sem_init(&sem_bin_pcp_cargar_archivo, 0, 0);

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

int safa_send(int socket, e_tipo_msg tipo_msg, ...){
	t_msg* mensaje_a_enviar;
	int ret;
 	t_dtb* dtb;
 	va_list arguments;
	va_start(arguments, tipo_msg);
 	switch(tipo_msg){
		case QUANTUM: // A CPU
		case HANDSHAKE: // A CPU
			mensaje_a_enviar = msg_create(SAFA, QUANTUM, (void**) quantum, sizeof(int));
		break;
 		case EXEC:
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

				// Le mando el quantum inicial
				pthread_mutex_lock(&sem_mutex_config_quantum);
				safa_send(socket, HANDSHAKE);
				pthread_mutex_unlock(&sem_mutex_config_quantum);

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

