#include "FM9_consola.h"


void fm9_consola_init(){
	char* linea;
	printf("Bienvenido a la consola de FM9, las funciones que puede realizar son:\n");
	printf("1. dump \n");
	printf("2. dump [pcb_id]\n");
	printf("3. salir\n\n");
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

	/* Comando DUMP */
	if(argc == 1 && !strcmp(argv[0], "dump")){ // Muestra su storage completo, sin estructuras administrativas
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










