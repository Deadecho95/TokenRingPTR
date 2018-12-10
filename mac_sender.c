//////////////////////////////////////////////////////////////////////////////////
/// \file mac_sender.c
/// \brief MAC sender thread
/// \author Pascal Sartoretti (pascal dot sartoretti at hevs dot ch)
/// \version 1.0 - original
/// \date  2018-02
//////////////////////////////////////////////////////////////////////////////////
#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "ext_led.h"


//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacSender(void *argument)
{
	osMessageQueueId_t	buffer = osMessageQueueNew(2,sizeof(struct queueMsg_t),NULL); 
	struct queueMsg_t queueMsg;		// queue message
	uint8_t * qPtr;
	uint8_t * tokenPtr;
	osStatus_t retCode;
	uint8_t * msg;

	//------------------------------------------------------------------------------
	for (;;)						// loop until doomsday
	{	
		//----------------------------------------------------------------------------
		// QUEUE READ										
		//----------------------------------------------------------------------------
		retCode = osMessageQueueGet( 	
			queue_macS_id,
			&queueMsg,
			NULL,
			osWaitForever); 	
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);				
		
		switch(queueMsg.type){
			case NEW_TOKEN :
					//----------------------------------------------------------------------------
					// Mem alloc				
					//----------------------------------------------------------------------------
						msg = osMemoryPoolAlloc(memPool,osWaitForever);											
						msg[0] = TOKEN_TAG;
						queueMsg.anyPtr = msg;
						queueMsg.type = TO_PHY;
					//--------------------------------------------------------------------------
					// QUEUE SEND	(send frame to the next station)
					//--------------------------------------------------------------------------
					retCode = osMessageQueuePut(
						queue_phyS_id,
						&queueMsg,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);						
				break;
			
			case START :
				break;

			case STOP :
				break;

			case DATA_IND :
		//--------------------------------------------------------------------------
		// QUEUE SEND	(send frame to the buffer)
		//--------------------------------------------------------------------------
		retCode = osMessageQueuePut(
			buffer,
			&queueMsg,
			osPriorityNormal,
			osWaitForever);
		CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
				break;

			case TOKEN :
				//update token info
				tokenPtr = queueMsg.anyPtr;
				if(gTokenInterface.connected == 1){
				qPtr[gTokenInterface.myAddress+1] = 10;
				}
				else{
				qPtr[gTokenInterface.myAddress+1] = 8;
				}
				//----------------------------------------------------------------------------
				// QUEUE READ										
				//----------------------------------------------------------------------------
				retCode = osMessageQueueGet( 	
					queue_macS_id,
					&queueMsg,
					NULL,
					NULL); 	
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
				qPtr = queueMsg.anyPtr;
				
				
				break;			
			
			default:
				break;
		}
		
		
		
		
		
	}
}
