#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/config.h>
#include <commons/string.h>

int main(int argc, char* argv[]) {

	if(argc != 5){
		printf("SAFA DAM FM9 MDJ\n\n");
		return EXIT_FAILURE;
	}

	char* safa_ip = strdup("\""); string_append(&safa_ip, argv[1]); string_append(&safa_ip, "\"");
	char* dam_ip = strdup("\""); string_append(&dam_ip, argv[2]); string_append(&dam_ip, "\"");
	char* fm9_ip = strdup("\""); string_append(&fm9_ip, argv[3]); string_append(&fm9_ip, "\"");
	char* mdj_ip = strdup("\""); string_append(&mdj_ip, argv[4]); string_append(&mdj_ip, "\"");

	char* dir_names[] = {
			"../configs/",
			"../configs_prueba_algoritmo_propio/",
			"../configs_prueba_algoritmos/",
			"../configs_prueba_completa/",
			"../configs_prueba_filesystem/",
			"../configs_prueba_minima/",
			(char*) NULL };


	int i;
	char* dir;
	for(i = 0; (dir = dir_names[i]) != NULL; i++){

		/* ~~~~~ CPU ~~~~~ */
		char* config_cpu_path = strdup(dir);
		string_append(&config_cpu_path, "CPU.txt");
		t_config* config = config_create(config_cpu_path);
		config_set_value(config, "IP_SAFA", safa_ip);
		config_set_value(config, "IP_DIEGO", dam_ip);
		config_set_value(config, "IP_MEM", fm9_ip);
		config_save(config);
		config_destroy(config);
		free(config_cpu_path);


		/* ~~~~~ DAM ~~~~~ */
		char* config_dam_path = strdup(dir);
		string_append(&config_dam_path, "DAM.txt");
		config = config_create(config_dam_path);
		config_set_value(config, "IP_SAFA", safa_ip);
		config_set_value(config, "IP_MDJ", mdj_ip);
		config_set_value(config, "IP_FM9", fm9_ip);
		config_save(config);
		config_destroy(config);
		free(config_dam_path);
	}


	return EXIT_SUCCESS;
}
