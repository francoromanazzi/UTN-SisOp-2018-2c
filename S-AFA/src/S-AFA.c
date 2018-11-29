 #include "S-AFA.h"

static t_safa_semaforo* _safa_recursos_encontrar_o_crear(char* nombre_recurso, int valor_iniciar_recurso);
static void _safa_recursos_asignar(t_safa_semaforo* sem, unsigned int id);

int main(void){
	if(safa_initialize() == -1){
		exit(EXIT_FAILURE);
	}

	/* Creo el socket de escucha y comienzo el select */
	char* local_ip = get_local_ip();
	if((listening_socket = socket_create_listener(local_ip, safa_config_get_string_value("PUERTO"))) == -1){
		log_error(logger, "[S-AFA] No pude crear el socket de escucha");
		free(local_ip);
		safa_exit();
		exit(EXIT_FAILURE);
	}
	log_info(logger, "[S-AFA] Escucho en el socket %d. Mi IP es: %s",listening_socket, local_ip);
	free(local_ip);

	socket_start_listening_select(listening_socket, safa_manejador_de_eventos, 0 /*, 1, SAFA, INOTIFY, fd_inotify */); // TODO DESCOMENTAR

	safa_exit();
	return EXIT_SUCCESS;
}

int safa_initialize(){

	void _create_fd_inotify(){
		fd_inotify = inotify_init();
		inotify_add_watch(fd_inotify, CONFIG_PATH, (IN_CLOSE_WRITE || IN_MOVED_TO || IN_MODIFY) /*(IN_CLOSE | IN_MOVE | IN_MODIFY | IN_MOVE_SELF | IN_IGNORED)*/);
	}

	safa_protocol_initialize();
	safa_util_initialize();
	metricas_initialize();
	//_create_fd_inotify(); // TODO DESCOMENTAR

	estado_operatorio = false;
	cpu_conectado = false;
	dam_conectado = false;
	op_dummy_en_progreso = false;

	cpu_conexiones = list_create();
	pthread_mutex_init(&sem_mutex_cpu_conexiones, NULL);

	tabla_recursos = dictionary_create();
	pthread_mutex_init(&sem_mutex_tabla_recursos, NULL);

	sem_init(&sem_cont_recepcion_interrupcion_cpu, 0, 0);

	return 1;
}

