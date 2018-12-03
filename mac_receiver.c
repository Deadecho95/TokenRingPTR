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
	uint8_t crc = 0;
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
	// Test Frame				
	//----------------------------------------------------------------------------
		qPtr = queueMsg.anyPtr;
		if(qPtr[0] == TOKEN_TAG){							// token frame 
			retCode = osMessageQueuePut(
				queue_macS_id,
				&queueMsg,
				NULL,
				osWaitForever);
		}
		else{ 			// data frame
			//crc//
			for(int i = 0; i < qPtr[2]; i++){
				crc = crc + *qPtr+2+i;
			}
			crc = crc%6;
			//crcr not ok
			if(crc != ((qPtr[qPtr[2]+3])&0b11111100)>>2){
				qPtr[qPtr[2]+3] = qPtr[qPtr[2]+3] | 0b00000010;	//READ = 1
				retCode = osMessageQueuePut(
					queue_phyS_id,
					&queueMsg,
					NULL,
					osWaitForever);
			}
			//crc ok
			else{		//data frame ok
				qPtr[qPtr[2]+3] = qPtr[qPtr[2]+3] | 0b00000011;	//READ = 1 ack = 1
				switch(){
					case :
						break;
					
					default:
						break;
				}
			}
			
				
		//----------------------------------------------------------------------------
		// Mem alloc				
		//----------------------------------------------------------------------------
			msg = osMemoryPoolAlloc(memPool,osWaitForever);											
			memcpy(msg,&qPtr[1],size-2);
			queueMsg.anyPtr = msg;
		}

	}
}
