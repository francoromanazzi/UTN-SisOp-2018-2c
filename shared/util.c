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

void split_liberar(char** split){
	unsigned int i = 0;
	for(; split[i] != NULL;i++){
		free(split[i]);
	}
	free(split);
}

int split_cant_elem(char** split){
	int i = 0;
	for(; split[i] != NULL; i++);
	return i;
}

char* get_local_ip(){
	char* ret = calloc(1, 16);
    const char* google_dns_server = "8.8.8.8";
    int dns_port = 53;

    struct sockaddr_in serv;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    //Socket could not be created
    if(sock < 0){
    	strcpy(ret, "127.0.0.1");
    	return ret;
    }

    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(google_dns_server);
    serv.sin_port = htons(dns_port);

    int err = connect(sock, (const struct sockaddr*)&serv, sizeof(serv));
    if (err < 0){
    	strcpy(ret, "127.0.0.1");
        return ret;
    }

    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    err = getsockname(sock, (struct sockaddr*)&name, &namelen);

    const char* p = inet_ntop(AF_INET, &name.sin_addr, ret, 80);
    close(sock);
    if(p == NULL){
    	strcpy(ret, "127.0.0.1");
        return ret;
    }
    return ret;
}
