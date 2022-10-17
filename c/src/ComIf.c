/*
 * ComIF.c
 *
 *  Created on: Apr 15, 2018
 *      Author: Surendar Vijayakumar
 */

/***************************************************/
/* ComIf Main Source File                          */
/***************************************************/

/***************************************************/
/* Header Inclusions                               */
/***************************************************/

#include "ComIf.h"

/***************************************************/
/* Global Variables Definitions                    */
/***************************************************/


/***************************************************/
/* Function Definitions                            */
/***************************************************/

UBYTE ComIf_GetTxMsgHandle(ComIf_ChannelType Channel, UBYTE ID)
{
    UBYTE handle = P_COMIF_INVALID_HANDLE;
    
    if(Channel < C_ComIfChannel_TOTAL)
    {
        ComIfTxMessageConfig *TxMsgCfgPtr = ComIf_ChannelConfig[Channel].TxMessages;
        UBYTE TotalTxMessages = ComIf_ChannelConfig[Channel].TotalTxMessages;
        
        for(UBYTE i = 0; i < TotalTxMessages; i++)
        {
            if(TxMsgCfgPtr[i].ID == ID)
            {
                handle = i;
                break;
            }
        }
    }
    
    return handle;
}

UBYTE ComIf_GetRxMsgHandle(ComIf_ChannelType Channel, UBYTE ID)
{
    UBYTE handle = P_COMIF_INVALID_HANDLE;
    
    if(Channel < C_ComIfChannel_TOTAL)
    {
        ComIfRxMessageConfig *RxMsgCfgPtr = ComIf_ChannelConfig[Channel].RxMessages;
        UBYTE TotalRxMessages = ComIf_ChannelConfig[Channel].TotalRxMessages;
        
        for(UBYTE i = 0; i < TotalRxMessages; i++)
        {
            if(RxMsgCfgPtr[i].ID == ID)
            {
                handle = i;
                break;
            }
        }
    }
    
    return handle;
}

void ComIf_ErrorNotify(ComIf_ChannelConfigType * Channel, ULONG Debug0, ULONG Debug1)
{
	if(Channel->ErrorCallback != NULL)
	{
		Channel->ErrorCallback(Debug0, Debug1);
	}
}
