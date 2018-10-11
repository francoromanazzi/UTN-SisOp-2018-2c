#include "MDJ_consola.h"

void mdj_consola_init(){
	pwd = strdup(paths_estructuras[ARCHIVOS]);
	char* linea, *pwd_prompt;

	printf("Bienvenido! Ingrese \"ayuda\" para ver una lista con todos los comandos disponibles \n");
	while(1) {
		//pwd_prompt = strstr(strstr(pwd, "Archivos/"), "/");
		pwd_prompt = string_substring_from(strstr(pwd, "Archivos/"), strlen("Archivos"));
		string_append(&pwd_prompt, "> ");
		linea = readline(pwd_prompt);
		if(linea)
			add_history(linea);
		if(!strncmp(linea, "salir", 5)) {
			free(linea);
			break;
		}
		mdj_procesar_comando(linea);
		free(linea);
		free(pwd_prompt);
	}
	exit(EXIT_SUCCESS);
}

void mdj_procesar_comando(char* linea){
	char** argv = string_split(linea, " ");
	int argc = split_cant_elem(argv);

	void* _obtener_contenido_archivo(char* path_real, int* ret_size){
		FILE* f_bloque;
		t_config* config_archivo = config_create(path_real);
		int bytes_restantes = config_get_int_value(config_archivo, "TAMANIO");
		char** bloques_strings = config_get_array_value(config_archivo, "BLOQUES");
		void* ret = NULL;
		int i = 0;
		*ret_size = 0;

		while(bytes_restantes > 0){
			char* ruta_bloque = string_new();
			string_append(&ruta_bloque, paths_estructuras[BLOQUES]);
			string_append(&ruta_bloque, bloques_strings[i]);
			string_append(&ruta_bloque, ".bin");
			f_bloque = fopen(ruta_bloque, "rb");
			free(ruta_bloque);
			void* data = malloc(config_get_int_value(config_metadata, "TAMANIO_BLOQUES"));
			int bytes_a_leer = bytes_restantes < config_get_int_value(config_metadata, "TAMANIO_BLOQUES")
					? bytes_restantes : config_get_int_value(config_metadata, "TAMANIO_BLOQUES");
			int bytes_leidos = fread(data, 1, bytes_a_leer, f_bloque);

			ret = realloc(ret, *ret_size + bytes_leidos);
			memcpy(ret + *ret_size, data, bytes_leidos);
			*ret_size += bytes_leidos;
			free(data);

			bytes_restantes -= config_get_int_value(config_metadata, "TAMANIO_BLOQUES");
			i++;
		}
		split_liberar(bloques_strings);
		config_destroy(config_archivo);
		return ret;
	}

	/* Comando ayuda */
	if(!strcmp(linea, "ayuda")){
		printf("\nls:\t\tLista los directorios y archivos del directorio actual\n");
		printf("ls [path]:\tLista los directorios y archivos del directorio [path]\n");
		printf("cd [path]:\tSe cambia el directorio actual por [path]\n");
		printf("md5 [path]:\tGenera el md5 del archivo [path]\n");
		printf("cat [path]:\tMuestra el contenido de [path]\n");
		printf("clear:\t\tLimpia la pantalla\n");
		printf("salir:\t\tCierra la consola\n\n");
	}

	/* Comando ls */
	else if(argc == 1 && !strcmp(argv[0], "ls")){
		DIR *d;
		struct dirent *dir;
		d = opendir(pwd);
		if(d){
			while ((dir = readdir(d)) != NULL) {
				if(strcmp(dir->d_name, ".") && strcmp(dir->d_name, ".."))
					printf("%s\t", dir->d_name);
			}
			closedir(d);
		}
		printf("\n\n");
	}

	/* Comando ls [path] */
	else if(argc == 2 && !strcmp(argv[0], "ls")){
		DIR *d;
		struct dirent *dir;
		char* pwd_copy = strdup(pwd);
		string_append(&pwd_copy, argv[1]);
		if(!string_ends_with(argv[1], "/")){
			string_append(&pwd_copy, "/");
		}
		d = opendir(pwd_copy);
		if(d){
			while ((dir = readdir(d)) != NULL) {
				if(strcmp(dir->d_name, ".") && strcmp(dir->d_name, ".."))
					printf("%s\t", dir->d_name);
			}
			closedir(d);
		}
		else{
			printf("[ERROR] El directorio no existe");
		}
		printf("\n\n");
	}

	/* Comando cd [path] */
	else if(argc == 2 && !strcmp(argv[0], "cd")){
		if(!strcmp(argv[1], ".")){
		}
		else if(!strcmp(argv[1], "..")){
			if(!strcmp(pwd, paths_estructuras[ARCHIVOS])){
				printf("[ERROR] Ya se encuentra en el directorio raiz\n\n");
			}
			else{ // Voy un directorio atras
				int i;
				for(i = strlen(pwd) - 2 ; i >= 0; i--){
					if(pwd[i] == '/'){
						pwd[i + 1] = '\0';
						break;
					}
				}
			}
		}
		else{
			char* pwd_copy = strdup(pwd);
			string_append(&pwd_copy, argv[1]);
			if(!string_ends_with(argv[1], "/")){
				string_append(&pwd_copy, "/");
			}
			DIR* dir = opendir(pwd_copy);
			if(dir){ // Existe el directorio
			    free(pwd);
			    pwd = strdup(pwd_copy);
			    closedir(dir);
			}
			else if (ENOENT == errno) {// No existe el directorio
				printf("[ERROR] El directorio no existe\n\n");
			}
			else {
			    printf("[ERROR] Ocurrio un error inesperado\n\n");
			}
			free(pwd_copy);
		}
	}

	/* Comando md5 [path] */
	else if(argc == 2 && !strcmp(argv[0], "md5")){
		char* pwd_copy = strdup(pwd);
		string_append(&pwd_copy, argv[1]);

		if(access(pwd_copy, F_OK) != -1 ){ // El archivo existe
			int contenido_size;
			void* contenido = _obtener_contenido_archivo(pwd_copy, &contenido_size);
			void* digest = malloc(MD5_DIGEST_LENGTH);
			MD5_CTX context;
			MD5_Init(&context);
			MD5_Update(&context, contenido, contenido_size);
			MD5_Final(digest, &context);
			free(contenido);

			char* digest_str = malloc(MD5_DIGEST_LENGTH + 1);
			memcpy((void*) digest_str, digest, MD5_DIGEST_LENGTH);
			digest_str[MD5_DIGEST_LENGTH] = '\0';
			printf("%s\n\n", digest_str);
			free(digest_str);
			free(digest);
		}
		else{ // El archivo no existe
			printf("[ERROR] El archivo no existe\n\n");
		}
		free(pwd_copy);
	}

	/* Comando cat [path] */
	else if(argc == 2 && !strcmp(argv[0], "cat")){
		char* pwd_copy = strdup(pwd);
		string_append(&pwd_copy, argv[1]);

		if(access(pwd_copy, F_OK) != -1 ){ // El archivo existe
			int contenido_size;
			void* contenido = _obtener_contenido_archivo(pwd_copy, &contenido_size);

			char* contenido_str = malloc(contenido_size + 1);
			memcpy((void*) contenido_str, contenido, contenido_size);
			contenido_str[contenido_size] = '\0';
			printf("%s\n", contenido_str);

			free(contenido_str);
			free(contenido);
		}
		else{ // El archivo no existe
			printf("[ERROR] El archivo no existe\n\n");
		}
		free(pwd_copy);
	}

	/* Comando clear */
	else if(argc == 1 && !strcmp(argv[0], "clear")){
		system("clear");
	}

	/* Error al ingresar comando */
	else{
		printf("[ERROR] No se pudo reconocer el comando\n\n");
	}
	split_liberar(argv);
}
