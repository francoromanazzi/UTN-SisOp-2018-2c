#ifndef MDJ_H_
#define MDJ_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <stdarg.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <commons/config.h>
	#include <commons/log.h>
	#include <shared/socket.h>
	#include <shared/util.h>

	/* Constantes */
	#define IP "127.0.0.1"

	#define CONFIG_PATH "../configs/MDJ.txt"
	#define LOG_DIRECTORY_PATH "../logs/"
	#define LOG_PATH "../logs/MDJ.log"


	/* Escructuras de datos */
	enum keys{
		PUERTO, PUNTO_MONTAJE, RETARDO, TAMANIO_BLOQUES, CANTIDAD_BLOQUES
	};


	/* Variables globales */
	t_config* config;
	t_log* logger;
	int listenning_socket;

	char* datosConfigMDJ[5];
	int transfer_size;

	int mdj_send(int socket, e_tipo_msg tipo_msg, ...);
	int mdj_manejador_de_eventos(int socket, t_msg* msg);

	void crearEstructuras();
	//crearArchivo
	//agrega al principio del path "../"
		void crearArchivo(char* path,int cantidadLineas);
	//borrarArchivo
	//agrega al principio del path "../"
		void borrarArchivo(char* path);
	int validarArchivo(char* path);
	void mdj_exit();

#endif /* MDJ_H_ */
