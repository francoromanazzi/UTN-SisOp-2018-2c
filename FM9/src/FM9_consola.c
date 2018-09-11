#include "FM9_consola.h"

static void gestor_procesar_comando(char*);

void fm9_consola_init(){
	char* linea;
	printf("Bienvenido a la consola de FM9, las funciones que puede realizar son:\n");
	printf("1. dump [pcb_id]\n");
	printf("2. salir\n");
	while(1) {
			linea = readline("FM9> ");
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
		if(argc == 2 && !strcmp(argv[0], "dump")){
				printf("En construccion\n\n");
				split_liberar(argv);
			}else{
				printf("No se pudo reconocer el comando\n\n");
						split_liberar(argv);
			}
}


