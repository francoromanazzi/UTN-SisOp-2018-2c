#include "FM9.h"

int main(void) {
	config_create("../../configs/FM9.txt");
	logger = log_create("../../logs/FM9.log", "FM9", false, LOG_LEVEL_TRACE);

	fm9_exit();
	return EXIT_SUCCESS;
}

void fm9_exit(){
	log_destroy(logger);
	config_destroy(config);
}
