#include "CPU.h"

int main(void) {
	if(cpu_initialize() == -1){ // Hubo algun error
		cpu_exit(); return EXIT_FAILURE;
	}

	while(1){
		if(cpu_esperar_orden_safa() == -1){
			cpu_exit();
			return EXIT_FAILURE;
		}
	}

	cpu_exit();
	return EXIT_SUCCESS;
}

int cpu_initialize(){

	void _cpu_config_create_fixed(){
		config = config_create(CONFIG_PATH);
		util_config_fix_comillas(&config, "IP_SAFA");
		util_config_fix_comillas(&config, "IP_DIEGO");
		util_config_fix_comillas(&config, "IP_MEM");

		char* retardo_microsegundos_str = string_itoa(1000 * config_get_int_value(config, "RETARDO"));
		config_set_value(config, "RETARDO", retardo_microsegundos_str); // Milisegundos a microsegundos
		free(retardo_microsegundos_str);
	}

	void _cpu_log_create(){
		mkdir(LOG_DIRECTORY_PATH, 0777);
		char* thread_id = string_itoa((int) process_get_thread_id());
		char* path_log = strdup(LOG_DIRECTORY_PATH);
		string_append(&path_log, "CPU_");
		string_append(&path_log, thread_id);
		string_append(&path_log, ".log");
		logger = log_create(path_log, "CPU", false, LOG_LEVEL_TRACE);
		free(path_log);
		free(thread_id);
	}


	_cpu_config_create_fixed();
	_cpu_log_create();

	retardo_ejecucion = config_get_int_value(config, "RETARDO");

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

	return 1;
}

int cpu_send(int socket, e_tipo_msg tipo_msg, ...){
	t_msg* mensaje_a_enviar;
	int ret;

	char* path;
	char* datos;
	char* recurso;
	unsigned int id;
	int base, offset, cant_lineas, direccion_logica;
	t_dtb* dtb;
	struct timespec time;

	va_list arguments;
	va_start(arguments, tipo_msg);

	switch(tipo_msg){
		case CONEXION: // A SAFA, DAM y FM9
			mensaje_a_enviar = empaquetar_int(OK);
		break;

		case ABRIR: // A DAM
			path = va_arg(arguments, char*);
			id = va_arg(arguments, unsigned int);
			mensaje_a_enviar = empaquetar_abrir(path, id);
		break;

		case GET_FM9: // A FM9
			id = va_arg(arguments, unsigned int);
			base = va_arg(arguments, int);
			offset = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_get_fm9(id, base, offset);
		break;

		case ESCRIBIR_FM9: // A FM9 (asignar)
			id = va_arg(arguments, unsigned int);
			base = va_arg(arguments, int);
			offset = va_arg(arguments, int);
			datos = va_arg(arguments, char*);
			mensaje_a_enviar = empaquetar_escribir_fm9(id, base, offset, datos);
		break;

		case FLUSH: // A DAM
			id = va_arg(arguments, unsigned int);
			path = va_arg(arguments, char*);
			base = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_flush(id, path, base);
		break;

		case CLOSE: // A FM9
			id = va_arg(arguments, unsigned int);
			base = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_close(id, base);
		break;

		case WAIT: // A SAFA
			//id = va_arg(arguments, unsigned int); ?????
			recurso = va_arg(arguments, char*);
			mensaje_a_enviar = empaquetar_string(recurso);
		break;

		case SIGNAL: // A SAFA
			recurso = va_arg(arguments, char*);
			mensaje_a_enviar = empaquetar_string(recurso);
		break;

		case CREAR_MDJ: // A DAM
			id = va_arg(arguments, unsigned int);
			path = va_arg(arguments, char*);
			cant_lineas = va_arg(arguments, int);
			mensaje_a_enviar = empaquetar_crear_mdj(id, path, cant_lineas);
		break;

		case BORRAR: // A DAM
			id = va_arg(arguments, unsigned int);
			path = va_arg(arguments, char*);
			mensaje_a_enviar = empaquetar_borrar(id, path);
		break;

		case BLOCK: // A SAFA
		case EXIT: // A SAFA
		case READY: // A SAFA
			dtb = va_arg(arguments, t_dtb*);
			mensaje_a_enviar = empaquetar_dtb(dtb);
		break;

		case TIEMPO_RESPUESTA: // A SAFA
			id = va_arg(arguments, unsigned int);
			clock_gettime(CLOCK_MONOTONIC, &time);
			mensaje_a_enviar = empaquetar_tiempo_respuesta(id, time);
		break;

		case LIBERAR_MEMORIA_FM9: // A FM9
			id = va_arg(arguments, unsigned int);
			mensaje_a_enviar = empaquetar_int((int) id);
		break;

		case RESULTADO_LIBERAR_MEMORIA_FM9: // A SAFA
			mensaje_a_enviar = empaquetar_int(OK);
		break;
	}
	mensaje_a_enviar->header->emisor = CPU;
	mensaje_a_enviar->header->tipo_mensaje = tipo_msg;
	ret = msg_send(socket, *mensaje_a_enviar);
	msg_free(&mensaje_a_enviar);

	va_end(arguments);
	return ret;
}

