#include "consola.h"

void consola_iniciar(){
	char * linea;

	/* Cosas para el autocompletar: */
	rl_attempted_completion_function = (CPPFunction *) consola_autocompletar;
	t_config* config_mdj = config_create("/home/utnso/workspace/tp-2018-2c-RegorDTs/configs/MDJ.txt");
	util_config_fix_comillas(&config_mdj, "PUNTO_MONTAJE");
	ruta_fs_archivos = strdup(config_get_string_value(config_mdj, "PUNTO_MONTAJE"));
	string_append(&ruta_fs_archivos, "Archivos");
	config_destroy(config_mdj);
	chdir(ruta_fs_archivos);
	free(ruta_fs_archivos);


	printf("Bienvenido! Ingrese \"ayuda\" para ver una lista con todos los comandos disponibles \n");
	while(1) {
		linea = readline("S-AFA> ");
		if(linea)
			add_history(linea);
		if(!strncmp(linea, "salir", 5)) {
			free(linea);
			break;
		}
		consola_procesar_comando(linea);
		free(linea);
	}
	exit(EXIT_SUCCESS);
}

void consola_procesar_comando(char* linea){
	t_safa_msg* msg_recibido;
	unsigned int id;
	int ret;

	char** argv = string_split(linea, " ");
	int argc = split_cant_elem(argv);

	bool _mismo_id_proceso_a_finalizar(void* _id){
		return (unsigned int) _id == (unsigned) atoi(argv[1]);
	}


	/* Comando ayuda */
	if(argc == 1 && !strcmp(argv[0], "ayuda")){
		printf("\nejecutar [ruta]+:\tEjecuta el o los escriptorios ubicados en las rutas\n");
		printf("status:\t\t\tMuestra el estado de cada cola\n");
		printf("status [pcb_id]:\tMuestra todos los datos del DTB especificado\n");
		printf("finalizar [pcb_id]:\tFinaliza la ejecucion de un GDT especificado\n");
		printf("metricas:\t\tMuestra distintas metricas del sistema\n");
		printf("metricas [pcb_id]:\tMuestra distintas metricas del sistema y del DTB especificado\n");
		printf("clear:\t\tLimpia la pantalla\n");
		printf("salir:\t\t\tCierra la consola\n\n");
	}

	/* Comando ejecutar [ruta]+ */
	else if(argc >= 2 && !strcmp(argv[0],"ejecutar")){
		int i;
		for(i = 1; i < argc; i++){
			sem_wait(&sem_bin_crear_dtb_1);
			metricas_tiempo_add_start((unsigned) dtb_get_gdt_id_count(false));
			sem_post(&sem_bin_crear_dtb_0);
			safa_protocol_encolar_msg_y_avisar(CONSOLA, PLP, CREAR_DTB, strdup(argv[i]));
		}
	}

	/* Comando status */
	else if(argc == 1 && !strcmp(argv[0], "status")){

		safa_protocol_encolar_msg_y_avisar(CONSOLA, PLP, STATUS);
		msg_recibido = consola_esperar_msg(STATUS);
		t_status* status = (t_status*) (msg_recibido->data);

		printf("CANT_PROCESOS: %d, MULTIPROGRAMACION: %d", status->cant_procesos_activos, safa_config_get_int_value("MULTIPROGRAMACION"));

		printf("\nNEW:%d READY:%d BLOCK:%d EXEC: %d EXIT:%d \n",
				status->new->elements_count,
				status->ready->elements_count,
				status->block->elements_count,
				status->exec->elements_count,
				status->exit->elements_count);
		int i;

		printf("\n-----------------Cola NEW-----------------:\n");
		for(i = 0; i < status->new->elements_count; i++){
			dtb_mostrar(list_get(status->new, i));
		}

		printf("\n-----------------Cola READY-----------------:\n");
		for(i = 0; i < status->ready->elements_count; i++){
			dtb_mostrar(list_get(status->ready, i));
		}

		printf("\n-----------------Cola BLOCK-----------------:\n");
		for(i = 0; i < status->block->elements_count; i++){
			dtb_mostrar(list_get(status->block, i));
		}

		printf("\n-----------------Cola EXEC-----------------:\n");
		for(i = 0; i < status->exec->elements_count; i++){
			dtb_mostrar(list_get(status->exec, i));
		}

		printf("\n-----------------Cola EXIT-----------------:\n");
		for(i = 0; i < status->exit->elements_count; i++){
			dtb_mostrar(list_get(status->exit, i));
		}
		printf("\n");
		status_free(status);
		msg_recibido->data = NULL;
		safa_protocol_msg_free(msg_recibido);
	}

	/* Comando status [pcb_id] */
	else if(argc == 2 && !strcmp(argv[0], "status")){
		safa_protocol_encolar_msg_y_avisar(CONSOLA, PLP, STATUS_DTB, atoi(argv[1]) );
		msg_recibido = consola_esperar_msg(STATUS_DTB);
		t_dtb* dtb = (t_dtb*) (msg_recibido->data);

		if(dtb == NULL){
			printf("No se encontro el proceso con ID: %s\n\n", argv[1]);
			safa_protocol_msg_free(msg_recibido);
		}
		else{
			dtb_mostrar(dtb);
			dtb_destroy(dtb);
			msg_recibido->data = NULL;
			safa_protocol_msg_free(msg_recibido);
		}
	}

	/* Comando finalizar [pcb_id] */
	else if(argc == 2 && !strcmp(argv[0], "finalizar")){
		if(!strcmp(argv[1], "0"))
			printf("No se puede finalizar el proceso dummy\n\n");
		else if(atoi(argv[1]) == 0)
			printf("Se debe ingresar el numero de ID\n\n");
		else {
			safa_protocol_encolar_msg_y_avisar(CONSOLA, PLP, EXIT_DTB_CONSOLA, (unsigned) atoi(argv[1]));
			msg_recibido = consola_esperar_msg(EXIT_DTB_CONSOLA);
			memcpy((void*) &id, msg_recibido->data, sizeof(unsigned int));
			memcpy((void*) &ret, msg_recibido->data + sizeof(unsigned int), sizeof(int));

			switch(ret){
				case 0:
					printf("No se ha encontrado el proceso con ID: %d\n\n", id);
				break;

				case 1:
					printf("El proceso con ID: %d ha sido finalizado satisfactoriamente\n\n", id);
				break;

				case 2:
					printf("El proceso con ID: %d sera finalizado cuando vuelva de EXEC\n\n", id);
				break;

				case 3:
					printf("El proceso con ID: %d ya se encontraba finalizado\n\n", id);
				break;
			}
			safa_protocol_msg_free(msg_recibido);
		}
	}

	/* Comando metricas */
	else if(argc == 1 && !strcmp(argv[0], "metricas")){
		consola_print_metricas();
	}

	/* Comando metricas [pcb_id] */
	else if(argc == 2 && !strcmp(argv[0], "metricas")){
		if(!strcmp(argv[1], "0"))
			printf("No se puede seleccionar al proceso dummy\n\n");
		else if(atoi(argv[1]) == 0)
			printf("Se debe ingresar el numero de ID\n\n");
		else {
			safa_protocol_encolar_msg_y_avisar(CONSOLA, PLP, METRICAS_DTB, (unsigned) atoi(argv[1]));
			msg_recibido = consola_esperar_msg(METRICAS_DTB);
			ret = (int) msg_recibido->data;
			msg_recibido->data = NULL;

			if(ret == -1){
				printf("No se ha encontrado el proceso con ID: %d\n", (unsigned) atoi(argv[1]));
			}
			else {
				printf("Cant de sentencias ejecutadas que espero en NEW: %d\n", ret);
			}
			safa_protocol_msg_free(msg_recibido);

			printf("~~~~~~~~~~\n");
			consola_print_metricas();
		}
	}

	/* Comando clear */
	else if(argc == 1 && !strcmp(argv[0], "clear")){
		system("clear");
	}

	/* Error al ingresar comando */
	else{
		printf("No se pudo reconocer el comando\n\n");

	}
	split_liberar(argv);
}

