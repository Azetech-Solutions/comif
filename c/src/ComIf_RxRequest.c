/*
 * ComIf_RxRequest.c
 *
 *  Created on: 08-Jun-2022
 *      Author: Hari
 */

/***************************************************/
/* ComIf Receive Request Source File               */
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

UBYTE ComIf_Rx_Request(ComIf_ChannelType ChannelIndex, UBYTE MsgIndex)
{
	UBYTE retval = COMIF_EC_GENERIC_ERROR;

	if(ChannelIndex < C_ComIfChannel_TOTAL)
	{
		ComIf_ChannelConfigType * Channel = &ComIf_ChannelConfig[ChannelIndex];

		if((Channel->ChannelFlags.IsRxRequestEnabled == TRUE) &&  (MsgIndex < Channel->TotalRxMessages))
		{
			ComIfRxMessageConfig * RxMsgCfg = &(Channel->RxMessages[MsgIndex]);

			// Set the Rx Request Flag
			RxMsgCfg->RxFlags.RxRequest = TRUE;

			retval = COMIF_EC_NO_ERROR;
		}
	}

	return retval;
}


UBYTE ComIf_Rx_CancelRequest(ComIf_ChannelType ChannelIndex, UBYTE MsgIndex)
{
	UBYTE retval = COMIF_EC_GENERIC_ERROR;

	if(ChannelIndex < C_ComIfChannel_TOTAL)
	{
		ComIf_ChannelConfigType * Channel = &ComIf_ChannelConfig[ChannelIndex];

		if((Channel->ChannelFlags.IsRxRequestEnabled == TRUE) &&  (MsgIndex < Channel->TotalRxMessages))
		{
			ComIfRxMessageConfig * RxMsgCfg = &(Channel->RxMessages[MsgIndex]);

			// Set the Rx Request Flag
			RxMsgCfg->RxFlags.RxRequest = FALSE;

			retval = COMIF_EC_NO_ERROR;
		}
	}

	return retval;
}


void ComIf_RxMainFunction(void)
{
	/* This cyclic main function is needed only for Rx Requests */
#ifdef COMIF_RX_REQUEST_ENABLED
	/* Loop All Channels */
	for(ComIf_ChannelType ChannelIndex = 0; ChannelIndex < C_ComIfChannel_TOTAL; ChannelIndex++)
	{
		ComIf_ChannelConfigType * Channel = &ComIf_ChannelConfig[ChannelIndex];

		if((Channel->ChannelFlags.IsRxRequestEnabled == TRUE) && (Channel->RequestReception != NULL))
		{
			// If the receive request is enabled for the channel, then loop thru the requested message and trigger for the request
			for(UBYTE i = 0; i < Channel->TotalRxMessages; i++)
			{
				ComIfRxMessageConfig * RxMsgCfg = &(Channel->RxMessages[i]);

				if(RxMsgCfg->RxFlags.RxRequest == TRUE)
				{
					if(RxMsgCfg->RxFlags.RxRequestPlaced == FALSE)
					{
						if(Channel->RequestReception(RxMsgCfg->ID) == COMIF_EC_NO_ERROR)
						{
							RxMsgCfg->RxFlags.RxRequestPlaced = TRUE;
						}

						// Load the timeout timer
						Channel->RxRequestTimeoutCounter = Channel->RxRequestTimeout;
					}
					else
					{
						// Wait for the reception to be completed for the placed request

						if(Channel->RxRequestTimeoutCounter > 0)
						{
							Channel->RxRequestTimeoutCounter--;
						}

						if(Channel->RxRequestTimeoutCounter == 0)
						{
							/* Report Error */
							ComIf_ErrorNotify(Channel, COMIF_EC_REQUEST_TIMEOUT, COMIF_DEBUG_DATA_WORDS(ChannelIndex, i));

							// Clear the request flags
							RxMsgCfg->RxFlags.RxRequest = FALSE;
							RxMsgCfg->RxFlags.RxRequestPlaced = FALSE;

							// Continue to next message as the current message is timed out
							continue;
						}
						// If the message is requested for reception and received already, then clear the flag
						else if((RxMsgCfg->RxFlags.NewMessageReceived == TRUE) || (RxMsgCfg->RxFlags.ErrorInReception == TRUE))
						{
							RxMsgCfg->RxFlags.RxRequest = FALSE;
							RxMsgCfg->RxFlags.RxRequestPlaced = FALSE;
						}
						else
						{
							// Wait for the reception to be completed
						}
					}

					// Break the message loop as the channel is processing or waiting for the current message to be received
					break;
				}
			}
		}
	}
#endif
}