void safa_iniciar_estado_operatorio(){
	estado_operatorio = true;

	cola_new = list_create();
	cola_ready = list_create(); pthread_mutex_init(&sem_mutex_cola_ready, NULL);
	cola_exec = list_create();
	cola_block = list_create(); list_add(cola_block, dtb_create_dummy());
	cola_exit = list_create(); pthread_mutex_init(&sem_mutex_cola_exit, NULL);

	sem_init(&sem_bin_crear_dtb_1, 0, 1);
	sem_init(&sem_bin_crear_dtb_0, 0, 0);

	lista_cpus_desalojables = list_create();
	pthread_mutex_init(&sem_mutex_lista_cpus_desalojables, NULL);

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
	int ret, id, ok;
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

 		case LIBERAR_MEMORIA_FM9: // A CPU
 			id = (int) va_arg(arguments, unsigned int);
 			mensaje_a_enviar = empaquetar_int(id);
 		break;

 		case RESULTADO_WAIT:
 			ok = va_arg(arguments, int);
 			mensaje_a_enviar = empaquetar_int(ok);
 		break;

 		case RESULTADO_SIGNAL:
 			mensaje_a_enviar = empaquetar_int(OK);
 		break;

 		case INTERRUPCION:
 			mensaje_a_enviar = empaquetar_int(OK);
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

	void _eliminar_socket_cpu_desalojable(){

		bool _mismo_socket(void* _socket){
			return (int) _socket == socket;
		}


		pthread_mutex_lock(&sem_mutex_lista_cpus_desalojables);
		(void) list_remove_by_condition(lista_cpus_desalojables, _mismo_socket);
		pthread_mutex_unlock(&sem_mutex_lista_cpus_desalojables);
	}

	bool _verificar_interrupcion_ok(t_msg* _msg){
		t_dtb* dtb = desempaquetar_dtb(msg);
		bool ret = dtb->gdt_id != 0;
		dtb_destroy(dtb);
		return ret;
	}


	log_info(logger, "[S-AFA] EVENTO: Emisor: %d, Tipo: %d, Tamanio: %d",msg->header->emisor,msg->header->tipo_mensaje,msg->header->payload_size);

	void* data;
	struct timespec time;
	unsigned int id;
	int ok;
	char* nombre_recurso;
	e_emisor destinatario_sentencia_ejecutada;

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

			case RESULTADO_FLUSH:
				log_info(logger, "[S-AFA] Recibi el resultado del flush");
				data = malloc(msg->header->payload_size);
				memcpy(data, msg->payload, msg->header->payload_size);
				safa_protocol_encolar_msg_y_avisar(S_AFA, PCP, RESULTADO_IO_DAM, data);
			break;

			case RESULTADO_CREAR_MDJ:
				log_info(logger, "[S-AFA] Recibi el resultado de crear un nuevo archivo");
				data = malloc(msg->header->payload_size);
				memcpy(data, msg->payload, msg->header->payload_size);
				safa_protocol_encolar_msg_y_avisar(S_AFA, PCP, RESULTADO_IO_DAM, data);
			break;

			case RESULTADO_BORRAR:
				log_info(logger, "[S-AFA] Recibi el resultado de borrar un archivo");
				data = malloc(msg->header->payload_size);
				memcpy(data, msg->payload, msg->header->payload_size);
				safa_protocol_encolar_msg_y_avisar(S_AFA, PCP, RESULTADO_IO_DAM, data);
			break;

			default:
				log_info(logger, "[S-AFA] No entendi el mensaje de DAM");
			break;
		}
	}
	else if(msg->header->emisor == CPU){
		switch(msg->header->tipo_mensaje){
			case CONEXION:
				ultimo_socket_cpu = socket;

				safa_send(socket, HANDSHAKE);
			break;

			case CONEXION_INTERRUPCIONES:
				log_info(logger, "[S-AFA] Se conecto un CPU");
				cpu_conectado = true;

				safa_send(socket, HANDSHAKE);

				conexion_cpu_add_new(ultimo_socket_cpu, socket); // Agrego el nuevo socket a la lista de CPUs

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

			case SENTENCIA_EJECUTADA:
				destinatario_sentencia_ejecutada = (e_emisor) desempaquetar_int(msg);
				log_info(logger, "[S-AFA] CPU me aviso que ejecuto una sentencia dirigida al modulo %d", destinatario_sentencia_ejecutada);
				metricas_nueva_sentencia_ejecutada(destinatario_sentencia_ejecutada);
			break;

			case BLOCK:
				conexion_cpu_set_active(socket); // Esta CPU es seleccionable de nuevo
				_eliminar_socket_cpu_desalojable(); // Por si estaba en la cola de baja prioridad
				safa_protocol_encolar_msg_y_avisar(S_AFA, PCP, BLOCK_DTB, desempaquetar_dtb(msg));
			break;

			case READY:
				conexion_cpu_set_active(socket); // Esta CPU es seleccionable de nuevo
				_eliminar_socket_cpu_desalojable(); // Por si estaba en la cola de baja prioridad
				safa_protocol_encolar_msg_y_avisar(S_AFA, PCP, READY_DTB, desempaquetar_dtb(msg));
			break;

			case INTERRUPCION:
				if(_verificar_interrupcion_ok(msg)){ // Por ahi CPU ya habia desalojado antes el DTB por otras razones
					conexion_cpu_set_active(socket); // Esta CPU es seleccionable de nuevo
					safa_protocol_encolar_msg_y_avisar(S_AFA, PCP, READY_DTB, desempaquetar_dtb(msg));
				}
				sem_post(&sem_cont_recepcion_interrupcion_cpu);
			break;

			case EXIT:
				conexion_cpu_set_active(socket); // Esta CPU es seleccionable de nuevo
				_eliminar_socket_cpu_desalojable(); // Por si estaba en la cola de baja prioridad
				safa_protocol_encolar_msg_y_avisar(S_AFA, PCP, EXIT_DTB, desempaquetar_dtb(msg));
			break;

			case WAIT:
				desempaquetar_wait_signal(msg, &id, &nombre_recurso);
				log_info(logger, "[S-AFA] CPU me pidio WAIT de %s del PID: %d", nombre_recurso, id);

				pthread_mutex_lock(&sem_mutex_tabla_recursos);
				if(safa_recursos_wait(id, nombre_recurso)) // Pude asignarle el recurso
					safa_send(socket, RESULTADO_WAIT, OK);
				else 									   // No pude asignarle el recurso
					safa_send(socket, RESULTADO_WAIT, NO_OK);
				pthread_mutex_unlock(&sem_mutex_tabla_recursos);

				free(nombre_recurso);
			break;

			case SIGNAL:
				desempaquetar_wait_signal(msg, &id, &nombre_recurso);
				log_info(logger, "[S-AFA] CPU me pidio SIGNAL de %s del PID: %d", nombre_recurso, id);

				pthread_mutex_lock(&sem_mutex_tabla_recursos);
				safa_recursos_signal(id, nombre_recurso);
				pthread_mutex_unlock(&sem_mutex_tabla_recursos);

				free(nombre_recurso);

				safa_send(socket, RESULTADO_SIGNAL);
			break;

			case RESULTADO_LIBERAR_MEMORIA_FM9:
				conexion_cpu_set_active(socket); // Esta CPU es seleccionable de nuevo
				safa_protocol_encolar_msg_y_avisar(S_AFA, PCP, NUEVO_CPU_DISPONIBLE);
			break;

			default:
				log_info(logger, "[S-AFA] No entendi el mensaje de CPU");
		}
	}
	else if(msg->header->emisor == SAFA){
		switch(msg->header->tipo_mensaje){
			case INOTIFY:
				log_info(logger, "[INOTIFY] Nuevo evento");
				safa_manejar_inotify();
			break;
		}
	}
	else if(msg->header->emisor == DESCONOCIDO){
		log_info(logger, "[S-AFA] Me hablo alguien desconocido");
	}
	return 1;
}

