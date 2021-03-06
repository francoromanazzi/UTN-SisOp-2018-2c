#include "FM9_consola.h"

static void _SEG_estr_adm_huecos_print_y_log();
static void _SEG_estr_adm_tabla_segmentos_proceso_print_y_log(t_proceso* proceso);

static void _TPI_SPA_estr_adm_bitmap_frames_print_y_log();

static void _SPA_estr_adm_tabla_segmentos_proceso_print_y_log(t_proceso* proceso);


void fm9_consola_init(){
	char* linea;

	rl_attempted_completion_function = (CPPFunction *) fm9_consola_autocompletar;

	printf("Bienvenido! Ingrese \"ayuda\" para ver una lista con todos los comandos disponibles \n");
	while(1) {
		linea = readline("FM9> ");
		if(linea)
			add_history(linea);
		if(!strncmp(linea, "salir", 5)) {
			free(linea);
			break;
		}
		fm9_procesar_comando(linea);
		free(linea);
	}
	exit(EXIT_SUCCESS);
}

void fm9_procesar_comando(char* linea){
	char** argv = string_split(linea, " ");
	int argc = split_cant_elem(argv);

	/* Comando ayuda */
	if(argc == 1 && !strcmp(argv[0], "ayuda")){
		printf("\ndump:\t\tMuestra todas las lineas del storage, sin estructuras administrativas\n");
		printf("dump [pcb_id]:\tMuestra las lineas usadas por un proceso, y sus estructuras administrativas\n");
		printf("dump e:\tMuestra el estado de las estructuras administrativas\n");
		printf("clear:\t\tLimpia la pantalla\n");
		printf("salir:\t\tCierra la consola\n\n");
	}

	/* Comando DUMP */
	else if(argc == 1 && !strcmp(argv[0], "dump")){ // Muestra su storage completo, sin estructuras administrativas
		int i;
		char* str;
		log_info(logger, "~~~~~~~~~~ DUMP ~~~~~~~~~~");
		for(i = 0; i < storage_cant_lineas; i++){
			str = malloc(max_linea);
			memcpy((void*) str, (void*) storage + (i * max_linea), max_linea);
			log_info(logger, "%d: %s", i, str);
			printf("%d: %s\n", i, str);
			free(str);
		}
		printf("\n");
	}

	/* Comando DUMP e */ //TODO
	else if(argc == 2 && !strcmp(argv[0], "dump") && !strcmp(argv[1], "e")){ // Muestra todas las estructuras administrativas
		log_info(logger, "~~~~~~~~~~ DUMP e ~~~~~~~~~~", argv[1]);
		fm9_dump_estructuras();
	}

	/* Comando DUMP [pid]*/
	else if(argc == 2 && !strcmp(argv[0], "dump")){
		log_info(logger, "~~~~~~~~~~ DUMP [pid: %s] ~~~~~~~~~~", argv[1]);
		fm9_dump_pid(atoi(argv[1]));
	}

	/* Comando clear */
	else if(argc == 1 && !strcmp(argv[0], "clear")){
		system("clear");
	}

	/* Comando no reconocido */
	else{
		printf("No se pudo reconocer el comando\n\n");
	}
	split_liberar(argv);
}

static void _SEG_estr_adm_huecos_print_y_log(){
	char* huecos_str = string_new(), *aux;
	int i;

	log_info(logger, "Lista de huecos: ");
	printf("Lista de huecos: \n");

	for(i = 0; i < list_size(lista_huecos_storage); i++){
		t_vector2* hueco = (t_vector2*) list_get(lista_huecos_storage, i);
		string_append(&huecos_str, "(");
		aux = string_itoa(hueco->x);
		string_append(&huecos_str, aux);
		free(aux);
		string_append(&huecos_str, ",");
		aux = string_itoa(hueco->y);
		string_append(&huecos_str, aux);
		free(aux);
		string_append(&huecos_str, ")");
		string_append(&huecos_str, ",");
	}
	if(i > 0) huecos_str[strlen(huecos_str) - 1] = '\0'; // Saco la ultima coma

	log_info(logger, "\t[%s]", huecos_str);
	printf("\t[%s]\n", huecos_str);
	free(huecos_str);
}

