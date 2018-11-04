#include "FM9_consola.h"

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


void _fm9_dump_pid_seg_pura(unsigned int pid){

	bool _mismo_id(void* proceso){
		return ((t_proceso*) proceso)->pid == pid;
	}

	pthread_mutex_lock(&sem_mutex_lista_procesos);
	t_proceso* proceso = list_find(lista_procesos, _mismo_id);

	int i = 0;

	void _estr_adm_proceso_print_y_log(){

		void _estr_adm_fila_tabla_segmento_print_y_log(void* _fila_tabla){
			t_fila_tabla_segmento* fila_tabla = (t_fila_tabla_segmento*) _fila_tabla;
			log_info(logger, "\tNroSeg: %d Base: %d Limite: %d", fila_tabla->nro_seg, fila_tabla->base, fila_tabla->limite);
			printf("\tNroSeg: %d Base: %d Limite: %d\n", fila_tabla->nro_seg, fila_tabla->base, fila_tabla->limite);
		}

		void _huecos_print_y_log(){
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
			if(i > 0) huecos_str[strlen(huecos_str) - 1] = '\0';

			log_info(logger, "\t[%s]", huecos_str);
			printf("\t[%s]\n", huecos_str);
			free(huecos_str);
		}

		pthread_mutex_lock(&sem_mutex_lista_huecos_storage);
		_huecos_print_y_log();
		pthread_mutex_unlock(&sem_mutex_lista_huecos_storage);

		log_info(logger, "Tabla de segmentos del proceso %d:", proceso->pid);
		printf("Tabla de segmentos del proceso %d: \n", proceso->pid);
		list_iterate(proceso->lista_tabla_segmentos, _estr_adm_fila_tabla_segmento_print_y_log);
	}

	void _fila_tabla_segmento_print_y_log(void* _fila_tabla){
		t_fila_tabla_segmento* fila_tabla = ((t_fila_tabla_segmento*) _fila_tabla);
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
		return;
	}

	/* Imprimo estructuras administrativas */
	_estr_adm_proceso_print_y_log();

	/* Imprimo lineas de los segmentos del proceso */
	if(proceso->lista_tabla_segmentos->elements_count <= 0){
		log_info(logger, "[PID: %d] No tiene datos asociados ", proceso->pid);
		printf("[PID: %d] No tiene datos asociados\n\n", proceso->pid);
	}
	pthread_mutex_unlock(&sem_mutex_lista_procesos);

	list_iterate(proceso->lista_tabla_segmentos, _fila_tabla_segmento_print_y_log);
}

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








