#include "CPU.h"

int main(void) {
	if(cpu_initialize() == -1){ // Hubo algun error
		cpu_exit(); return EXIT_FAILURE;
	}

	while(1){
		if(cpu_esperar_dtb() == -1){
			cpu_exit();
			return EXIT_FAILURE;
		}
	}

	cpu_exit();
	return EXIT_SUCCESS;
}

int cpu_initialize(){

	void config_create_fixed(char* path){
		config = config_create(path);
		util_config_fix_comillas(&config, "IP_SAFA");
		util_config_fix_comillas(&config, "IP_DIEGO");
		util_config_fix_comillas(&config, "IP_MEM");
	}

	config_create_fixed("/home/utnso/workspace/tp-2018-2c-RegorDTs/configs/CPU.txt");

	mkdir("/home/utnso/workspace/tp-2018-2c-RegorDTs/logs",0777);
	char* thread_id = string_itoa((int) process_get_thread_id());
	char* path_log = strdup("/home/utnso/workspace/tp-2018-2c-RegorDTs/logs/CPU_");
	string_append(&path_log, thread_id);
	string_append(&path_log, ".log");
	logger = log_create(path_log, "CPU", false, LOG_LEVEL_TRACE);
	free(path_log);
	free(thread_id);

	retardo_ejecucion = config_get_int_value(config, "RETARDO");

	pthread_mutex_init(&sem_mutex_quantum, NULL);

	if(!cpu_connect_to_safa()){
		log_error(logger, "No pude conectarme a SAFA");
		return -1;
	}
	if(!cpu_connect_to_dam()){
		log_error(logger, "No pude conectarme a DAM");
		return -1;
	}
	log_info(logger, "Me conecte a DAM");

	if(!cpu_connect_to_fm9()){
		log_error(logger, "No pude conectarme a FM9");
		return -1;
	}
	log_info(logger, "Me conecte a FM9");

	log_info(logger, "NUEVO CPU");
	return 1;
}

int cpu_send(int socket, e_tipo_msg tipo_msg, ...){
	t_msg* mensaje_a_enviar;
	int ret;

	int base, offset;
	t_dtb* dtb;

	va_list arguments;
	va_start(arguments, tipo_msg);

	switch(tipo_msg){
		case CONEXION:
			mensaje_a_enviar = msg_create(CPU, CONEXION, (void**) 1, sizeof(int));
		break;

		case ABRIR: // A DAM
			dtb = va_arg(arguments, t_dtb*);
			mensaje_a_enviar = empaquetar_abrir( dtb->ruta_escriptorio, dtb->gdt_id);
		break;

		case GET: // A FM9
			base = va_arg(arguments, int);
			offset = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_get(base, offset);
		break;

		case BLOCK: // A SAFA
		case EXIT: // A SAFA
		case READY: // A SAFA
			dtb = va_arg(arguments, t_dtb*);
			mensaje_a_enviar = empaquetar_dtb(dtb);
		break;
	}
	mensaje_a_enviar->header->emisor = CPU;
	mensaje_a_enviar->header->tipo_mensaje = tipo_msg;
	ret = msg_send(socket, *mensaje_a_enviar);
	msg_free(&mensaje_a_enviar);

	va_end(arguments);
	return ret;
}

int cpu_esperar_dtb(){
	t_msg* msg = malloc(sizeof(t_msg));
	t_dtb* dtb;

	if(msg_await(safa_socket, msg) == -1){
		log_info(logger, "Se desconecto S-AFA");
		msg_free(&msg);
		return -1;
	}

	switch(msg->header->tipo_mensaje){
		case EXEC:
			dtb = desempaquetar_dtb(msg);
			if(dtb->flags.inicializado == 0) // DUMMY
				log_info(logger, "Recibi ordenes de S-AFA de ejecutar el DUMMY, el solicitante tiene ID: %d", dtb->gdt_id);
			else
				log_info(logger, "Recibi ordenes de S-AFA de ejecutar el programa con ID: %d", dtb->gdt_id);
			cpu_ejecutar_dtb(dtb);
			dtb_destroy(dtb);
		break;

		case QUANTUM:
			pthread_mutex_lock(&sem_mutex_quantum);
			memcpy(&quantum, msg->payload, sizeof(int));
			pthread_mutex_unlock(&sem_mutex_quantum);
			log_info(logger, "Mi nuevo quantum es de %d", quantum);
		break;

		case DESCONEXION: // Nunca recibe este mensaje... y tira segfault si se desconecta S-AFA :(
			log_info(logger, "Se desconecto S-AFA");
			msg_free(&msg);
			return -1;
		break;

		default:
			log_error(logger, "No entendi el mensaje de SAFA");
	}
	msg_free(&msg);
	return 1;
}

