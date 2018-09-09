#include "FM9.h"

int main(void) {
	config_create("../../configs/FM9.txt");
	mkdir("../../logs",0777);
	logger = log_create("../../logs/FM9.log", "FM9", true, LOG_LEVEL_TRACE);

	listening_socket = socket_create_listener(IP, config_get_string_value(config, "PUERTO"));
	if(listening_socket<0){
		log_info(logger,"No se pudo crear socket de escucha");
		close(listening_socket);
		exit(1);
	}

	log_info(logger, "Comienzo a escuchar  por el socket%i",listening_socket);

		listen(listening_socket, BACKLOG);

		struct sockaddr_in dirCliente;
		socklen_t dirlong= sizeof(struct sockaddr_in);


		int nuevo_cliente_socket = accept(listening_socket,(struct sockaddr *) &dirCliente,&dirlong);
		if(nuevo_cliente_socket<0){
			log_info(logger, "Error en Socket cliente");
			fm9_exit();
			exit(1);
		}

		log_info(logger, "Recibi conexion en el socket %d", nuevo_cliente_socket);

	fm9_exit();

	return EXIT_SUCCESS;
}

void fm9_exit(){
	close(listening_socket);
	close(nuevo_cliente_socket);
	log_destroy(logger);
	config_destroy(config);
}
