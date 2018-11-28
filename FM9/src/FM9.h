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
	#include <math.h>

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
			unsigned int nro_seg;
			unsigned int base;
			unsigned int limite;
		} t_fila_tabla_segmento_SEG;

		t_list* lista_procesos; // lista de t_proceso
		pthread_mutex_t sem_mutex_lista_procesos; // Mutex sobre dump y operacion asignar

		t_list* lista_huecos_storage; // lista de t_vector2
		pthread_mutex_t sem_mutex_lista_huecos_storage; // Mutex sobre dump y operacion asignar

		int _SEG_dir_logica_a_fisica(unsigned int pid, int nro_seg, int offset, int* ok, bool permisos_totales);
		int _SEG_best_fit(int cant_lineas);
		void _SEG_nuevo_hueco_disponible(int linea_inicio, int linea_fin);
		void _SEG_close(unsigned int id, int base, int* ok);
		void _SEG_liberar_memoria_proceso(unsigned int id);


/* TABLA DE PAGINAS INVERTIDA */
		typedef struct {
			unsigned int pid;
			unsigned int nro_pagina;
			unsigned int bit_validez : 1;
		} t_fila_tabla_paginas_invertida;

		t_list* tabla_paginas_invertida; // lista de t_fila_tabla_paginas_invertida
		pthread_mutex_t sem_mutex_tabla_paginas_invertida;

		typedef struct{
			unsigned int pid;
			unsigned int nro_pag_inicial;
			unsigned int nro_pag_final;
			unsigned int fragm_interna_ult_pagina;
		} t_fila_TPI_archivos;

		t_list* tabla_archivos_TPI; // lista de t_fila_TPI_archivos
		pthread_mutex_t sem_mutex_tabla_archivos_TPI;

		int _TPI_dir_logica_a_fisica(unsigned int pid, int pag_inicial, int offset, int* ok, bool permisos_totales);
		void _TPI_close(unsigned int id, int base, int* ok);
		void _TPI_liberar_memoria_proceso(unsigned int id);
		unsigned int _TPI_incrementar_ultima_pagina_archivo_de_proceso(unsigned int pid, int pag_inicial);
		unsigned int _TPI_primera_pagina_disponible_proceso(unsigned int pid);


/* SEGMENTACION PAGINADA */
		typedef struct {
			unsigned int nro_seg;
			t_list* lista_tabla_paginas; // lista de t_fila_tabla_paginas_SPA
			unsigned int frag_interna_ult_pagina; // cantidad de lineas perdidas en la ult pag del segmento
		} t_fila_tabla_segmento_SPA;

		typedef struct {
			unsigned int nro_frame;
			unsigned int nro_pagina;
		} t_fila_tabla_paginas_SPA;

		int _SPA_dir_logica_a_fisica(unsigned int pid, int nro_seg, int offset, int* ok, bool permisos_totales);
		void _SPA_close(unsigned int id, int base, int* ok);
		void _SPA_liberar_memoria_proceso(unsigned int id);


/* SEGMENTACION PURA x SEGMENTACION PAGINADA */
		typedef struct {
			unsigned int pid;
			t_list* lista_tabla_segmentos; // lista de t_fila_tabla_segmento_{SEG|SPA}
		} t_proceso;


/* TABLA DE PAGINAS INVERTIDA x SEGMENTACION PAGINADA */
		t_bitarray* bitmap_frames;
		pthread_mutex_t sem_mutex_bitmap_frames;

		int _TPI_SPA_obtener_frame_libre();


/* Variables globales */
		t_config* config;
		t_log* logger;
		int listening_socket;


/* Config */
		e_modo modo;
		int tamanio;
		int max_linea;
		int storage_cant_lineas;
		int tam_frame_bytes, tam_frame_lineas;
		int cant_frames;
		char* storage;


	int fm9_initialize();

	int fm9_send(int socket, e_tipo_msg tipo_msg, ...);

	bool fm9_crear_nuevo_hilo(int socket_nuevo_cliente);
	void fm9_nuevo_cliente_iniciar(int socket);
	int fm9_manejar_nuevo_mensaje(int socket, t_msg* msg);

	int (*fm9_dir_logica_a_fisica)(unsigned int id, int base, int offset, int* ok, bool permisos_totales);
	void fm9_storage_escribir(unsigned int id, int base, int offset, char* str, int* ok, bool permisos_totales);
	char* fm9_storage_leer(unsigned int id, int base, int offset, int* ok, bool permisos_totales);
	int fm9_storage_nuevo_archivo(unsigned int id, int* ok);
	void fm9_storage_realocar(unsigned int id, int base, int offset, int* ok);
	void (*fm9_close)(unsigned int id, int base, int* ok);
	void (*fm9_liberar_memoria_proceso)(unsigned int id);

	void fm9_exit();

#endif /* FM9_H_ */