static void _SEG_estr_adm_tabla_segmentos_proceso_print_y_log(t_proceso* proceso){

	void __SEG_estr_adm_fila_tabla_segmento_print_y_log(void* _fila_tabla){
		t_fila_tabla_segmento_SEG* fila_tabla = (t_fila_tabla_segmento_SEG*) _fila_tabla;
		log_info(logger, "\tNroSeg: %d Base: %d Limite: %d", fila_tabla->nro_seg, fila_tabla->base, fila_tabla->limite);
		printf("\tNroSeg: %d Base: %d Limite: %d\n", fila_tabla->nro_seg, fila_tabla->base, fila_tabla->limite);
	}


	log_info(logger, "Tabla de segmentos del proceso %d:", proceso->pid);
	printf("Tabla de segmentos del proceso %d: \n", proceso->pid);
	list_iterate(proceso->lista_tabla_segmentos, __SEG_estr_adm_fila_tabla_segmento_print_y_log);
}


void _SEG_dump_pid(unsigned int pid){

	bool _mismo_id(void* proceso){
		return ((t_proceso*) proceso)->pid == pid;
	}

	pthread_mutex_lock(&sem_mutex_lista_procesos);
	t_proceso* proceso = list_find(lista_procesos, _mismo_id);

	int i = 0;

	void _estr_adm_proceso_print_y_log(){

		pthread_mutex_lock(&sem_mutex_lista_huecos_storage);
		_SEG_estr_adm_huecos_print_y_log();
		pthread_mutex_unlock(&sem_mutex_lista_huecos_storage);

		_SEG_estr_adm_tabla_segmentos_proceso_print_y_log(proceso);
	}

	void _fila_tabla_segmento_print_y_log(void* _fila_tabla){
		t_fila_tabla_segmento_SEG* fila_tabla = ((t_fila_tabla_segmento_SEG*) _fila_tabla);
		int i, ok;
		char* linea;

		log_info(logger,"Segmento %d", fila_tabla->nro_seg);
		printf("Segmento %d\n", fila_tabla->nro_seg);

		for(i = 0; i <= fila_tabla->limite; i++){
			linea = fm9_storage_leer(proceso->pid, fila_tabla->nro_seg, i, &ok, false);
			log_info(logger,"\t%d: %s", i, linea);
			printf("\t%d: %s\n", i, linea);
			free(linea);
		}
	}


	if(proceso == NULL){
		pthread_mutex_unlock(&sem_mutex_lista_procesos);
		printf("No se encontro dicho proceso\n\n");
		log_info(logger, "No se encontro dicho proceso");
		return;
	}

	/* Imprimo estructuras administrativas */
	log_info(logger, "~~~~~~~~~~ ESTR ADM ~~~~~~~~~~");
	printf("~~~~~~~~~~ ESTR ADM ~~~~~~~~~~\n");
	_estr_adm_proceso_print_y_log();

	/* Imprimo lineas de los segmentos del proceso */
	log_info(logger, "~~~~~~~~~~ LINEAS ~~~~~~~~~~");
	printf("\n~~~~~~~~~~ LINEAS ~~~~~~~~~~\n");

	if(proceso->lista_tabla_segmentos->elements_count <= 0){
		log_info(logger, "No tiene datos asociados");
		printf("No tiene datos asociados\n\n");
	}

	pthread_mutex_unlock(&sem_mutex_lista_procesos);

	list_iterate(proceso->lista_tabla_segmentos, _fila_tabla_segmento_print_y_log);
}

void _SEG_dump_estructuras(){
	pthread_mutex_lock(&sem_mutex_lista_huecos_storage);
	_SEG_estr_adm_huecos_print_y_log();
	pthread_mutex_unlock(&sem_mutex_lista_huecos_storage);

	pthread_mutex_lock(&sem_mutex_lista_procesos);
	list_iterate(lista_procesos, (void (*)(void*)) _SEG_estr_adm_tabla_segmentos_proceso_print_y_log);
	pthread_mutex_unlock(&sem_mutex_lista_procesos);
}


