/***************************************************************************//**
 * @file
 * @brief Silabs Network Co-Processor (NCP) library
 * This library allows customers create applications work in NCP mode.
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#ifndef USER_COMMAND_H_
#define USER_COMMAND_H_

#define THRESHOLD_COEX_REQ_NO_PTA_REQUEST_ON_SCAN 	139u
enum {
	OPCODE_VER_GET			=0x00,
	OPCODE_COEX_SET			=0x01,
	OPCODE_COEX_GET			=0x02,
	OPCODE_PWM_SET			=0x03,
	OPCODE_PWM_GET			=0x04,
	OPCODE_COEX_LL_THRESH_SET =0x05,
	OPCODE_COEX_LL_THRESH_GET =0x06,
	OPCODE_LL_PRIO_GET		=0x07,
};

#define COEX_PTA_REQ_ON_SCAN	0u
#define COEX_PTA_NO_REQ_ON_SCAN	1u
#define USER_COMMAND_VER_HI		1u
#define USER_COMMAND_VER_LO		0u
#define USER_COMMAND_VER_MAGIC	0xA5

#define MAX_RESPONSE_LEN		3u


#endif /* USER_COMMAND_H_ */
