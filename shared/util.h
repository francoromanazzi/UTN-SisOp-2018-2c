#ifndef SHARED_UTIL_H_
#define SHARED_UTIL_H_
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <unistd.h>

	#include <commons/config.h>
	#include <commons/string.h>

	#define max(a,b) \
		({ __typeof__ (a) _a = (a); \
		__typeof__ (b) _b = (b); \
		_a > _b ? _a : _b; })

	#define min(a,b) \
		({ __typeof__ (a) _a = (a); \
		__typeof__ (b) _b = (b); \
		_a < _b ? _a : _b; })

	#define ceiling(x,y) (((x) + (y) - 1) / (y))


	typedef struct {
		int x;
		int y;
	} t_vector2;


	/**
	* @NAME: util_config_fix_comillas
	* @DESC: Dentro de los archivos de config, cuando lee una IP, lo convierte en memoria de ""127.0.0.1"\0" a "127.0.0.1\0"
	*/
	void util_config_fix_comillas(t_config** config, char* key);

	/**
	* @NAME: free_memory
	* @DESC: Libera la memoria de un puntero, si no es NULL
	*/
	void free_memory(void** puntero);

	void split_liberar(char** split);

	int split_cant_elem(char**split);

	char* get_local_ip();

#endif /* SHARED_UTIL_H_ */
