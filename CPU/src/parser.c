#include "parser.h"

t_operacion* parse(char* linea){

	void split_liberar(char** split){
		unsigned int i = 0;
		for(; split[i] != NULL; i++){
			free(split[i]);
		}
		free(split);
	}

	int split_cant_elem(char** split){
		int i = 0;
		for(; split[i] != NULL; i++);
		return i;
	}

	t_operacion* ret = malloc(sizeof(t_operacion));
	ret->operandos = dictionary_create();

	char** argv = string_split(linea, " ");
	int argc = split_cant_elem(argv);

	/* ABRIR path */
	if(!strcmp(argv[0], "abrir")){
		ret->tipo_operacion = OP_ABRIR;
		dictionary_put(ret->operandos, "path", (void*) strdup(argv[1]));
	}

	/* CONCENTRAR */
	else if(!strcmp(argv[0], "concentrar")){
		ret->tipo_operacion = OP_CONCENTRAR;
	}

	/* ASIGNAR path linea datos */
	else if(!strcmp(argv[0], "asignar")){
		ret->tipo_operacion = OP_ASIGNAR;
		dictionary_put(ret->operandos, "path", (void*) strdup(argv[1]));
		dictionary_put(ret->operandos, "linea", (void*) strdup(argv[2]));
		dictionary_put(ret->operandos, "datos", (void*) strdup(argv[3]));
	}

	/* WAIT recurso */
	else if(!strcmp(argv[0], "wait")){
		ret->tipo_operacion = OP_WAIT;
		dictionary_put(ret->operandos, "recurso", (void*) strdup(argv[1]));
	}

	/* SIGNAL recurso */
	else if(!strcmp(argv[0], "signal")){
		ret->tipo_operacion = OP_SIGNAL;
		dictionary_put(ret->operandos, "recurso", (void*) strdup(argv[1]));
	}

	/* FLUSH path */
	else if(!strcmp(argv[0], "flush")){
		ret->tipo_operacion = OP_FLUSH;
		dictionary_put(ret->operandos, "path", (void*) strdup(argv[1]));
	}

	/* CLOSE path */
	else if(!strcmp(argv[0], "close")){
		ret->tipo_operacion = OP_CLOSE;
		dictionary_put(ret->operandos, "path", (void*) strdup(argv[1]));
	}

	/* CREAR path cant_lineas */
	else if(!strcmp(argv[0], "crear")){
		ret->tipo_operacion = OP_CREAR;
		dictionary_put(ret->operandos, "path", (void*) strdup(argv[1]));
		dictionary_put(ret->operandos, "cant_lineas", (void*) strdup(argv[2]));
	}

	/* BORRAR path */
	else if(!strcmp(argv[0], "borrar")){
		ret->tipo_operacion = OP_BORRAR;
		dictionary_put(ret->operandos, "path", (void*) strdup(argv[1]));
	}

	split_liberar(argv);
	return ret;
}
