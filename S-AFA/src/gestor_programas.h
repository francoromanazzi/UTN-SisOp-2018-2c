#ifndef GESTOR_PROGRAMAS_H_
#define GESTOR_PROGRAMAS_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <commons/log.h>
	#include <commons/string.h>
	#include <readline/readline.h>
	#include <readline/history.h>
	#include "S-AFA.h"
	#include "planificador.h"
	#include "DTB.h"

	void gestor_consola_iniciar();
	void gestor_iniciar_op_dummy(t_dtb* dummy);


#endif /* GESTOR_PROGRAMAS_H_ */