void _TPI_dump_pid(unsigned int id){

	int nro_frame = 0;

	bool _mismo_pid(void* pag){
		return ((t_fila_tabla_paginas_invertida*) pag)->bit_validez == 1 && ((t_fila_tabla_paginas_invertida*) pag)->pid == id;
	}

	void _TPI_estr_adm_pid_print_y_log(unsigned int pid){

	}

	void __estr_adm_pagina_print_y_log(void* pagina){
		if(_mismo_pid(pagina)){
			t_fila_tabla_paginas_invertida* pag = (t_fila_tabla_paginas_invertida*) pagina;
			log_info(logger, "NRO_FRAME: %d, NRO_PAG: %d, VALIDEZ: %d", nro_frame, pag->nro_pagina, pag->bit_validez);
			printf("NRO_FRAME: %d, NRO_PAG: %d, VALIDEZ: %d\n", nro_frame, pag->nro_pagina, pag->bit_validez);
		}
		nro_frame++;
	}

	void __lineas_pagina_print_y_log(void* pagina){

		bool ___mismo_archivo(void* arch){
			return ((t_fila_TPI_archivos*) arch)->pid == id
					&& ((t_fila_tabla_paginas_invertida*) pagina)->nro_pagina >= ((t_fila_TPI_archivos*) arch)->nro_pag_inicial
					&& ((t_fila_tabla_paginas_invertida*) pagina)->nro_pagina <= ((t_fila_TPI_archivos*) arch)->nro_pag_final
					&& ((t_fila_tabla_paginas_invertida*) pagina)->bit_validez == 1;
		}

		if(_mismo_pid(pagina)){
			t_fila_tabla_paginas_invertida* pag = (t_fila_tabla_paginas_invertida*) pagina;
			log_info(logger, "NRO_PAG: %d", pag->nro_pagina);
			printf("NRO_PAG: %d\n", pag->nro_pagina);

			pthread_mutex_lock(&sem_mutex_tabla_archivos_TPI);
			int nro_pag_inicial_base = ((t_fila_TPI_archivos*) list_find(tabla_archivos_TPI, ___mismo_archivo))->nro_pag_inicial;
			pthread_mutex_unlock(&sem_mutex_tabla_archivos_TPI);

			int i, ok;
			char* linea;
			for(i = 0; i < tam_frame_lineas; i++){
				pthread_mutex_unlock(&sem_mutex_tabla_paginas_invertida);
				linea = fm9_storage_leer(id, nro_pag_inicial_base, i + (pag->nro_pagina * tam_frame_lineas), &ok, false);
				pthread_mutex_lock(&sem_mutex_tabla_paginas_invertida);

				log_info(logger,"\t%d: %s", i, linea);
				printf("\t%d: %s\n", i, linea);
				free(linea);
			}
		}
	}


	pthread_mutex_lock(&sem_mutex_tabla_paginas_invertida);
	if((list_find(tabla_paginas_invertida, _mismo_pid)) == NULL) {
		pthread_mutex_unlock(&sem_mutex_tabla_paginas_invertida);
		printf("No se encontro dicho proceso\n\n");
		log_info(logger, "No se encontro dicho proceso");
		return;
	}

	log_info(logger, "~~~~~~~~~~ ESTR ADM ~~~~~~~~~~");
	printf("~~~~~~~~~~ ESTR ADM ~~~~~~~~~~\n");

	pthread_mutex_lock(&sem_mutex_bitmap_frames);
	_TPI_SPA_estr_adm_bitmap_frames_print_y_log();
	pthread_mutex_unlock(&sem_mutex_bitmap_frames);

	list_iterate(tabla_paginas_invertida, __estr_adm_pagina_print_y_log);

	log_info(logger, "~~~~~~~~~~ LINEAS ~~~~~~~~~~");
	printf("\n~~~~~~~~~~ LINEAS ~~~~~~~~~~\n");


	list_iterate(tabla_paginas_invertida, __lineas_pagina_print_y_log);

	pthread_mutex_unlock(&sem_mutex_tabla_paginas_invertida);

	printf("\n");
}

