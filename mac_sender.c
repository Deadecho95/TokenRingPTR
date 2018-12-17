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
	uint8_t * oldMsgPtr;
	uint8_t * tokenPtr;
	osStatus_t retCode;
	uint8_t * msg;
	uint8_t * qPtr;
	uint8_t crc = 0;

	

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
					// Mem alloc (keep old msg)			
					//----------------------------------------------------------------------------
						msg = osMemoryPoolAlloc(memPool,osWaitForever);						
						memset(msg,0,17);
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
				gTokenInterface.connected = 1;
				break;

			case STOP :
				gTokenInterface.connected = 0;
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
				tokenPtr[gTokenInterface.myAddress+1] = 10;
				}
				else{
				tokenPtr[gTokenInterface.myAddress+1] = 8;
				}
				for(int i = 0; i < 16; i++){
					gTokenInterface.station_list[i] = *(tokenPtr+i+1);
				}
				
				//--------------------------------------------------------------------------
				// QUEUE SEND	(send msg to lcd)
				//--------------------------------------------------------------------------
				queueMsg.type = TOKEN_LIST;		
				retCode = osMessageQueuePut(
					queue_lcd_id,
					&queueMsg,
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
				
				//----------------------------------------------------------------------------
				// QUEUE READ (read the buffer)							
				//----------------------------------------------------------------------------
				retCode = osMessageQueueGet( 	
					buffer,
					&queueMsg,
					NULL,
					NULL); 	
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);

				
				if(retCode == osOK){
					//----------------------------------------------------------------------------
					// Mem alloc (creae data to send to the other station)	
					//----------------------------------------------------------------------------
					msg = osMemoryPoolAlloc(memPool,osWaitForever);
					memset(msg,0,MAX_BLOCK_SIZE);
					*(msg) = (gTokenInterface.myAddress << 3) + queueMsg.sapi;//control scr
					*(msg+1) = (queueMsg.addr <<3) + queueMsg.sapi;						//control dest
					*(msg+2) = strlen(queueMsg.anyPtr);												//length
					memcpy(msg+3,queueMsg.anyPtr,*(msg+2));										//copy data
					//crc//
					crc = 0;
					for(int i = 0; i < *(msg+2)+3; i++){
						crc = crc + *(msg+i);
					}
					crc = crc&0x3f;
					*(msg+msg[2]+3) = crc<<2;
					
					//------------------------------------------------------------------------
					// MEMORY RELEASE	(delete data ind)
					//------------------------------------------------------------------------
					retCode = osMemoryPoolFree(memPool,queueMsg.anyPtr);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);					
					
					//----------------------------------------------------------------------------
					// Mem alloc				
					//----------------------------------------------------------------------------
					oldMsgPtr = osMemoryPoolAlloc(memPool,osWaitForever);			
					memcpy(oldMsgPtr,msg,msg[2]+4);

					
					//--------------------------------------------------------------------------
					// QUEUE SEND	(send msg to phy)
					//--------------------------------------------------------------------------
					queueMsg.type = TO_PHY;
					queueMsg.anyPtr = msg;
					
					retCode = osMessageQueuePut(
						queue_phyS_id,
						&queueMsg,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);


				}
				else{
					//--------------------------------------------------------------------------
					// QUEUE SEND	(send token to the next station)
					//--------------------------------------------------------------------------
					queueMsg.anyPtr = tokenPtr;
					queueMsg.type = TO_PHY;
					retCode = osMessageQueuePut(
						queue_phyS_id,
						&queueMsg,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
				}
				break;	

			case DATABACK :

	//----------------------------------------------------------------------------
	// Test Frame				
	//----------------------------------------------------------------------------
		qPtr = queueMsg.anyPtr;
		
		if((qPtr[qPtr[2]+3]&1) == 1)//ack ok
		{
			//------------------------------------------------------------------------
			// MEMORY RELEASE	(delete old msg)
			//------------------------------------------------------------------------
			retCode = osMemoryPoolFree(memPool,oldMsgPtr);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
			
			//--------------------------------------------------------------------------
			// QUEUE SEND	(send token to phy)
			//--------------------------------------------------------------------------
			queueMsg.type = TO_PHY;
			queueMsg.anyPtr = tokenPtr;
			
			retCode = osMessageQueuePut(
				queue_phyS_id,
				&queueMsg,
				osPriorityNormal,
				osWaitForever);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
		}
		else if(((qPtr[qPtr[2]+3]>>1)&0x1) == 0)// not read
		{
			
			//----------------------------------------------------------------------------
			// Mem alloc				
			//----------------------------------------------------------------------------
			msg = osMemoryPoolAlloc(memPool,osWaitForever);						
			memset(msg,0,17);
			sprintf ((char*)msg, "station %d not respond\r\n", ((oldMsgPtr[1]&0x78)>>3)+1);
			
			queueMsg.anyPtr = msg;
			queueMsg.type = MAC_ERROR;
			
			//------------------------------------------------------------------------
			// MEMORY RELEASE	(delete old msg)
			//------------------------------------------------------------------------
			retCode = osMemoryPoolFree(memPool,oldMsgPtr);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
			
			//--------------------------------------------------------------------------
			// QUEUE SEND	(send an error)
			//--------------------------------------------------------------------------
			queueMsg.type = MAC_ERROR;
			queueMsg.anyPtr = msg;
			queueMsg.addr = (oldMsgPtr[0] & 0x78)>>3;
			
			retCode = osMessageQueuePut(
				queue_lcd_id,
				&queueMsg,
				osPriorityNormal,
				osWaitForever);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);


			
			//--------------------------------------------------------------------------
			// QUEUE SEND	(send token to phy)
			//--------------------------------------------------------------------------
			queueMsg.type = TO_PHY;
			queueMsg.anyPtr = tokenPtr;
			
			retCode = osMessageQueuePut(
				queue_phyS_id,
				&queueMsg,
				osPriorityNormal,
				osWaitForever);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);				
		}
		else{//ack not ok
			//--------------------------------------------------------------------------
			// QUEUE SEND	(send oldmsg to phy)
			//--------------------------------------------------------------------------
			queueMsg.type = TO_PHY;
			queueMsg.anyPtr = oldMsgPtr;
			
			retCode = osMessageQueuePut(
				queue_phyS_id,
				&queueMsg,
				osPriorityNormal,
				osWaitForever);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
		}
		
			//------------------------------------------------------------------------
			// MEMORY RELEASE	(delete data ind)
			//------------------------------------------------------------------------
			retCode = osMemoryPoolFree(memPool,qPtr);
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);			
				break;				
			
			default:
				break;
		}
		
		
		
		
		
	}
}
