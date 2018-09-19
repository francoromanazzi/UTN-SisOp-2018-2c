#ifndef OPERACION_H_
#define OPERACION_H_
	#include <stdlib.h>

	#include <commons/collections/dictionary.h>

	typedef enum {OP_ABRIR, OP_CONCENTRAR, OP_ASIGNAR, OP_WAIT, OP_SIGNAL, OP_FLUSH, OP_CLOSE, OP_CREAR, OP_BORRAR} e_tipo_operacion;

	typedef struct{
		e_tipo_operacion tipo_operacion;
		t_dictionary* operandos;
	}t_operacion;

	void operacion_free(t_operacion** operacion);

#endif /* OPERACION_H_ */
