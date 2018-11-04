#ifndef MDJ_CONSOLA_H_
#define MDJ_CONSOLA_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <dirent.h>
	#include <openssl/md5.h>
	#include <unistd.h>
	#include <commons/log.h>
	#include <commons/string.h>
	#include <readline/readline.h>
	#include <readline/history.h>

	#include <shared/util.h>
	#include "MDJ.h"


	char* pwd;

	void mdj_consola_init();
	void mdj_consola_procesar_comando(char* linea);

	char** mdj_consola_autocompletar();
	char* mdj_consola_autocompletar_command_generator();
	extern char** completion_matches(char *, rl_compentry_func_t *);

#endif /* MDJ_CONSOLA_H_ */
