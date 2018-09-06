#include "S-AFA.h"

int main(void) {
	config = config_create("../../configs/S-AFA.txt");
	logger = log_create("../../logs/S-AFA.log", "S-AFA", false, LOG_LEVEL_TRACE);

	/* Empiezo en estado corrupto */
	/* Como salgo? Conexion con DAM y por lo menos un CPU */
	if((listening_socket = socket_create_listener(IP, config_get_string_value(config, "PUERTO"))) == -1){
		log_error(logger, "No pude crear el socket de escucha");
		safa_exit();
		exit(EXIT_FAILURE);
	}
	log_info(logger, "Escucho en el socket %d",listening_socket);

	listen(listening_socket, BACKLOG);
	struct sockaddr_in addr;	// Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	socklen_t addrlen = sizeof(addr);

	int nuevo_cliente_socket = accept(listening_socket, (struct sockaddr *) &addr, &addrlen);
	log_info(logger, "Recibi conexion en el socket %d", nuevo_cliente_socket);

	/* Ya sali del estado corrupto, y estoy en estado  operativo */

	/* Creo el hilo consola del gestor de programas */
	if(pthread_create( &thread_consola, NULL, (void*) gestor_consola_iniciar, NULL) ){
		log_error(logger,"No pude crear el hilo para la consola");
		safa_exit();
		exit(EXIT_FAILURE);
	}
	log_info(logger, "Creo el hilo para la consola");
	pthread_detach(thread_consola);


	/* TODO: Gestionar las conexiones. select() ??? */
	for(;;);
	safa_exit();
	return EXIT_SUCCESS;
}

void safa_exit(void){
	close(listening_socket);
	close(nuevo_cliente_socket);
	log_destroy(logger);
	config_destroy(config);
}