int cpu_esperar_orden_safa(){
	t_msg* msg = malloc(sizeof(t_msg)), *msg_recibido;
	t_dtb* dtb;
	unsigned int id;

	if(msg_await(safa_socket, msg) == -1){
		log_info(logger, "Se desconecto S-AFA");
		free(msg);
		return -1;
	}

	switch(msg->header->tipo_mensaje){
		case EXEC:
			dtb = desempaquetar_dtb(msg);
			if(dtb->flags.inicializado == 0) // DUMMY
				log_info(logger, "Recibi ordenes de S-AFA de ejecutar el DUMMY, el solicitante tiene ID: %d", dtb->gdt_id);
			else
				log_info(logger, "Recibi ordenes de S-AFA de ejecutar el programa con ID: %d con %d unidades de quantum", dtb->gdt_id, dtb->quantum_restante);
			cpu_ejecutar_dtb(dtb);
			dtb_destroy(dtb);
		break;

		case DESCONEXION: // Nunca recibe este mensaje... y tira segfault si se desconecta S-AFA :(
			log_info(logger, "Se desconecto S-AFA");
			msg_free(&msg);
			return -1;
		break;

		case LIBERAR_MEMORIA_FM9:
			id = (unsigned int) desempaquetar_int(msg);
			log_info(logger, "S-AFA me pidio avisarle a FM9 que libere la memoria de un proceso");

			cpu_send(fm9_socket, LIBERAR_MEMORIA_FM9, id);
			msg_recibido = malloc(sizeof(t_msg));
			if(msg_await(fm9_socket, msg_recibido) == -1){
				log_error(logger, "Se desconecto FM9");
				free(msg_recibido);
				cpu_exit();
				exit(EXIT_FAILURE);
			}
			msg_free(&msg_recibido);

			cpu_send(safa_socket, RESULTADO_LIBERAR_MEMORIA_FM9);
		break;

		default:
			log_error(logger, "No entendi el mensaje de SAFA");
	}
	msg_free(&msg);
	return 1;
}

void cpu_ejecutar_dtb(t_dtb* dtb){
	dtb_mostrar(dtb);

	if(dtb->flags.inicializado == 0){ // DUMMY
		/* Le pido a Diego que abra el escriptorio */
		cpu_send(dam_socket, ABRIR, dtb->ruta_escriptorio, dtb->gdt_id);

		/* Le envio a SAFA el DTB, y le pido que lo bloquee */
		cpu_send(safa_socket, BLOCK, dtb);
	}
	else{ // NO DUMMY
		bool primera_vez_en_ejecutarse = dtb->pc == 0;
		int operaciones_realizadas = 0;
		int nro_error;
		int base_escriptorio = (int) dictionary_get(dtb->archivos_abiertos, dtb->ruta_escriptorio);

		while(dtb->quantum_restante > 0){
			usleep(retardo_ejecucion);

			/* ------------------------- 1RA FASE: FETCH ------------------------- */
			char* instruccion;
			do
				instruccion = cpu_fetch(dtb, base_escriptorio);
			while(instruccion[0] == '#'); // Bucle, para ignorar las lineas con comentarios
			if(!strcmp(instruccion, "")){ // No hay mas instrucciones para leer
				log_info(logger, "Termine de ejecutar todo el DTB con ID: %d", dtb->gdt_id);
				cpu_send(fm9_socket, LIBERAR_MEMORIA_FM9, dtb->gdt_id); // Le pido a FM9 que libere la memoria del proceso
				cpu_send(safa_socket, EXIT, dtb); // Le aviso a SAFA que ya termine de ejecutar este DTB
				free(instruccion);
				return;
			}
			log_info(logger, "La proxima instruccion es: %s", instruccion);

			/* ---------------------- 2DA FASE: DECODIFICACION ---------------------- */
			t_operacion* operacion = cpu_decodificar(instruccion);
			free(instruccion);

			/* ------------------------- 3RA FASE: EJECUCION ------------------------- */
			if(primera_vez_en_ejecutarse && operaciones_realizadas == 0) cpu_send(safa_socket, TIEMPO_RESPUESTA, dtb->gdt_id);

			operaciones_realizadas++;
			dtb->quantum_restante--;

			if ((nro_error = cpu_ejecutar_operacion(dtb, operacion)) != OK){
				if(nro_error == BLOCK){ // Tengo que pedir a SAFA que bloquee
					log_info(logger, "Le pido a SAFA que bloquee el DTB con ID:%d", dtb->gdt_id);
					cpu_send(safa_socket, BLOCK, dtb);
				}
				else{
					log_error(logger, "Error %d al ejecutar el DTB con ID:%d", nro_error, dtb->gdt_id);
					dtb->flags.error_nro = nro_error;
					cpu_send(fm9_socket, LIBERAR_MEMORIA_FM9, dtb->gdt_id); // Le pido a FM9 que libere la memoria del proceso
					cpu_send(safa_socket, EXIT, dtb);
				}
				operacion_free(&operacion);
				return; // Desalojo el DTB
			}
			operacion_free(&operacion);
		} // Fin while(dtb->quantum_restante > 0)
		log_info(logger, "Se me termino el quantum");
		cpu_send(safa_socket, READY, dtb);
	}
}

