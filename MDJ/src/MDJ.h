#ifndef MDJ_H_
#define MDJ_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <stdarg.h>
	#include <sys/stat.h>
	#include <sys/mman.h>
	#include <sys/types.h>
	#include <limits.h>
	#include <errno.h>
	#include <pthread.h>
	#include <assert.h>

	#include <commons/config.h>
	#include <commons/log.h>
	#include <commons/bitarray.h>

	#include <shared/socket.h>
	#include <shared/util.h>
	#include "MDJ_consola.h"


	/* Constantes */
	#define CONFIG_PATH "../configs/MDJ.txt"
	#define LOG_DIRECTORY_PATH "../logs/"
	#define LOG_PATH "../logs/MDJ.log"

	#define MDJ_ERROR_PATH_INEXISTENTE -600
	#define MDJ_ERROR_ARCHIVO_YA_EXISTENTE -601
	#define MDJ_ERROR_ARCHIVO_NO_EXISTENTE -602
	#define MDJ_ERROR_ESPACIO_INSUFICIENTE -603

	/* Escructuras de datos */
	typedef enum{METADATA, ARCHIVOS, BLOQUES} e_key_path_estructura;

	/* Variables globales */
		t_config* config;
		t_config* config_metadata;
		t_log* logger;
		int listenning_socket;
		t_bitarray* bitmap;
		FILE* f_bitmap; // Usado por mdj_bitmap_save
		char* paths_estructuras[3]; // e_key_path_estructura
		int cant_bloques;
		int tam_bloques;

	void crearEstructuras();

	int mdj_send(int socket, e_tipo_msg tipo_msg, ...);
	int mdj_manejador_de_eventos(int socket, t_msg* msg);

	void validarArchivo(char* path, int* ok);
	void crearArchivo(char* path, int cant_lineas, int* ok);
	void obtenerDatos(char* path, int offset, int bytes_restantes, void** ret_buffer, int* ret_buffer_size);
	void guardarDatos(char* path, int offset, int size, void* buffer, int* ok);
	void borrarArchivo(char* path, int* ok);

	void mdj_exit();

#endif /* MDJ_H_ */
