#include "gestor_programas.h"

static void gestor_procesar_comando(char*);

void gestor_consola_iniciar(){
	char * linea;
	printf("Bienvenido! Ingrese \"ayuda\" para ver una lista con todos los comandos disponibles \n");
	while(1) {
		linea = readline("S-AFA> ");
		if(linea)
			add_history(linea);
		if(!strncmp(linea, "salir", 5)) {
			free(linea);
			break;
		}
		gestor_procesar_comando(linea);
		free(linea);
	}
	exit(EXIT_SUCCESS);
}

static void gestor_procesar_comando(char* linea){
	void split_liberar(char** split){
		unsigned int i = 0;
		for(;split[i] != NULL;i++){
			free(split[i]);
		}
		free(split);
	}
	int split_cant_elem(char**split){
		int i = 0;
		for(;split[i] != NULL; i++);
		return i;
	}


	char** argv = string_split(linea, " ");
	int argc = split_cant_elem(argv);

	/* Comando ayuda */
	if(!strcmp(linea, "ayuda")){
		printf("\nejecutar [ruta]:\tEjecuta el escriptorio ubicado en la ruta\n");
		printf("status:\t\t\tMuestra el estado de cada cola\n");
		printf("status [pcb_id]:\tMuestra todos los datos del DTB especificado\n");
		printf("finalizar [pcb_id]:\tFinaliza la ejecucion de un GDT especificado\n");
		printf("metricas:\t\tMuestra distintas metricas del sistema\n");
		printf("metricas [pcb_id]:\tMuestra distintas metricas del sistema y del DTB especificado\n");
		printf("salir:\t\t\tCierra la consola\n\n");
		split_liberar(argv);
	}
	/* Comando ejecutar [ruta]*/
	else if(argc == 2 && !strcmp(argv[0],"ejecutar")){
		planificador_crear_dtb_y_encolar(argv[1]);
		split_liberar(argv);
	}
	/* Comando status*/
	else if(argc == 1 && !strcmp(argv[0], "status")){

		split_liberar(argv);
	}
	/* Comando status [pcb_id] */
	else if(argc == 2 && !strcmp(argv[0], "status")){
		char* estado_actual;
		t_dtb* dtb = planificador_encontrar_dtb( (unsigned) atoi(argv[1]) , &estado_actual);
		if(dtb != NULL)
			dtb_mostrar(dtb, estado_actual);
		else
			printf("No se encontro el DTB con ID = %s\n", argv[1]);
		dtb_destroy(dtb);
		free(estado_actual);
		split_liberar(argv);
	}
	/* Comando finalizar [pcb_id] */
	else if(argc == 2 && !strcmp(argv[0], "finalizar")){

		split_liberar(argv);
	}
	/* Comando metricas */
	else if(argc == 1 && !strcmp(argv[0], "metricas")){

		split_liberar(argv);
	}
	/* Comando metricas [pcb_id] */
	else if(argc == 2 && !strcmp(argv[0], "metricas")){

		split_liberar(argv);
	}
	/* Error al ingresar comando */
	else{
		printf("No se pudo reconocer el comando\n");
		split_liberar(argv);
	}
}
