#include "operacion.h"

void operacion_free(t_operacion** operacion){
	dictionary_destroy_and_destroy_elements((*operacion)->operandos, free);
	free(*operacion);
}
