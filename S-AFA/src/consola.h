#ifndef CONSOLA_H_
#define CONSOLA_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <readline/readline.h>
	#include <readline/history.h>
	#include <pthread.h>

	#include <commons/collections/list.h>
	#include <commons/config.h>
	#include <commons/string.h>

	#include <shared/DTB.h>
	#include <shared/msg.h>
	#include <shared/protocol.h>
	#include <shared/util.h>

	#include "safa_protocol.h"
	#include "safa_util.h"
	#include "metricas.h"

	void consola_iniciar();
	void consola_procesar_comando(char* linea);
	t_safa_msg* consola_esperar_msg(e_safa_tipo_msg tipo_msg);
	void consola_status_free(t_status* status);

#endif /* CONSOLA_H_ */