void safa_manejar_inotify(){
	char buf[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
	const struct inotify_event *event;
	ssize_t len;
	char *ptr;

	/* Read some events. */
	len = read(fd_inotify, buf, 4096);
	if (len == -1 && errno != EAGAIN) {
		log_error(logger, "[INOTIFY] read");
	    return;
	}


   if (len <= 0){
	   log_error(logger, "[INOTIFY] len <= 0");
	   return;
   }


   /* Loop over all events in the buffer */
   for (ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) {
	   event = (const struct inotify_event *) ptr;

        if ((event->mask & IN_CLOSE_WRITE) ||
			(event->mask & IN_MOVED_TO) ||
			(event->mask & IN_MODIFY)
        ){

			/* Actualizo config */
			pthread_mutex_lock(&sem_mutex_config);

			char* viejo_algoritmo = strdup(config_get_string_value(config, "ALGORITMO"));
			int viejo_quantum = config_get_int_value(config, "QUANTUM");
			int vieja_multiprogramacion = config_get_int_value(config, "MULTIPROGRAMACION");
			int viejo_retardo = config_get_int_value(config, "RETARDO_PLANIF");

			config_destroy(config);
			safa_config_create_fixed();

			if(strcmp(viejo_algoritmo, config_get_string_value(config, "ALGORITMO")))
				log_info(logger, "[INOTIFY] Nuevo algoritmo: %s", config_get_string_value(config, "ALGORITMO"));
			if(viejo_quantum != config_get_int_value(config, "QUANTUM"))
				log_info(logger, "[INOTIFY] Nuevo quantum: %d", config_get_int_value(config, "QUANTUM"));
			if(vieja_multiprogramacion != config_get_int_value(config, "MULTIPROGRAMACION")){
				log_info(logger, "[INOTIFY] Nueva multiprogramacion: %d", config_get_int_value(config, "MULTIPROGRAMACION"));
				if(vieja_multiprogramacion < config_get_int_value(config, "MULTIPROGRAMACION"))
					safa_protocol_encolar_msg_y_avisar(S_AFA, PLP, GRADO_MULTIPROGRAMACION_AUMENTADO);
			}
			if(viejo_retardo != config_get_int_value(config, "RETARDO_PLANIF"))
				log_info(logger, "[INOTIFY] Nuevo retardo (uS): %d", config_get_int_value(config, "RETARDO_PLANIF"));
			pthread_mutex_unlock(&sem_mutex_config);

			free(viejo_algoritmo);
        }
    }

}

static t_safa_semaforo* _safa_recursos_encontrar_o_crear(char* nombre_recurso, int valor_iniciar_recurso){
	t_safa_semaforo* sem;
	if((sem = (t_safa_semaforo*) dictionary_get(tabla_recursos, nombre_recurso)) == NULL){ // Este recurso no existia
		sem = malloc(sizeof(t_safa_semaforo));
		sem->valor = valor_iniciar_recurso;
		sem->lista_pid_asignados = list_create();
		sem->lista_pid_bloqueados = list_create();
		dictionary_put(tabla_recursos, nombre_recurso, (void*) sem);
	}
	return sem;
}

static void _safa_recursos_asignar(t_safa_semaforo* sem, unsigned int id){

	bool _mismo_id(void* v2){
		return  (unsigned int) (((t_vector2*) v2)->x) == id;
	}

	t_vector2* v2_asignacion;
	if((v2_asignacion = (t_vector2*) list_find(sem->lista_pid_asignados, _mismo_id)) == NULL){ // Este proceso no tenia asignado otro
		v2_asignacion = malloc(sizeof(t_vector2));
		v2_asignacion->x = (int) id;
		v2_asignacion->y = 0;
		list_add(sem->lista_pid_asignados, (void*) v2_asignacion);
	}
	v2_asignacion->y++;
}


bool safa_recursos_wait(unsigned int id, char* nombre_recurso){

	bool _mismo_id(void* v2){
		return  (unsigned int) (((t_vector2*) v2)->x) == id;
	}

	t_safa_semaforo* sem = _safa_recursos_encontrar_o_crear(nombre_recurso, 1);

	if( --(sem->valor) < 0){ // No pude asignar el recurso. Bloqueo el DTB
		list_add(sem->lista_pid_bloqueados, (void*) id);
		//safa_protocol_encolar_msg_y_avisar(S_AFA, PCP, BLOCK_DTB_ID, id);
		return false;
	}

	/* Puedo asignar el recurso */
	_safa_recursos_asignar(sem, id);

	return true;
}

void safa_recursos_signal(unsigned int id_signal, char* nombre_recurso){
	unsigned int id_a_desbloquear;

	bool _mismo_id_a_desbloquear(void* _id){
		return (unsigned int) _id == id_a_desbloquear;
	}

	bool _mismo_id_signal(void* v2){
		return  (unsigned int) (((t_vector2*) v2)->x) == id_signal;
	}


	t_safa_semaforo* sem = _safa_recursos_encontrar_o_crear(nombre_recurso, 1);

	/* Le desasigno una instancia del recurso a id_signal, si es que tenia */
	t_vector2* asignacion;
	if((asignacion = list_find(sem->lista_pid_asignados, _mismo_id_signal)) != NULL){
		asignacion->y = max(0, --(asignacion->y));
	}

	if( ++(sem->valor) <= 0){ // Habian procesos esperando este recurso
		unsigned int id_a_desbloquear = (unsigned int) list_remove(sem->lista_pid_bloqueados, 0); // FIFO
		_safa_recursos_asignar(sem, id_a_desbloquear);

		if((list_find(sem->lista_pid_bloqueados, _mismo_id_a_desbloquear)) == NULL){ // Verifico que no haya realizado otros wait
			safa_protocol_encolar_msg_y_avisar(S_AFA, PCP, READY_DTB_ID, id_a_desbloquear);
		}
	}
}

void safa_recursos_liberar_pid(unsigned int id){ // Esta funcion la invoca el hilo PCP

	void _intentar_liberar_recurso(char* nombre_recurso, void* _sem){

		bool __mismo_id_vector2(void* v2){
			return  (unsigned int) (((t_vector2*) v2)->x) == id;
		}
		bool __mismo_id(void* _id){
			return (unsigned int) _id == id;
		}


		t_safa_semaforo* sem = _sem;

		while(list_remove_by_condition(sem->lista_pid_bloqueados, __mismo_id)); // Elimino todos los pedidos

		/* Le saco las instancias del recurso, incremento el valor del semaforo y debloqueo */
		t_vector2* asignacion;
		if((asignacion = list_remove_by_condition(sem->lista_pid_asignados, __mismo_id_vector2)) != NULL){
			sem->valor += asignacion->y;

			/* Desbloqueo */
			while(!list_is_empty(sem->lista_pid_bloqueados) && --(sem->valor) >= 0){
				unsigned int id_a_desbloquear = (unsigned int) list_remove(sem->lista_pid_bloqueados, 0); // FIFO

				bool _mismo_id_a_desbloquear(void* _id){
					return (unsigned int) _id == id_a_desbloquear;
				}


				_safa_recursos_asignar(sem, id_a_desbloquear);

				if((list_find(sem->lista_pid_bloqueados, _mismo_id_a_desbloquear)) == NULL){ // Verifico que no haya realizado otros wait
					safa_protocol_encolar_msg_y_avisar(S_AFA, PCP, READY_DTB_ID, id_a_desbloquear);
				}
			}

			free(asignacion);
		}
	}

	pthread_mutex_lock(&sem_mutex_tabla_recursos);
	dictionary_iterator(tabla_recursos, _intentar_liberar_recurso);
	pthread_mutex_unlock(&sem_mutex_tabla_recursos);
}

void safa_exit(){
	close(listening_socket);
	close(dam_socket);
	list_destroy_and_destroy_elements(cpu_conexiones, free);
	log_destroy(logger);
	config_destroy(config);
}

