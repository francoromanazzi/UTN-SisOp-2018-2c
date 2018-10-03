#ifndef MDJ_H_
#define MDJ_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <commons/config.h>
	#include <commons/log.h>
	#include <shared/socket.h>
	#include <shared/util.h>

	/* Constantes */
	#define IP "127.0.0.1"

	/* Variables globales */
	t_config* config;
	t_log* logger;
	int listenning_socket;
	int dam_socket;
	enum keys{
		PUERTO, PUNTO_MONTAJE, RETARDO, TAMANIO_BLOQUES, CANTIDAD_BLOQUES
	};
	char* datosConfigMDJ[5];

	void mdj_esperar_ordenes_dam();
	void config_create_fixed(char* path);
	void crearEstructuras();
	//crearArchivo
	//agrega al principio del path "../"
		void crearArchivo(char* path,int cantidadLineas);
	//borrarArchivo
	//agrega al principio del path "../"
		void borrarArchivo(char* path);
	bool validarArchivo(char* path);
	void mdj_exit();

#endif /* MDJ_H_ */
