#ifndef FM9_H_
#define FM9_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <stdarg.h>
	#include <sys/stat.h>
 	#include <sys/types.h>
	#include <unistd.h>
	#include <sys/socket.h>
	#include <pthread.h>

	#include <commons/config.h>
	#include <commons/bitarray.h>
	#include <commons/log.h>
	#include <shared/socket.h>
	#include <shared/util.h>

	#include "FM9_consola.h"

	/* Constantes */
		#define IP "127.0.0.1"

		#define CONFIG_PATH "../configs/FM9.txt"
		#define LOG_DIRECTORY_PATH "../logs/"
		#define LOG_PATH "../logs/FM9.log"

		#define FM9_ERROR_SEG_FAULT -500
		#define FM9_ERROR_INSUFICIENTE_ESPACIO -501
		#define FM9_ERROR_NO_ENCONTRADO_EN_ESTR_ADM -502


	typedef enum {SEG, TPI, SPA} e_modo;

	/* SEGMENTACION PURA */
		typedef struct {
			unsigned int pid;
			t_list* lista_tabla_segmentos; // lista de t_fila_tabla_segmento
		} t_proceso;

		typedef struct {
			unsigned int nro_seg;
			unsigned int base;
			unsigned int limite;
		} t_fila_tabla_segmento;

		t_list* lista_procesos; // lista de t_proceso
		pthread_mutex_t sem_mutex_lista_procesos; // Mutex sobre dump y operacion asignar

		t_bitarray* bitarray_lineas;
		pthread_mutex_t sem_mutex_bitarray_lineas; // Mutex sobre dump y operacion asignar

		int _fm9_dir_logica_a_fisica_seg_pura(unsigned int pid, int nro_seg, int offset, int* ok);

	/* Variables globales */
		t_config* config;
		t_log* logger;
		int listening_socket;

		/* Config: */
		e_modo modo;
		int tamanio;
		int max_linea;
		int storage_cant_lineas;
		int tam_pagina;
		int transfer_size;
		char* storage;

		pthread_mutex_t sem_mutex_realocacion_en_progreso; // Mutex sobre hilos queriendo abrir (crear) archivos en simultaneo
		pthread_mutex_t sem_mutex_escritura_en_progreso; // Mutex sobre dump y operacion asignar


	int fm9_initialize();

	int fm9_send(int socket, e_tipo_msg tipo_msg, ...);

	bool fm9_crear_nuevo_hilo(int socket_nuevo_cliente);
	void fm9_nuevo_cliente_iniciar(int socket);
	int fm9_manejar_nuevo_mensaje(int socket, t_msg* msg);

	int (*fm9_dir_logica_a_fisica)(unsigned int, int, int, int*);
	void fm9_storage_escribir(unsigned int id, int base, int offset, char* str, int* ok);
	char* fm9_storage_leer(unsigned int id, int base, int offset, int* ok);
	int fm9_storage_nuevo_archivo(unsigned int id, int* ok);
	void fm9_storage_realocar(unsigned int id, int base, int offset, int* ok); // First fit

	void fm9_exit();

#endif /* FM9_H_ */
