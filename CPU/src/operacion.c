#include "operacion.h"

void operacion_free(t_operacion** operacion){
	dictionary_destroy((*operacion)->operandos);
	free(*operacion);
}