void _TPI_dump_estructuras(){
	int i = 0;

	void __pagina_print_y_log(void* _pagina) {
		t_fila_tabla_paginas_invertida* pagina = (t_fila_tabla_paginas_invertida*) _pagina;
		printf("PID: %d, NRO_PAG: %d, NRO_FRAME: %d, VALIDEZ: %d\n", pagina->pid, pagina->nro_pagina, i, pagina->bit_validez);
		log_info(logger, "PID: %d, NRO_PAG: %d, NRO_FRAME: %d, VALIDEZ: %d", pagina->pid, pagina->nro_pagina, i, pagina->bit_validez);
		i++;
	}


	pthread_mutex_lock(&sem_mutex_bitmap_frames);
	_TPI_SPA_estr_adm_bitmap_frames_print_y_log();
	pthread_mutex_unlock(&sem_mutex_bitmap_frames);

	log_info(logger, "Tabla de paginas invertida:");
	printf("Tabla de paginas invertida: \n");

	pthread_mutex_lock(&sem_mutex_tabla_paginas_invertida);
	list_iterate(tabla_paginas_invertida, __pagina_print_y_log);
	pthread_mutex_unlock(&sem_mutex_tabla_paginas_invertida);

	printf("\n");
}


static void _TPI_SPA_estr_adm_bitmap_frames_print_y_log(){
	char* bitmap_str = string_new();
	int i;

	for(i = 0; i < cant_frames; i++){
		string_append(&bitmap_str, bitarray_test_bit(bitmap_frames, i) ? "1" : "0");
		string_append(&bitmap_str, ", ");
	}

	if(i > 0) bitmap_str[strlen(bitmap_str) - 2] = '\0'; // Saco la ultima coma

	log_info(logger, "Bitmap de frames:");
	printf("Bitmap de frames:\n");
	log_info(logger, "\t[%s]", bitmap_str);
	printf("\t[%s]\n", bitmap_str);
	free(bitmap_str);
}

static void _SPA_estr_adm_tabla_segmentos_proceso_print_y_log(t_proceso* proceso){

	void __SPA_estr_adm_fila_tabla_segmento_print_y_log(void* _fila_tabla){

		void ___SPA_estr_adm_fila_tabla_paginas_print_y_log(void* _fila_tabla_pag){
			t_fila_tabla_paginas_SPA* fila_tabla_pag = (t_fila_tabla_paginas_SPA*) _fila_tabla_pag;
			log_info(logger, "\t\tNro pag: %d, Nro frame: %d", fila_tabla_pag->nro_pagina, fila_tabla_pag->nro_frame);
			printf("\t\tNro pag: %d, Nro frame: %d\n", fila_tabla_pag->nro_pagina, fila_tabla_pag->nro_frame);
		}


		t_fila_tabla_segmento_SPA* fila_tabla = (t_fila_tabla_segmento_SPA*) _fila_tabla;
		log_info(logger, "\tSegmento: %d", fila_tabla->nro_seg);
		printf("\tSegmento: %d\n", fila_tabla->nro_seg);

		list_iterate(fila_tabla->lista_tabla_paginas, ___SPA_estr_adm_fila_tabla_paginas_print_y_log);
	}


	log_info(logger, "Tabla de segmentos del proceso %d:", proceso->pid);
	printf("Tabla de segmentos del proceso %d: \n", proceso->pid);
	list_iterate(proceso->lista_tabla_segmentos, __SPA_estr_adm_fila_tabla_segmento_print_y_log);
}

