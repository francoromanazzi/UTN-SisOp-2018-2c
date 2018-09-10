#include "util.h"

void free_memory(void** puntero){
	if((*puntero) != NULL){
		free(*puntero);
		*puntero = NULL;
	}
}

void util_config_fix_comillas(t_config** config, char* key){
	char* string_sacar_comillas(char* str){
		char* ret = string_substring(str, 1, strlen(str) - 2);
		return ret;
	}
	char* value = string_sacar_comillas( config_get_string_value(*config, key) );
	config_set_value(*config, key, value);
	free(value);
}