void consola_print_metricas(){
	printf("Porcentaje de sentencias hacia diego: %.2f\%%\n", metricas_porcentaje_sentencias_hacia_diego());
	printf("Tiempo de respuesta promedio: %.2fs\n\n", metricas_tiempo_get_promedio());
}

t_safa_msg* consola_esperar_msg(e_safa_tipo_msg tipo_msg){
	t_list* lista_msg_distinto_tipo = list_create();

	while(1){
		sem_wait(&sem_cont_cola_msg_consola);
		t_safa_msg* msg = safa_protocol_desencolar_msg(CONSOLA);
		if(msg->tipo_msg != tipo_msg){
			list_add(lista_msg_distinto_tipo, (void*) msg);
		}
		else{ // Encontre el mensaje buscado
			int i = 0;
			/* Vuelvo a encolar los mensajes de distinto tipo */
			pthread_mutex_lock(&sem_mutex_cola_msg_consola);
			for(; i<lista_msg_distinto_tipo->elements_count; i++){
				list_add(cola_msg_consola, list_remove(lista_msg_distinto_tipo, i));
				sem_post(&sem_cont_cola_msg_consola);
			}
			pthread_mutex_unlock(&sem_mutex_cola_msg_consola);
			list_destroy(lista_msg_distinto_tipo);
			return msg;
		}
	}
}


/* Attempt to complete on the contents of TEXT.  START and END show the
   region of TEXT that contains the word to complete.  We can use the
   entire line in case we want to do some simple parsing.  Return the
   array of matches, or NULL if there aren't any. */
char** consola_autocompletar (text, start, end)
     char *text;
     int start, end;
{
  char **matches;

  matches = (char **)NULL;

  /* If this word is at the start of the line, then it is a command
     to complete.  Otherwise it is the name of a file in the current
     directory. */
  if (start == 0)
    matches = completion_matches (text, consola_autocompletar_command_generator);

  return (matches);
}

/* Generator function for command completion.  STATE lets us know whether to start
   from scratch; without any state (i.e. STATE == 0), then we start at the top of the list. */
char* consola_autocompletar_command_generator (text, state)
     char *text;
     int state;
{
	static int list_index, len;
	char *name;
	static char* commands[] = {"ayuda", "ejecutar", "status", "finalizar", "metricas", "clear", "salir", (char*) NULL};

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






















