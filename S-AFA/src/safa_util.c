#include "safa_util.h"

//static void safa_config_inotify_handle_events(int fd, int wd);

void safa_util_initialize(){
	pthread_mutex_init(&sem_mutex_config, NULL);

	safa_config_create_fixed();

	mkdir(LOG_DIRECTORY_PATH, 0777);
	logger = log_create(LOG_PATH, "S-AFA", false, LOG_LEVEL_TRACE);
}

void safa_config_create_fixed(){
	config = config_create(CONFIG_PATH);

/*	static int i = 0;
	char* i_str = string_itoa(i++);
	char* path_log = strdup("CONFIG_");
	string_append(&path_log, i_str);
	string_append(&path_log, ".txt");
	config_save_in_file(config, path_log);
	free(path_log);
	free(i_str);*/

	util_config_fix_comillas(&config, "ALGORITMO");
	char* retardo_microsegundos_str = string_itoa(1000 * config_get_int_value(config, "RETARDO_PLANIF"));
	config_set_value(config, "RETARDO_PLANIF", retardo_microsegundos_str); // Milisegundos a microsegundos
	free(retardo_microsegundos_str);
}

int safa_config_get_int_value(char* key){
	pthread_mutex_lock(&sem_mutex_config);
	int ret = config_get_int_value(config, key);
	pthread_mutex_unlock(&sem_mutex_config);
	return ret;
}

char* safa_config_get_string_value(char* key){
	pthread_mutex_lock(&sem_mutex_config);
	char* ret = config_get_string_value(config, key);
	pthread_mutex_unlock(&sem_mutex_config);
	return ret;
}

t_status* status_copiar(t_status* otro_status){
	t_status* ret = malloc(sizeof(t_status));
	ret->cant_procesos_activos = otro_status->cant_procesos_activos;

	ret->new = list_duplicate(otro_status->new);
	list_iterate(ret->new, dtb_copiar_sobreescribir);

	ret->ready = list_duplicate(otro_status->ready);
	list_iterate(ret->ready, dtb_copiar_sobreescribir);

	ret->exec = list_duplicate(otro_status->exec);
	list_iterate(ret->exec, dtb_copiar_sobreescribir);

	ret->block = list_duplicate(otro_status->block);
	list_iterate(ret->block, dtb_copiar_sobreescribir);

	ret->exit = list_duplicate(otro_status->exit);
	list_iterate(ret->exit, dtb_copiar_sobreescribir);

	return ret;
}

void status_free(t_status* status){
	list_destroy_and_destroy_elements(status->new, dtb_destroy_v2);
	list_destroy_and_destroy_elements(status->block, dtb_destroy_v2);
	list_destroy_and_destroy_elements(status->ready, dtb_destroy_v2);
	list_destroy_and_destroy_elements(status->exec, dtb_destroy_v2);
	list_destroy_and_destroy_elements(status->exit, dtb_destroy_v2);
	free(status);
}

static void safa_config_inotify_handle_events(int fd, int wd){
	char buf[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
	const struct inotify_event *event;
	int i;
	ssize_t len;
	char *ptr;

	/* Loop while events can be read from inotify file descriptor. */
	for (;;) {
		/* Read some events. */
		len = read(fd, buf, sizeof buf);
		if (len == -1 && errno != EAGAIN) {
			log_error(logger, "[INOTIFY] read");
		    exit(EXIT_FAILURE);
		}

	   if (len <= 0)
		   break;

	   /* Loop over all events in the buffer */
	   for (ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) {
		   event = (const struct inotify_event *) ptr;

            if (event->mask & IN_CLOSE_WRITE){
				log_info(logger, "[INOTIFY] El archivo config ha sido modificado");

				/* Actualizo config */
				pthread_mutex_lock(&sem_mutex_config);
				config_destroy(config);
				safa_config_create_fixed();
				log_info(logger, "[INOTIFY] Nuevo algoritmo: %s", config_get_string_value(config, "ALGORITMO"));
				log_info(logger, "[INOTIFY] Nuevo quantum: %d", config_get_int_value(config, "QUANTUM"));
				log_info(logger, "[INOTIFY] Nueva multiprogramacion: %d", config_get_int_value(config, "MULTIPROGRAMACION"));
				log_info(logger, "[INOTIFY] Nuevo retardo: %d", config_get_int_value(config, "RETARDO_PLANIF"));
				pthread_mutex_unlock(&sem_mutex_config);
            }
	    }
	}
}

void safa_config_inotify_iniciar(){
	int fd, poll_num, wd;
	nfds_t nfds;
	struct pollfd pollfd;

	/* Create the file descriptor for accessing the inotify API */
	fd = inotify_init1(IN_NONBLOCK);
	if(fd == -1){
		log_error(logger, "[INOTIFY] inotify_init1");
		exit(EXIT_FAILURE);
	}

	wd = inotify_add_watch(fd, CONFIG_PATH, IN_CLOSE_WRITE);
	if (wd == -1) {
		log_error(logger, "[INOTIFY] inotify_add_watch");
		exit(EXIT_FAILURE);
	}

	/* Prepare for polling */
	nfds = 1;

	/* Inotify input */
	pollfd.fd = fd;
	pollfd.events = POLLIN;

	/* Wait for events */
	while(1){
		poll_num = poll(&pollfd, nfds, -1);
		if (poll_num == -1) {
			if (errno == EINTR)
				continue;
			log_error(logger, "[INOTIFY] poll");
			exit(EXIT_FAILURE);
		}

		if (poll_num > 0) {
			if (pollfd.revents & POLLIN) {
				safa_config_inotify_handle_events(fd, wd);
			}
		}
	}

	close(fd);
	exit(EXIT_SUCCESS);
}

/*void safa_config_inotify_iniciar(){
	char target[FILENAME_MAX];
	   int result;
	   int fd;
	   int wd;
	   const int event_size = sizeof(struct inotify_event);
	   const int buf_len = 1024 * (event_size + FILENAME_MAX);

	   strcpy (target, CONFIG_PATH);

	   fd = inotify_init();

	   wd = inotify_add_watch (fd, CONFIG_PATH, IN_CLOSE_WRITE);

	   while (1) {
	     char buff[buf_len];
	     int no_of_events, count = 0;

	     no_of_events = read (fd, buff, buf_len);

	     while (count < no_of_events) {
	       struct inotify_event *event = (struct inotify_event *)&buff[count];

	       if (event->len){
	         if (event->mask & IN_CLOSE_WRITE){
	        	 if(!(event->mask & IN_ISDIR)){
	        	 	              log_info(logger,"%s opened for writing was closed", target);
	        	 	              fflush(stdout);
	        	 	           }
	        	 	           else{
	        	 	              log_info(logger,"{ELSE} %s opened for writing was closed", target);
	        	 	              fflush(stdout);
	        	 	           }
	         }

	       }
	       count += event_size + event->len;
	     }
	   }

	   return;
}*/










































