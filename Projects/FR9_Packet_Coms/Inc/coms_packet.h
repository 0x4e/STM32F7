/*
 * coms_packet.h
 *
 *  Created on: 23 Oct 2017
 *      Author: nick
 */

#ifndef INC_COMS_PACKET_H_
#define INC_COMS_PACKET_H_

typedef enum {
    COMM_FW_VERSION = 0,
    COMM_READ_REG32,
	COMM_READ_REG16
} COMM_PACKET_ID;


void coms_packet_create_task(void);

#endif /* INC_COMS_PACKET_H_ */
