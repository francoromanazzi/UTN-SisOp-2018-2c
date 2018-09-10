#ifndef SHARED_PROTOCOL_H_
#define SHARED_PROTOCOL_H_
	#include "msg.h"
	#include "/home/utnso/workspace/tp-2018-2c-RegorDTs/S-AFA/src/DTB.h"

	t_msg* empaquetar_dtb(t_dtb* dtb);
	t_dtb* desempaquetar_dtb(t_msg* msg);

#endif /* SHARED_PROTOCOL_H_ */
