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
	config = config_create("../../configs/S-AFA.txt");
	retardo_planificacion = config_get_int_value(config, "RETARDO_PLANIF");
	mkdir("../../logs",0777);
	logger = log_create("../../logs/S-AFA.log", "S-AFA", false, LOG_LEVEL_TRACE);
	cpu_conexiones = list_create();
}

void safa_iniciar_estado_operatorio(){
	estado_operatorio = true;

	/* Creo el hilo consola del gestor de programas */
	if(pthread_create( &thread_consola, NULL, (void*) gestor_consola_iniciar, NULL) ){
		log_error(logger,"No pude crear el hilo para la consola");
			safa_exit();
		exit(EXIT_FAILURE);
	}
	log_info(logger, "Creo el hilo para la consola");
	pthread_detach(thread_consola);

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
	t_dtb* dtb_recibido;

	bool _mismo_fd_socket(void* conexion_cpu_en_lista){
		return ((t_conexion_cpu*) conexion_cpu_en_lista)->socket == socket;
	}

	log_info(logger, "EVENTO: Emisor: %d, Tipo: %d, Tamanio: %d, Mensaje: %s",msg->header->emisor,msg->header->tipo_mensaje,msg->header->payload_size,(char*) msg->payload);

	if(msg->header->emisor == DAM){
		switch(msg->header->tipo_mensaje){
			case CONEXION:
				log_info(logger, "Se conecto DAM");
				dam_conectado = true;
				dam_socket = socket;
				msg_free(&msg);

				if(!estado_operatorio && cpu_conectado){
					safa_iniciar_estado_operatorio();
				}
			break;

			case DESCONEXION:
				log_error(logger, "Se desconecto DAM");
				msg_free(&msg);
				return -1;
			break;

			case RESULTADO_ABRIR:
				log_info(logger, "Recibi el resultado de cargar algo en memoria");
				planificador_cargar_archivo_en_dtb(msg);
				msg_free(&msg);
			break;

			default:
				log_info(logger, "No entendi el mensaje de DAM");
				msg_free(&msg);
			break;
		}
	}
	else if(msg->header->emisor == CPU){
		switch(msg->header->tipo_mensaje){
			case CONEXION:
				log_info(logger, "Se conecto un CPU");
				cpu_conectado = true;

				// Agrego el nuevo socket a la lista de CPUs
				t_conexion_cpu*  nueva_conexion_cpu = malloc(sizeof(t_conexion_cpu));
				nueva_conexion_cpu->socket = socket;
				nueva_conexion_cpu->en_uso = 0;
				list_add(cpu_conexiones, (void*) nueva_conexion_cpu);

				// Le mando el quantum
				int quantum = config_get_int_value(config, "QUANTUM");
				t_msg* mensaje_a_enviar = msg_create(SAFA, HANDSHAKE, (void*) quantum, sizeof(int));
				msg_send(socket, *mensaje_a_enviar);
				msg_free(&mensaje_a_enviar);
				msg_free(&msg);

				if(!estado_operatorio && dam_conectado){
					safa_iniciar_estado_operatorio();
				}
			break;

			case DESCONEXION:
				log_info(logger, "Se desconecto un CPU");
				list_remove_and_destroy_by_condition(cpu_conexiones, _mismo_fd_socket, free);
				msg_free(&msg);
				return -1;
			break;

			case BLOCK:
				conexion_cpu_set_active(socket); // Esta CPU es seleccionable de nuevo
				dtb_recibido = desempaquetar_dtb(msg);
				planificador_cargar_nuevo_path_vacio_en_dtb(dtb_recibido);
				pcp_mover_dtb(dtb_recibido->gdt_id, "EXEC", "BLOCK");
				dtb_destroy(dtb_recibido);
				msg_free(&msg);
			break;

			case READY:
				conexion_cpu_set_active(socket); // Esta CPU es seleccionable de nuevo
				dtb_recibido = desempaquetar_dtb(msg);
				planificador_cargar_nuevo_path_vacio_en_dtb(dtb_recibido);
				pcp_mover_dtb(dtb_recibido->gdt_id, "EXEC", "READY");
				dtb_destroy(dtb_recibido);
				msg_free(&msg);
			break;

			case EXIT:
				conexion_cpu_set_active(socket); // Esta CPU es seleccionable de nuevo
				dtb_recibido = desempaquetar_dtb(msg);
				planificador_finalizar_dtb(dtb_recibido->gdt_id);
				dtb_destroy(dtb_recibido);
				msg_free(&msg);
			break;

			default:
				log_info(logger, "No entendi el mensaje de CPU");
				msg_free(&msg);
		}
	}
	else if(msg->header->emisor == DESCONOCIDO){
		log_info(logger, "Me hablo alguien desconocido");
		msg_free(&msg);
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

