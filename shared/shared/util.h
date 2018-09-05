#ifndef SHARED_UTIL_H_
#define SHARED_UTIL_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <commons/config.h>
	#include <commons/string.h>
	#include <string.h>

	/**
	* @NAME: util_config_fix_comillas
	* @DESC: Dentro de los archivos de config, cuando lee una IP, lo convierte en memoria de ""127.0.0.1"\0" a "127.0.0.1\0"
	*/
	void util_config_fix_comillas(t_config** config, char* key);

#endif /* SHARED_UTIL_H_ */
