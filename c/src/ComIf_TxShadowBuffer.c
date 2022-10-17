/*
 * ComIf_TxShadowBuffer.c
 *
 *  Created on: 09-Apr-2022
 *      Author: Hari
 */

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

/* Function to get the Shadow Buffer */
UBYTE * ComIf_GetTxShadowBuffer(UBYTE ChannelIndex, UBYTE MsgIndex)
{
    UBYTE * BufPtr = NULL;
    
    if(ChannelIndex < C_ComIfChannel_TOTAL)
	{
		ComIf_ChannelConfigType * Channel = &ComIf_ChannelConfig[ChannelIndex];

		if(MsgIndex < Channel->TotalTxMessages)
		{
			ComIfTxMessageConfig * TxMsgCfg = &(Channel->TxMessages[MsgIndex]);
            
            BufPtr = TxMsgCfg->ShadowBuffer;
        }
    }
    
    return BufPtr;
}

/* Function to Update the Shadow Buffer */
UBYTE ComIf_UpdateTxShadowBuffer(UBYTE ChannelIndex, UBYTE MsgIndex, void * DataPtr, UBYTE Length)
{
	UBYTE retval = COMIF_EC_GENERIC_ERROR;

	if(ChannelIndex < C_ComIfChannel_TOTAL)
	{
		ComIf_ChannelConfigType * Channel = &ComIf_ChannelConfig[ChannelIndex];

		if(MsgIndex < Channel->TotalTxMessages)
		{
			ComIfTxMessageConfig * TxMsgCfg = &(Channel->TxMessages[MsgIndex]);

			if(
		#if (COMIF_MAX_SUPPORTED_DLC < 255)
				((TxMsgCfg->Length <= COMIF_MAX_SUPPORTED_DLC) && (Length == TxMsgCfg->Length))
		#else
				(Length == TxMsgCfg->Length)
		#endif
		#ifdef COMIF_DYNAMIC_DATA_LENGTH_ENABLED
			 || ((TxMsgCfg->Length == 0) && (Length != 0) && (Length <= Channel->DynamicMaxDataLength))
		#endif
			)
			{
				int i = 0;

				/* Check if the data needs to be updated from the TxCbk */
				if(TxMsgCfg->TxCallback != NULL)
				{
					retval = TxMsgCfg->TxCallback((UBYTE*)DataPtr, &Length);
				}
				else
				{
					retval = COMIF_EC_NO_ERROR;
				}

				if(retval == COMIF_EC_NO_ERROR)
				{
					ComIf_Lock_TxShadowBuffer();
					/* Copy Data */
					for(i = 0; i < Length; i++)
					{
						TxMsgCfg->ShadowBuffer[i] = ((UBYTE*)DataPtr)[i];
					}
					ComIf_UnLock_TxShadowBuffer();

					retval = COMIF_EC_NO_ERROR;
				}
			}
			else
			{
				retval = COMIF_EC_INVALID_DLC;
			}
		}
		else
		{
			retval = COMIF_EC_INVALID_MSG;
		}
	}
	else
	{
		retval = COMIF_EC_INVALID_CHANNEL;
	}

	return retval;
}

/* Transmit From Shadow Buffer */
UBYTE ComIf_Transmit_ShadowBuffer(UBYTE ChannelIndex, UBYTE MsgIndex, UBYTE Length)
{
    UBYTE * BufPtr = ComIf_GetTxShadowBuffer(ChannelIndex, MsgIndex);
    
    if(BufPtr != NULL)
    {
        return ComIf_Transmit(ChannelIndex, MsgIndex, (void *)BufPtr, Length);
    }
    
    return COMIF_EC_GENERIC_ERROR;
}