char* cpu_fetch(t_dtb* dtb, int base_escriptorio){
	char* ret;
	int ok;

	/* Le pido a FM9 la proxima instruccion */
	cpu_send(fm9_socket, GET_FM9, dtb->gdt_id, base_escriptorio, dtb->pc);

	/* Espero de FM9 la proxima instruccion */
	t_msg* msg_resultado_get = malloc(sizeof(t_msg));
	msg_await(fm9_socket, msg_resultado_get);
	desempaquetar_resultado_get_fm9(msg_resultado_get, &ok, &ret);
	msg_free(&msg_resultado_get);

	if(ok != OK){
		// TODO: Fin de archivo, o es un error? Creo que igual nunca va a ser != OK
	}

	dtb->pc++;
	return ret;
}

t_operacion* cpu_decodificar(char* instruccion){
	return parse(instruccion);
}

int cpu_ejecutar_operacion(t_dtb* dtb, t_operacion* operacion){
	log_info(logger, "Ejecutando la operacion");

	t_msg* msg_recibido;
	char* path;
	char* datos;
	int linea, base, ok, cant_lineas;

	switch(operacion->tipo_operacion){
		case OP_ABRIR:
			path = (char*) dictionary_get(operacion->operandos, "path");

			/* 1. Verificar que el archivo no se encuentre abierto, (puede ser el escriptorio) */
			if(dictionary_has_key(dtb->archivos_abiertos, path) && (int) dictionary_get(dtb->archivos_abiertos, path) != -1)
				return OK;

			/* 2. Pedir a DAM que abra el archivo */
			cpu_send(dam_socket, ABRIR, path, dtb->gdt_id);

			/* Le envio a SAFA el DTB, y le pido que lo bloquee */
			return BLOCK;
		break;

		case OP_CONCENTRAR:
		break;

		case OP_ASIGNAR:
			path = (char*) dictionary_get(operacion->operandos, "path");
			linea = atoi((char*) dictionary_get(operacion->operandos, "linea"));
			datos = (char*) dictionary_get(operacion->operandos, "datos");

			/* 1. Verificar que el archivo se encuentre abierto, y que no sea el escriptorio */
			if(!dictionary_has_key(dtb->archivos_abiertos, path) || !strcmp(path, dtb->ruta_escriptorio) || (int) dictionary_get(dtb->archivos_abiertos, path) == -1)
				return ERROR_ASIGNAR_ARCHIVO_NO_ABIERTO;

			/* 2. Le pido a FM9 que actualize los datos */
			cpu_send(fm9_socket, ESCRIBIR_FM9, dtb->gdt_id, (int) dictionary_get(dtb->archivos_abiertos, path), linea, datos);

			/* Recibo de FM9 el resultado de escribir */
			msg_recibido = malloc(sizeof(t_msg));
			if(msg_await(fm9_socket, msg_recibido) == -1){
				log_error(logger, "Se desconecto FM9");
				free(msg_recibido);
				cpu_exit();
				exit(EXIT_FAILURE);
			}
			ok = desempaquetar_int(msg_recibido);
			msg_free(&msg_recibido);
			return ok;
		break;

		case OP_WAIT:
			cpu_send(safa_socket, WAIT, (char*) dictionary_get(operacion->operandos, "recurso"));

			/* Recibo de SAFA el resultado del wait */
			msg_recibido = malloc(sizeof(t_msg));
			if(msg_await(safa_socket, msg_recibido) == -1){
				log_error(logger, "Se desconecto SAFA");
				free(msg_recibido);
				cpu_exit();
				exit(EXIT_FAILURE);
			}
			ok = desempaquetar_int(msg_recibido);
			if(ok == NO_OK){
				msg_free(&msg_recibido);
				/* Le envio a SAFA el DTB, y le pido que lo bloquee */
				return BLOCK;
			}
			msg_free(&msg_recibido);
		break;

		case OP_SIGNAL:
			cpu_send(safa_socket, SIGNAL, (char*) dictionary_get(operacion->operandos, "recurso"));

			/* Espero a que SAFA me deje seguir ejecutando */
			msg_recibido = malloc(sizeof(t_msg));
			if(msg_await(safa_socket, msg_recibido) == -1){
				log_error(logger, "Se desconecto SAFA");
				free(msg_recibido);
				cpu_exit();
				exit(EXIT_FAILURE);
			}
			msg_free(&msg_recibido);
		break;

		case OP_FLUSH:
			path = (char*) dictionary_get(operacion->operandos, "path");

			/* 1. Verificar que el archivo se encuentre abierto, y que no sea el escriptorio */
			if(!dictionary_has_key(dtb->archivos_abiertos, path) || !strcmp(path, dtb->ruta_escriptorio) || (int) dictionary_get(dtb->archivos_abiertos, path) == -1)
				return ERROR_FLUSH_ARCHIVO_NO_ABIERTO;

			/* 2. Pedir a DAM que haga flush */
			cpu_send(dam_socket, FLUSH, dtb->gdt_id, path, (int) dictionary_get(dtb->archivos_abiertos, path));

			/* Le envio a SAFA el DTB, y le pido que lo bloquee */
			return BLOCK;
		break;

		case OP_CLOSE:
			path = (char*) dictionary_get(operacion->operandos, "path");

			/* 1. Verificar que el archivo se encuentre abierto, y que no sea el escriptorio */
			if(!dictionary_has_key(dtb->archivos_abiertos, path) || !strcmp(path, dtb->ruta_escriptorio) || (int) dictionary_get(dtb->archivos_abiertos, path) == -1)
				return ERROR_CLOSE_ARCHIVO_NO_ABIERTO;

			/* 2. Pedir a FM9 que libere el archivo */
			cpu_send(fm9_socket, CLOSE, dtb->gdt_id, (int) dictionary_get(dtb->archivos_abiertos, path));

			/* 3. Saco el archivo del DTB */
			dictionary_remove(dtb->archivos_abiertos, path);

			/* Recibo de FM9 el resultado de cerrar el archivo */
			msg_recibido = malloc(sizeof(t_msg));
			if(msg_await(fm9_socket, msg_recibido) == -1){
				log_error(logger, "Se desconecto FM9");
				free(msg_recibido);
				cpu_exit();
				exit(EXIT_FAILURE);
			}
			ok = desempaquetar_int(msg_recibido);
			msg_free(&msg_recibido);
			return ok;
		break;

		case OP_CREAR:
			path = (char*) dictionary_get(operacion->operandos, "path");
			cant_lineas = atoi((char*) dictionary_get(operacion->operandos, "cant_lineas"));

			/* 1. Le envio a DAM el archivo a crear */
			cpu_send(dam_socket, CREAR_MDJ, dtb->gdt_id, path, cant_lineas);

			/* Le envio a SAFA el DTB, y le pido que lo bloquee */
			return BLOCK;
		break;

		case OP_BORRAR:
			path = (char*) dictionary_get(operacion->operandos, "path");

			/* 1. Le envio a DAM el archivo a borrar */
			cpu_send(dam_socket, BORRAR, dtb->gdt_id, path);

			/* Le envio a SAFA el DTB, y le pido que lo bloquee */
			return BLOCK;
		break;
	}
	return OK;
}

int cpu_connect_to_safa(){
	safa_socket = socket_connect_to_server(config_get_string_value(config, "IP_SAFA"), config_get_string_value(config, "PUERTO_SAFA"));
	log_info(logger, "Intento conectarme a SAFA");

	int resultado_send = cpu_send(safa_socket, CONEXION);

	t_msg* msg = malloc(sizeof(t_msg));
	int result_recv = msg_await(safa_socket, msg);
	if(msg->header->tipo_mensaje == HANDSHAKE){
		log_info(logger, "Recibi handshake de SAFA");
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

	int resultado_send = cpu_send(dam_socket, CONEXION);

	return safa_socket > 0 && resultado_send > 0;
}

int cpu_connect_to_fm9(){
	fm9_socket = socket_connect_to_server(config_get_string_value(config, "IP_MEM"), config_get_string_value(config, "PUERTO_MEM"));
	log_info(logger, "Intento conectarme a FM9");

	int resultado_send = cpu_send(fm9_socket, CONEXION);

	return fm9_socket > 0 && resultado_send > 0;
}

void cpu_exit(){
	close(safa_socket);
	close(dam_socket);
	close(fm9_socket);
	log_destroy(logger);
	config_destroy(config);
}
