#include "CPU.h"

int main(void) {
	config_create_fixed("../../configs/CPU.txt");
	mkdir("../../logs",0777);
	logger = log_create("../../logs/CPU.log", "CPU", false, LOG_LEVEL_TRACE);

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

	for(;;) sleep(10);
	cpu_exit();
	return EXIT_SUCCESS;
}

void config_create_fixed(char* path){
	config = config_create(path);
	util_config_fix_comillas(&config, "IP_SAFA");
	util_config_fix_comillas(&config, "IP_DIEGO");
}

int cpu_connect_to_safa(){
	safa_socket = socket_connect_to_server(config_get_string_value(config, "IP_SAFA"), config_get_string_value(config, "PUERTO_SAFA"));
	log_info(logger, "Intento conectarme a SAFA");

	t_msg* mensaje_a_enviar = malloc(sizeof(t_msg));
	mensaje_a_enviar = msg_create(CPU, CONEXION, (void**) 1, sizeof(int));
	int resultado_send = msg_send(safa_socket, *mensaje_a_enviar);
	msg_free(&mensaje_a_enviar);

	t_msg* msg = malloc(sizeof(t_msg));
	int result_recv = msg_await(safa_socket, msg);
	if(msg->header->tipo_mensaje == HANDSHAKE){
		memcpy(&quantum, msg->payload, sizeof(int));
		log_info(logger, "Recibi handshake de SAFA, y ahora mi quantum es de %d", quantum);
		msg_free(&msg);
	}
	else{
		msg_free(&msg);
		return -1;
	}
	return safa_socket > 0 && resultado_send > 0 && result_recv > 0;
}

int cpu_connect_to_dam(){
	dam_socket = socket_connect_to_server(config_get_string_value(config, "IP_DIEGO"), config_get_string_value(config, "PUERTO_DIEGO"));
	log_info(logger, "Intento conectarme a DAM");
	t_msg* mensaje_a_enviar = malloc(sizeof(t_msg));
	mensaje_a_enviar = msg_create(CPU, CONEXION, (void**) 1, sizeof(int));
	int resultado_send = msg_send(dam_socket, *mensaje_a_enviar);
	msg_free(&mensaje_a_enviar);

	return safa_socket > 0 && resultado_send > 0;
}

void cpu_exit(){
	close(safa_socket);
	close(dam_socket);
	log_destroy(logger);
	config_destroy(config);
}
