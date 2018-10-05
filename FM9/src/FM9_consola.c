#include "FM9_consola.h"

static void fm9_procesar_comando(char*);

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

static void fm9_procesar_comando(char* linea){

	char** argv = string_split(linea, " ");
	int argc = split_cant_elem(argv);

	if(argc == 1 && !strcmp(argv[0], "dump")){
		//Muestra su storage completo
		int i = 0;
		char* str;
		for(; i< (tamanio/max_linea); i++){
			str = fm9_storage_leer(i);
			printf("%d: %s\n", i, str);
			free(str);
		}
		printf("\n");
	}
	else if(argc == 2 && !strcmp(argv[0], "dump")){
		printf("No se pudo reconocer el comando\n\n");
	}
	else{
		printf("No se pudo reconocer el comando\n\n");
	}
	split_liberar(argv);
}


