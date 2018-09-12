#include "DAM.h"

int main(void) {
	config_create_fixed("../../configs/DAM.txt");
	mkdir("../../logs",0777);
	logger = log_create("../../logs/DAM.log", "DAM", false, LOG_LEVEL_TRACE);

	/* Me conecto a S-AFA, FM9 y MDJ */
	if(!dam_connect_to_safa()) log_error(logger, "No pude conectarme a SAFA");
	else log_info(logger, "Me conecte a SAFA");

	if(!dam_connect_to_mdj()) log_error(logger, "No pude conectarme a MDJ");
	else log_info(logger, "Me conecte a MDJ");

	if(!dam_connect_to_fm9()) log_error(logger, "No pude conectarme a FM9");
	else log_info(logger, "Me conecte a FM9");

	/* Me quedo esperando conexiones de CPU y/o pedidos de S-AFA */
	if((listenning_socket = socket_create_listener(IP, config_get_string_value(config, "PUERTO"))) == -1){
		log_error(logger, "No pude crear el socket de escucha");
		dam_exit();
		exit(EXIT_FAILURE);
	}
	log_info(logger, "Escucho en el socket %d", listenning_socket);

	socket_start_listening_select(listenning_socket, dam_manejador_de_eventos);

	dam_exit();
	return EXIT_SUCCESS;
}

int dam_manejador_de_eventos(int socket, t_msg* msg){
	log_info(logger, "EVENTO: Emisor: %d, Tipo: %d, Tamanio: %d, Mensaje: %s",msg->header->emisor,msg->header->tipo_mensaje,msg->header->payload_size,(char*) msg->payload);
	char* path;
	unsigned int id;

	if(msg->header->emisor == CPU){
		switch(msg->header->tipo_mensaje){
			case CONEXION:
				log_info(logger, "Se conecto CPU");
			break;

			case DESCONEXION:
				log_info(logger, "Se desconecto CPU");
			break;

			case ABRIR:
				desempaquetar_abrir(msg,&path,&id);
				log_info(logger, "Ehhh, voy a buscar %s para %d", path, id);

				// TODO: preguntar a MDJ, recibir de MDJ, escribir en FM9, recibir resultado de FM9
				sleep(2); // Sacarlo despues

				t_msg* msg_a_enviar = empaquetar_resultado_abrir(1, id, path, 21198); // HARDCODEADO A MAS NO PODER
				msg_a_enviar->header->emisor = DAM;
				msg_a_enviar->header->tipo_mensaje = RESULTADO_ABRIR;
				msg_send(safa_socket, *msg_a_enviar);
				msg_free(&msg_a_enviar);
			break;

			case FLUSH:
				log_info(logger, "Iniciando operacion FLUSH");
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

int dam_connect_to_safa(){
	safa_socket = socket_connect_to_server(config_get_string_value(config, "IP_SAFA"), config_get_string_value(config, "PUERTO_SAFA"));
	t_msg* mensaje_a_enviar = malloc(sizeof(t_msg));
	mensaje_a_enviar = msg_create(DAM, CONEXION, (void**) 1, sizeof(int));
	int resultado_send = msg_send(safa_socket, *mensaje_a_enviar);
	msg_free(&mensaje_a_enviar);
	return safa_socket > 0 && resultado_send > 0;
}

int dam_connect_to_mdj(){
	mdj_socket  = socket_connect_to_server(config_get_string_value(config, "IP_MDJ"), config_get_string_value(config, "PUERTO_MDJ"));
	t_msg* mensaje_a_enviar = malloc(sizeof(t_msg));
	mensaje_a_enviar = msg_create(DAM, CONEXION, (void**) 1, sizeof(int));
	int resultado_send = msg_send(mdj_socket, *mensaje_a_enviar);
	msg_free(&mensaje_a_enviar);
	return safa_socket > 0 && resultado_send > 0;
}

int dam_connect_to_fm9(){
	fm9_socket  = socket_connect_to_server(config_get_string_value(config, "IP_FM9"), config_get_string_value(config, "PUERTO_FM9"));
	t_msg* mensaje_a_enviar = malloc(sizeof(t_msg));
	mensaje_a_enviar = msg_create(DAM, CONEXION, (void**) 1, sizeof(int));
	int resultado_send = msg_send(fm9_socket, *mensaje_a_enviar);
	msg_free(&mensaje_a_enviar);
	return safa_socket > 0 && resultado_send > 0;
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
