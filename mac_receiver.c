//////////////////////////////////////////////////////////////////////////////////
/// \file mac_receiver.c
/// \brief MAC receiver thread
/// \author Pascal Sartoretti (sap at hevs dot ch)
/// \version 1.0 - original
/// \date  2018-02
//////////////////////////////////////////////////////////////////////////////////
#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>
#include "main.h"


//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacReceiver(void *argument)
{
	struct queueMsg_t queueMsg;		// queue message
	uint8_t * msg;
	uint8_t * qPtr;
	size_t	size;
	osStatus_t retCode;
	
	//------------------------------------------------------------------------------
	for (;;)						// loop until doomsday
	{
		//----------------------------------------------------------------------------
		// QUEUE READ										
		//----------------------------------------------------------------------------
		retCode = osMessageQueueGet( 	
			queue_macR_id,
			&queueMsg,
			NULL,
			osWaitForever); 	
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);				
	//----------------------------------------------------------------------------
	// MEMORY ALLOCATION				
	//----------------------------------------------------------------------------
	msg = osMemoryPoolAlloc(memPool,osWaitForever);
	if(queueMsg.type == NEW_TOKEN){
		qPtr = (uint8_t*)&queueMsg + 8;
	}
	
	memcpy(msg,&qPtr[1],size-2);

									
	
	queueMsg.anyPtr = msg;
	queueMsg.type = FROM_PHY;

	}
}

