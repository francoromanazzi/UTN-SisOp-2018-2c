#include "MDJ_consola.h"

void mdj_consola_init(){
	pwd = strdup(paths_estructuras[ARCHIVOS]);
	chdir(pwd);

	char* linea, *pwd_prompt;

	rl_attempted_completion_function = (CPPFunction *) mdj_consola_autocompletar;

	printf("Bienvenido! Ingrese \"ayuda\" para ver una lista con todos los comandos disponibles \n");
	while(1) {
		pwd_prompt = string_substring_from(strstr(pwd, "Archivos/"), strlen("Archivos"));
		string_append(&pwd_prompt, "> ");
		linea = readline(pwd_prompt);
		if(linea)
			add_history(linea);
		if(!strncmp(linea, "salir", 5)) {
			free(linea);
			break;
		}
		mdj_consola_procesar_comando(linea);
		free(linea);
		free(pwd_prompt);
	}
	exit(EXIT_SUCCESS);
}

void mdj_consola_procesar_comando(char* linea){
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
			log_debug(logger, "[CONSOLA] Abro bloque %s", ruta_bloque);
			f_bloque = fopen(ruta_bloque, "rb");
			free(ruta_bloque);
			void* data = malloc(config_get_int_value(config_metadata, "TAMANIO_BLOQUES"));
			int bytes_a_leer = min(bytes_restantes, config_get_int_value(config_metadata, "TAMANIO_BLOQUES"));
			int bytes_leidos = fread(data, 1, bytes_a_leer, f_bloque);

			ret = realloc(ret, *ret_size + bytes_leidos);
			memcpy(ret + *ret_size, data, bytes_leidos);
			*ret_size += bytes_leidos;
			free(data);

			bytes_restantes -= bytes_leidos;
			i++;
		}
		split_liberar(bloques_strings);
		config_destroy(config_archivo);
		return ret;
	}

	/* Comando ayuda */
	if(argc == 1 && !strcmp(argv[0], "ayuda")){
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
						chdir(pwd);
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
			    chdir(pwd);
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

			/* DEBUG */
			char* contenido_str = malloc(contenido_size + 1);
			memcpy((void*) contenido_str, contenido, contenido_size);
			contenido_str[contenido_size] = '\0';
			printf("SIZE: %d\n%s.\n\n", contenido_size, contenido_str);
			free(contenido_str);


			unsigned char digest[MD5_DIGEST_LENGTH];
			MD5_CTX context;
			MD5_Init(&context);
			MD5_Update(&context, contenido, contenido_size);
			MD5_Final(digest, &context);
			free(contenido);

			char md5_string[MD5_DIGEST_LENGTH * 2 + 1];
			int i;
			for(i = 0; i < MD5_DIGEST_LENGTH; i++)
			    sprintf(&md5_string[i*2], "%02x", (unsigned int)digest[i]);

			printf("%s\n\n", md5_string);
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


/* Attempt to complete on the contents of TEXT.  START and END show the
   region of TEXT that contains the word to complete.  We can use the
   entire line in case we want to do some simple parsing.  Return the
   array of matches, or NULL if there aren't any. */
char** mdj_consola_autocompletar (text, start, end)
     char *text;
     int start, end;
{
  char **matches;

  matches = (char **)NULL;

  /* If this word is at the start of the line, then it is a command
     to complete.  Otherwise it is the name of a file in the current
     directory. */
  if (start == 0)
	  matches = completion_matches (text, mdj_consola_autocompletar_command_generator);
  //else
  //    matches = completion_matches (text, mdj_consola_autocompletar_filename_generator);

  return (matches);
}

/* Generator function for command completion.  STATE lets us know whether to start
   from scratch; without any state (i.e. STATE == 0), then we start at the top of the list. */
char* mdj_consola_autocompletar_command_generator (text, state)
     char *text;
     int state;
{
	static int list_index, len;
	char *name;
	static char* commands[] = {"ayuda", "ls", "cd", "md5", "cat", "clear", "salir", (char*) NULL};

	/* If this is a new word to complete, initialize now.  This includes
	 saving the length of TEXT for efficiency, and initializing the index
	 variable to 0. */
	if(!state){
	  list_index = 0;
	  len = strlen (text);
	}

	/* Return the next name which partially matches from the command list. */
	while ((name = commands[list_index]) != NULL){
	  list_index++;

	  if (strncmp (name, text, len) == 0)
		return (strdup(name));
	}

	/* If no names matched, then return NULL. */
	return ((char *)NULL);
}