void _SPA_dump_pid(unsigned int pid){

	bool _mismo_id(void* proceso){
		return ((t_proceso*) proceso)->pid == pid;
	}

	pthread_mutex_lock(&sem_mutex_lista_procesos);
	t_proceso* proceso = list_find(lista_procesos, _mismo_id);


	void _estr_adm_proceso_print_y_log(){
		pthread_mutex_lock(&sem_mutex_bitmap_frames);
		_TPI_SPA_estr_adm_bitmap_frames_print_y_log();
		pthread_mutex_unlock(&sem_mutex_bitmap_frames);

		_SPA_estr_adm_tabla_segmentos_proceso_print_y_log(proceso);
	}

	void _fila_tabla_segmento_print_y_log(void* _fila_tabla){
		t_fila_tabla_segmento_SPA* fila_tabla = ((t_fila_tabla_segmento_SPA*) _fila_tabla);


		void __fila_tabla_paginas_print_y_log(void* _fila_tabla_pag){
			int i, ok;
			char* linea;

			t_fila_tabla_paginas_SPA* fila_tabla_pag = (t_fila_tabla_paginas_SPA*) _fila_tabla_pag;
			log_info(logger, "\tNro pag: %d, Nro frame: %d", fila_tabla_pag->nro_pagina, fila_tabla_pag->nro_frame);
			printf("\tNro pag: %d, Nro frame: %d\n", fila_tabla_pag->nro_pagina, fila_tabla_pag->nro_frame);

			for(i = 0; i < tam_frame_lineas; i++){
				linea = fm9_storage_leer(proceso->pid, fila_tabla->nro_seg, i + (fila_tabla_pag->nro_pagina * tam_frame_lineas), &ok, false);
				log_info(logger,"\t\t%d: %s", i, linea);
				printf("\t\t%d: %s\n", i, linea);
				free(linea);
			}
		}


		log_info(logger, "Segmento: %d", fila_tabla->nro_seg);
		printf("Segmento: %d\n", fila_tabla->nro_seg);

		list_iterate(fila_tabla->lista_tabla_paginas, __fila_tabla_paginas_print_y_log);
	}


	if(proceso == NULL){
		pthread_mutex_unlock(&sem_mutex_lista_procesos);
		printf("No se encontro dicho proceso\n\n");
		log_info(logger, "No se encontro dicho proceso");
		return;
	}

	/* Imprimo estructuras administrativas */
	log_info(logger, "~~~~~~~~~~ ESTR ADM ~~~~~~~~~~");
	printf("~~~~~~~~~~ ESTR ADM ~~~~~~~~~~\n");
	_estr_adm_proceso_print_y_log();

	/* Imprimo lineas de los segmentos del proceso */
	log_info(logger, "~~~~~~~~~~ LINEAS ~~~~~~~~~~");
	printf("\n~~~~~~~~~~ LINEAS ~~~~~~~~~~\n");
	if(proceso->lista_tabla_segmentos->elements_count <= 0){
		log_info(logger, "No tiene datos asociados");
		printf("No tiene datos asociados\n\n");
	}

	pthread_mutex_unlock(&sem_mutex_lista_procesos);

	list_iterate(proceso->lista_tabla_segmentos, _fila_tabla_segmento_print_y_log);
}

void _SPA_dump_estructuras(){
	pthread_mutex_lock(&sem_mutex_bitmap_frames);
	_TPI_SPA_estr_adm_bitmap_frames_print_y_log();
	pthread_mutex_unlock(&sem_mutex_bitmap_frames);

	pthread_mutex_lock(&sem_mutex_lista_procesos);
	list_iterate(lista_procesos, (void (*)(void*)) _SPA_estr_adm_tabla_segmentos_proceso_print_y_log);
	pthread_mutex_unlock(&sem_mutex_lista_procesos);
}


// AUTOCOMPLETAR:

/* Attempt to complete on the contents of TEXT.  START and END show the
   region of TEXT that contains the word to complete.  We can use the
   entire line in case we want to do some simple parsing.  Return the
   array of matches, or NULL if there aren't any. */
char** fm9_consola_autocompletar (text, start, end)
     char *text;
     int start, end;
{
  char **matches;

  matches = (char **)NULL;

  /* If this word is at the start of the line, then it is a command
     to complete.  Otherwise it is the name of a file in the current
     directory. */
  if (start == 0)
    matches = completion_matches (text, fm9_consola_autocompletar_command_generator);

  return (matches);
}

/* Generator function for command completion.  STATE lets us know whether to start
   from scratch; without any state (i.e. STATE == 0), then we start at the top of the list. */
char* fm9_consola_autocompletar_command_generator (text, state)
     char *text;
     int state;
{
	static int list_index, len;
	char *name;
	static char* commands[] = {"ayuda", "dump", "clear", "salir", (char*) NULL};

	/* If this is a new word to complete, initialize now.  This includes
	 saving the length of TEXT for efficiency, and initializing the index
	 variable to 0. */
	if(!state){
	  list_index = 0;
	  len = strlen (text);
	}

	/* Return the next name which partially matches from the command list. */
	while ((name = commands[list_index]) != NULL){
	  list_index++;

	  if (strncmp (name, text, len) == 0)
		return (strdup(name));
	}

	/* If no names matched, then return NULL. */
	return ((char *)NULL);
}








