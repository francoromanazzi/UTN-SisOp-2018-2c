#include "MDJ.h"

int main(void) {
	config_create_fixed("../../configs/MDJ.txt");
	mkdir("../../logs",0777);
	logger = log_create("../../logs/MDJ.log", "MDJ", false, LOG_LEVEL_TRACE);

	mdj_exit();
	return EXIT_SUCCESS;
}

void config_create_fixed(char* path){
	config = config_create(path);
	util_config_fix_comillas(&config, "PUNTO_MONTAJE");
}

void mdj_exit(){
	log_destroy(logger);
	config_destroy(config);
}
