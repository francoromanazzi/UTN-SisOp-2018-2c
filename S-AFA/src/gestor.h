#ifndef GESTOR_H_
#define GESTOR_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <commons/log.h>
	#include <commons/string.h>

	#include "S-AFA.h"
	#include "planificador.h"
	#include "gestor_consola.h"
	#include <shared/DTB.h>

	pthread_t thread_gestor_consola;

	void gestor_iniciar();

#endif /* GESTOR_H_ */
