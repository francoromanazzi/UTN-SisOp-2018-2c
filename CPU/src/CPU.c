#include "CPU.h"

int main(void) {
	config_create_fixed("/home/utnso/workspace/tp-2018-2c-RegorDTs/configs/CPU.txt");
	mkdir("/home/utnso/workspace/tp-2018-2c-RegorDTs/logs",0777);
	logger = log_create("/home/utnso/workspace/tp-2018-2c-RegorDTs/logs/CPU.log", "CPU", false, LOG_LEVEL_TRACE);
	retardo_ejecucion = config_get_int_value(config, "RETARDO");

	log_info(logger, "NUEVO CPU");

	if(!cpu_connect_to_safa()){
		log_error(logger, "No pude conectarme a SAFA");
		cpu_exit();
		return EXIT_FAILURE;
	}
	if(!cpu_connect_to_dam()){
		log_error(logger, "No pude conectarme a DAM");
		cpu_exit();
		return EXIT_FAILURE;
	}
	log_info(logger, "Me conecte a DAM");

	while(1)
		cpu_esperar_dtb();

	cpu_exit();
	return EXIT_SUCCESS;
}

void config_create_fixed(char* path){
	config = config_create(path);
	util_config_fix_comillas(&config, "IP_SAFA");
	util_config_fix_comillas(&config, "IP_DIEGO");
}

int cpu_send(int socket, e_tipo_msg tipo_msg, void* data){
	t_msg* mensaje_a_enviar;
	int ret;

	switch(tipo_msg){
		case CONEXION:
			mensaje_a_enviar = msg_create(CPU, CONEXION, (void**) 1, sizeof(int));
		break;

		case ABRIR:
			mensaje_a_enviar = empaquetar_abrir( ((t_dtb*) data)->ruta_escriptorio, ((t_dtb*) data)->gdt_id);
		break;

		case BLOCK:
		case EXIT:
		case READY:
			mensaje_a_enviar = empaquetar_dtb((t_dtb*) data);
		break;
	}
	mensaje_a_enviar->header->emisor = CPU;
	mensaje_a_enviar->header->tipo_mensaje = tipo_msg;
	ret = msg_send(socket, *mensaje_a_enviar);
	msg_free(&mensaje_a_enviar);
	return ret;
}

void cpu_esperar_dtb(){
	t_msg* msg = malloc(sizeof(t_msg));
	msg_await(safa_socket, msg);
	if(msg->header->tipo_mensaje == EXEC){
		t_dtb* dtb = desempaquetar_dtb(msg);
		if(dtb->flags.inicializado == 0) // DUMMY
			log_info(logger, "Recibi ordenes de S-AFA de ejecutar el DUMMY, el solicitante tiene ID: %d", dtb->gdt_id);
		else
			log_info(logger, "Recibi ordenes de S-AFA de ejecutar el programa con ID: %d", dtb->gdt_id);
		cpu_ejecutar_dtb(dtb);
		msg_free(&msg);
		dtb_destroy(dtb);
	}
	else if(msg->header->tipo_mensaje == DESCONEXION){ // Nunca recibe este mensaje... y tira segfault si se desconecta S-AFA :(
		log_info(logger, "Se desconecto S-AFA");
		msg_free(&msg);
	}
	else{
		log_error(logger, "No entendi el mensaje de SAFA");
		msg_free(&msg);
	}
}

void cpu_ejecutar_dtb(t_dtb* dtb){
	t_msg* mensaje_a_enviar;

	dtb_mostrar(dtb, "EXEC"); // Sacar despues

	if(dtb->flags.inicializado == 0){ // DUMMY
		/* Le pido a Diego que abra el escriptorio */
		cpu_send(dam_socket, ABRIR, (void*) dtb);

		/* Le envio a SAFA el DTB, y le pido que lo bloquee */
		cpu_send(safa_socket, BLOCK, (void*) dtb);
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
				//cpu_send(safa_socket, EXIT, (void*) dtb);
				// TODO: sacar lo de abajo y poner lo de arriba
				cpu_send(safa_socket, READY, (void*) dtb);
				free(instruccion);
				return;
			}

			/* ---------------------- 2DA FASE: DECODIFICACION ---------------------- */
			t_operacion* operacion = cpu_decodificar(instruccion);
			free(instruccion);

			/* -------------------- 3RA FASE: BUSQUEDA OPERANDOS -------------------- */
			if ((nro_error = cpu_buscar_operandos(operacion)) != OK){
				log_error(logger, "Error %d al ejecutar el DTB con ID:%d", nro_error, dtb->gdt_id);
				cpu_send(safa_socket, EXIT, (void*) dtb);
				return;
			}

			/* ------------------------- 4TA FASE: EJECUCION ------------------------- */
			if ((nro_error = cpu_ejecutar_operacion(operacion)) != OK){
				log_error(logger, "Error %d al ejecutar el DTB con ID:%d", nro_error, dtb->gdt_id);
				cpu_send(safa_socket, EXIT, (void*) dtb);
				return;
			}

			continuar_ejecucion = ++operaciones_realizadas < quantum;
		}
		cpu_send(safa_socket, READY, (void*) dtb);
	}
}

char* cpu_fetch(t_dtb* dtb, int base_escriptorio){
	char* ret = strdup("");
	return ret;
}

t_operacion* cpu_decodificar(char* instruccion){
	t_operacion* ret = malloc(sizeof(t_operacion));
	return ret;
}

int cpu_buscar_operandos(t_operacion* operacion){
	return OK;
}

int cpu_ejecutar_operacion(t_operacion* operacion){
	return OK;
}

int cpu_connect_to_safa(){
	safa_socket = socket_connect_to_server(config_get_string_value(config, "IP_SAFA"), config_get_string_value(config, "PUERTO_SAFA"));
	log_info(logger, "Intento conectarme a SAFA");

	int resultado_send = cpu_send(safa_socket, CONEXION, NULL);

	t_msg* msg = malloc(sizeof(t_msg));
	int result_recv = msg_await(safa_socket, msg);
	if(msg->header->tipo_mensaje == HANDSHAKE){
		memcpy(&quantum, msg->payload, sizeof(int));
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

void cpu_exit(){
	close(safa_socket);
	close(dam_socket);
	log_destroy(logger);
	config_destroy(config);
}