void cpu_ejecutar_dtb(t_dtb* dtb){
	dtb_mostrar(dtb, "EXEC"); // Sacar despues

	if(dtb->flags.inicializado == 0){ // DUMMY
		/* Le pido a Diego que abra el escriptorio */
		cpu_send(dam_socket, ABRIR, dtb);

		/* Le envio a SAFA el DTB, y le pido que lo bloquee */
		cpu_send(safa_socket, BLOCK, dtb);
	}
	else{ // NO DUMMY
		bool continuar_ejecucion = true;
		int operaciones_realizadas = 0;
		int nro_error;
		int base_escriptorio = (int) dictionary_get(dtb->archivos_abiertos, dtb->ruta_escriptorio);

		while(continuar_ejecucion){
			usleep(retardo_ejecucion);
			/* ------------------------- 1RA FASE: FETCH ------------------------- */
			char* instruccion;
			do
				instruccion = cpu_fetch(dtb, base_escriptorio);
			while(instruccion[0] == '#'); // Bucle, para ignorar las lineas con comentarios
			if(!strcmp(instruccion, "")){ // No hay mas instrucciones para leer
				log_info(logger, "Termine de ejecutar todo el DTB con ID: %d", dtb->gdt_id);
				cpu_send(safa_socket, EXIT, dtb); // Le aviso a SAFA que ya termine de ejecutar este DTB
				free(instruccion);
				return;
			}

			/* ---------------------- 2DA FASE: DECODIFICACION ---------------------- */
			t_operacion* operacion = cpu_decodificar(instruccion);
			free(instruccion);

			/* -------------------- 3RA FASE: BUSQUEDA OPERANDOS -------------------- */
			if ((nro_error = cpu_buscar_operandos(operacion)) != OK){
				log_error(logger, "Error %d al ejecutar el DTB con ID:%d", nro_error, dtb->gdt_id);
				cpu_send(safa_socket, EXIT, dtb);
				return;
			}

			/* ------------------------- 4TA FASE: EJECUCION ------------------------- */
			if ((nro_error = cpu_ejecutar_operacion(operacion)) != OK){
				log_error(logger, "Error %d al ejecutar el DTB con ID:%d", nro_error, dtb->gdt_id);
				cpu_send(safa_socket, EXIT, dtb);
				return;
			}
			operacion_free(&operacion);

			pthread_mutex_lock(&sem_mutex_quantum);
			continuar_ejecucion = ++operaciones_realizadas < quantum;
			pthread_mutex_unlock(&sem_mutex_quantum);
		}
		log_info(logger, "Se me termino el quantum");
		cpu_send(safa_socket, READY, dtb);
	}
}

char* cpu_fetch(t_dtb* dtb, int base_escriptorio){
	char* ret;

	/* Le pido a FM9 la proxima instruccion */
	cpu_send(fm9_socket, GET, base_escriptorio, dtb->pc);

	/* Espero de FM9 la proxima instruccion */
	t_msg* msg_resultado_get = malloc(sizeof(t_msg));
	msg_await(fm9_socket, msg_resultado_get);
	ret = (char*) desempaquetar_resultado_get(msg_resultado_get);
	log_info(logger, "La proxima instruccion es: %s", ret);

	msg_free(&msg_resultado_get);

	dtb->pc++;
	return ret;
}

t_operacion* cpu_decodificar(char* instruccion){
	t_operacion* ret = parse(instruccion);
	return ret;
}

int cpu_buscar_operandos(t_operacion* operacion){
	return OK;
}

int cpu_ejecutar_operacion(t_operacion* operacion){
	log_info(logger, "Ejecutando la operacion");
	switch(operacion->tipo_operacion){
		case OP_ABRIR:
		break;

		case OP_CONCENTRAR:
		break;

		case OP_ASIGNAR:
		break;

		case OP_WAIT:
		break;

		case OP_SIGNAL:
		break;

		case OP_FLUSH:
		break;

		case OP_CLOSE:
		break;

		case OP_CREAR:
		break;

		case OP_BORRAR:
		break;
	}
	return OK;
}

int cpu_connect_to_safa(){
	safa_socket = socket_connect_to_server(config_get_string_value(config, "IP_SAFA"), config_get_string_value(config, "PUERTO_SAFA"));
	log_info(logger, "Intento conectarme a SAFA");

	int resultado_send = cpu_send(safa_socket, CONEXION, NULL);

	t_msg* msg = malloc(sizeof(t_msg));
	int result_recv = msg_await(safa_socket, msg);
	if(msg->header->tipo_mensaje == HANDSHAKE){
		pthread_mutex_lock(&sem_mutex_quantum);
		memcpy(&quantum, msg->payload, sizeof(int));
		pthread_mutex_unlock(&sem_mutex_quantum);
		log_info(logger, "Recibi handshake de SAFA, y ahora mi quantum es de %d", quantum);
		msg_free(&msg);
	}
	else{
		log_error(logger, "No entendi el mensaje de SAFA");
		msg_free(&msg);
		return -1;
	}
	return safa_socket > 0 && resultado_send > 0 && result_recv > 0;
}

int cpu_connect_to_dam(){
	dam_socket = socket_connect_to_server(config_get_string_value(config, "IP_DIEGO"), config_get_string_value(config, "PUERTO_DIEGO"));
	log_info(logger, "Intento conectarme a DAM");

	int resultado_send = cpu_send(dam_socket, CONEXION, NULL);

	return safa_socket > 0 && resultado_send > 0;
}

int cpu_connect_to_fm9(){
	fm9_socket = socket_connect_to_server(config_get_string_value(config, "IP_MEM"), config_get_string_value(config, "PUERTO_MEM"));
	log_info(logger, "Intento conectarme a FM9");

	int resultado_send = cpu_send(fm9_socket, CONEXION, NULL);

	return fm9_socket > 0 && resultado_send > 0;
}

void cpu_exit(){
	close(safa_socket);
	close(dam_socket);
	close(fm9_socket);
	pthread_mutex_destroy(&sem_mutex_quantum);
	log_destroy(logger);
	config_destroy(config);
}