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
	osMessageQueueId_t queue;
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
		if(qPtr[0] == TOKEN_TAG){						// token frame
			queueMsg.type = TOKEN;
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
			crc = crc&0x3f;
			//crcr not ok
			if(crc != ((qPtr[qPtr[2]+3])&0xfc)>>2){
				qPtr[qPtr[2]+3] = qPtr[qPtr[2]+3] | 0x02;	//READ = 1
				retCode = osMessageQueuePut(
					queue_phyS_id,
					&queueMsg,
					NULL,
					osWaitForever);
			}
			//crc ok
			else{		//data frame ok
				qPtr[qPtr[2]+3] = qPtr[qPtr[2]+3] | 0x03;	//READ = 1 ack = 1
				//source of not
				if((qPtr[0]&0xf8>>3) == gTokenInterface.myAddress){//if source
					//if dest or not
					if((qPtr[1]&0xf8>>3) == gTokenInterface.myAddress || (qPtr[1]&0xf8>>3) == BROADCAST_ADDRESS ){//if dest
						//sapi chat
						if((qPtr[1]&0x07) == CHAT_SAPI && gTokenInterface.connected == 1 ){//chat
							queue = queue_chatR_id;
						}
						else{
							queue = queue_timeR_id;
						}//time
					  //--------------
						//databack
						//--------------
						queueMsg.type = DATABACK;
						queueMsg.addr = (qPtr[0]&0x78)>>3;
						queueMsg.sapi = (qPtr[0]&0x07);
						retCode = osMessageQueuePut(
							queue_macS_id,
							&queueMsg,
							NULL,
							osWaitForever);
						//----------------------------------------------------------------------------
						// Mem alloc				
						//----------------------------------------------------------------------------
							msg = osMemoryPoolAlloc(memPool,osWaitForever);											
							memcpy(msg,&qPtr[3],size-1);
							queueMsg.anyPtr = msg;
							queueMsg.type = DATA_IND;
							queueMsg.addr = (qPtr[0]&0x78)>>3;
							queueMsg.sapi = (qPtr[0]&0x07);
						//--------------------------------------------------------------------------
						// QUEUE SEND	(send received frame to chat reciver)
						//--------------------------------------------------------------------------
						retCode = osMessageQueuePut(
							queue,
							&queueMsg,
							osPriorityNormal,
							osWaitForever);
						CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
						
					}
					else{//not dest
						//databack
					}
				}
				else{//not source
					//todo
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
