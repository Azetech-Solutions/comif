/*
 * ComIf_RxIndication.c
 *
 *  Created on: 08-Apr-2022
 *      Author: Hari
 */

/***************************************************/
/* ComIf Receive Indication Source File            */
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
static UBYTE ComIf_GetHandle(ComIfRxMessageConfig *RxMsgList, UBYTE Count,  UBYTE ID)
{
	/* Default handle ID assigned */
	UBYTE handle = P_COMIF_INVALID_HANDLE;

	for (UB i = 0 ; i < Count; i++ )
	{
		if(ID == RxMsgList[i].ID )
		{
			handle = i;
			break;
		}
	}
	return handle;
}

static void ComIf_ResetRxInfo(ComIf_ChannelConfigType * Channel, BOOL IsError)
{
	Channel->ChannelFlags.IsReceiving = FALSE;
	Channel->ChannelFlags.DLCVerified = FALSE;

	if(Channel->RxMsgIndex != P_COMIF_INVALID_HANDLE)
	{
		ComIfRxMessageConfig * RxMsg = &(Channel->RxMessages[Channel->RxMsgIndex]);

		RxMsg->CurRxngIdx = 0;
		RxMsg->RxFlags.ErrorInReception = IsError;
		RxMsg->RxFlags.NewMessageReceived = !IsError;
		RxMsg->RxFlags.ReceptionStarted = FALSE;
	}

	Channel->RxMsgIndex = P_COMIF_INVALID_HANDLE;
	Channel->RxMsgLength = 0;
}

static UBYTE ComIf_StoreDataByte(ComIf_ChannelConfigType * Channel, UBYTE DataByte)
{
	UBYTE retval = COMIF_EC_NO_ERROR;

	ComIfRxMessageConfig * RxMsg = &(Channel->RxMessages[Channel->RxMsgIndex]);

	if((RxMsg->CurRxngIdx) == (Channel->RxMsgLength + RxMsg->ChecksumLength))
	{
		// If the Maximum Message Buffer Length reached, then do not process further and report error
		retval = COMIF_EC_INVALID_MSG;

        /* Report Error */
        ComIf_ErrorNotify(Channel, retval, COMIF_DEBUG_DATA_WORDS(RxMsg->CurRxngIdx, DataByte));
        
		// Reset the Reception information
		ComIf_ResetRxInfo(Channel, TRUE);
	}
	else
	{
		RxMsg->MessageBuffer[RxMsg->CurRxngIdx] = DataByte;
		RxMsg->CurRxngIdx++;
	}

	return retval;
}

/**
 * ComIf_RxIndication
 *
 * This function will be called by the communication channel's driver source.
 * This function will eliminate the Start, Stop and Delimiter bytes and straight away give the
 * Rx Indication
 **/
UBYTE ComIf_RxIndication(UBYTE ChannelIndex, UBYTE DataByte)
{
	UBYTE retval = 0;

	if(ChannelIndex < C_ComIfChannel_TOTAL)
	{
		ComIf_ChannelConfigType * Channel = &ComIf_ChannelConfig[ChannelIndex];

		/* Check for Commands */
		if(Channel->ChannelFlags.Delimit == FALSE)
		{
			if(DataByte == COMIF_STX)
			{
				if((Channel->ChannelFlags.IsReceiving == TRUE) && (Channel->RxMsgIndex != P_COMIF_INVALID_HANDLE))
				{
					/**
					 * If a STX is received while receiving the previous message,
					 * then, it means the transmission of the previous message has been disturbed.
					 * In such scenarios, set the error for the previously receiving message and
					 * start reading the new message
					 **/

                    /* Report Error */
					ComIf_ErrorNotify(Channel, COMIF_EC_INVALID_MSG, COMIF_DEBUG_DATA_WORDS(COMIF_STX, Channel->RxMsgIndex));
					
					ComIf_ResetRxInfo(Channel, TRUE);
				}
				else
				{
					Channel->ChannelFlags.IsReceiving =TRUE;
				}

				/* Wait for the ID to be received */
				Channel->RxMsgIndex = P_COMIF_INVALID_HANDLE;
			}

			else if(Channel->ChannelFlags.IsReceiving == TRUE)
			{
				// Only Delimit the data bytes and Checksum, ID and DLC are not part of the de-limitation
				if((DataByte == COMIF_DLE) && (Channel->ChannelFlags.DLCVerified == TRUE))
				{
					Channel->ChannelFlags.Delimit = TRUE;
				}
				else if(DataByte == COMIF_ETX)
				{
					// If End of Transmission is received, then calculate checksum and give Rx Indication
					ComIfRxMessageConfig * RxMsg = &(Channel->RxMessages[Channel->RxMsgIndex]);
					UBYTE DataLength = Channel->RxMsgLength;
					UBYTE ChecksumLength = RxMsg->ChecksumLength;

					// Calculate Checksum if all the bytes were received
					// If the receive checksum is ignored, but the transmitted still sends the CHK, then the CurRxngIdx will be greater
					// Even though we receive more data, we only should pass the actual data length to the user
					if((RxMsg->CurRxngIdx) == (DataLength + ChecksumLength))
					{
						ULONG ReceivedChecksum = 0;
						ULONG CalcChecksum = 0;

						/* Validate Checksum only when the configured checksum length is greater than 0 */
						if(ChecksumLength > 0)
						{
							ULONG ChecksumMask = ComIf_GetChecksumMaskValue(ChecksumLength);
							CalcChecksum = ComIf_CalcChecksum(RxMsg->MessageBuffer, DataLength) & ChecksumMask;

							for(UBYTE i = 0; i < ChecksumLength; i++)
							{
								ReceivedChecksum <<= 8;
								ReceivedChecksum |= ((UL)(RxMsg->MessageBuffer[DataLength + i]));
							}

							ReceivedChecksum &= ChecksumMask;

						}

						if(ReceivedChecksum == CalcChecksum)
						{
							/* A Valid  Message is being received */

							/* Send RxCbk */
							if(RxMsg->RxCallback != NULL)
							{
								RxMsg->RxCallback(DataLength, RxMsg->MessageBuffer);
							}
                            
                            /* Reset the Rx Info as No Error */
							ComIf_ResetRxInfo(Channel, FALSE);
						}
						else
						{
							/* Reset the Rx Info as Error */
							ComIf_ResetRxInfo(Channel, TRUE);

                            /* Report Error */
							ComIf_ErrorNotify(Channel, COMIF_DEBUG_DATA_WORDS(COMIF_EC_INVALID_CHK, Channel->RxMsgIndex), CalcChecksum);
						}
					}
					else
					{
						/* If the received information is less than the configured one, then possibly the data might be lost */

                        /* Report Error */
						ComIf_ErrorNotify(Channel, COMIF_EC_INVALID_MSG, COMIF_DEBUG_DATA_BYTES(COMIF_ETX, RxMsg->CurRxngIdx, DataLength, ChecksumLength));
						
						/* Reset the Rx Info as Error */
						ComIf_ResetRxInfo(Channel, TRUE);
					}
				}
				else
				{
					/* If the MsgIndex is INVALID, then the reception is waiting for the ID */
					if(Channel->RxMsgIndex == P_COMIF_INVALID_HANDLE)
					{
						// This is the ID Byte
						Channel->RxMsgIndex = ComIf_GetHandle(Channel->RxMessages, Channel->TotalRxMessages, DataByte);

						/* If we can't able to identify the Message in the Handle */
						if(Channel->RxMsgIndex == P_COMIF_INVALID_HANDLE)
						{
							/* Then, reset the reception interfaces */
							ComIf_ResetRxInfo(Channel, TRUE);

                            /* Report Error */
							ComIf_ErrorNotify(Channel, COMIF_EC_INVALID_ID, COMIF_DEBUG_DATA_WORDS(ChannelIndex, DataByte));
						}
					}
					else
					{
						// ID is available
						if(Channel->ChannelFlags.DLCVerified == FALSE)
						{
							// If the DLC is not verified, then this byte could be the DLC byte
							// Split Data and Checksum Length
						#if (COMIF_MAX_SUPPORTED_CHKLEN == 1)
							UBYTE DataLength = DataByte;
							UBYTE ChecksumLength = 1;
						#else
							UBYTE DataLength = DataByte & 0x3F;
							UBYTE ChecksumLength = ((DataByte & 0xC0) >> 6) + 1; // CHKLEN Coding : 0 -> 1 Byte Checksum ... 3 -> 4 Byte Checksum, So add 1
						#endif

							ComIfRxMessageConfig * RxMsg = &(Channel->RxMessages[Channel->RxMsgIndex]);

							if( ((RxMsg->Length == DataLength)
						#ifdef COMIF_DYNAMIC_DATA_LENGTH_ENABLED
							|| ((RxMsg->Length == 0) && (DataLength <= Channel->DynamicMaxDataLength))
						#endif
							   )
							 && ((RxMsg->ChecksumLength == ChecksumLength) || (RxMsg->ChecksumLength == 0)) // Ignore the received checksum length if configured to ignore
							  )
							{
								Channel->ChannelFlags.DLCVerified = TRUE;
								Channel->RxMsgLength = DataLength;
							}
							else
							{
								// If the received DLC does not match with the configured DLC, then report error and stop reception
								ComIf_ResetRxInfo(Channel, TRUE);
                                
                                /* Report Error */
								ComIf_ErrorNotify(Channel, COMIF_EC_INVALID_DLC, COMIF_DEBUG_DATA_BYTES(ChannelIndex, Channel->RxMsgIndex, DataByte, (RxMsg->Length | ((RxMsg->ChecksumLength - 1) << 6)) ));
							}
						}
						else
						{
							// If ID and DLC verified, then the receiving bytes are the data bytes only
							// Store the Data
							ComIf_StoreDataByte(Channel, DataByte);
						}
					}
				}
			}
			else
			{
				// If the Data is not targeted to be received, or the STX is not received, then ignore the data bytes.
			}
		}
		else
		{
			if(Channel->RxMsgIndex != P_COMIF_INVALID_HANDLE)
			{
				// Store the Data
				ComIf_StoreDataByte(Channel, DataByte);
			}

			/* Once stored, clear the delimit flag */
			Channel->ChannelFlags.Delimit = FALSE;
		}

	}

	return retval;
}

UBYTE ComIf_RxIndication_ASCIIString(UBYTE Channel, char * Message, UWORD Length)
{
	UBYTE retval = 0;
	UBYTE Data = 0;

	if((Length % 2) == 0)
	{
		while(Length > 0)
		{
			Data = (UB)(((*Message - 0x30) << 4) | (*(Message + 1) - 0x30));

			retval = ComIf_RxIndication(Channel, Data);

			Message += 2; // Move to next step
			Length -= 2;

			/* If the string has an odd number length, then send the last byte also and exit */
			/* This should not actually happen, but still sent to record the exact error instead of ignoring */
			if(*(Message + 1) == '\0')
			{
				Data = (UB)((*Message - 0x30) << 4);
				retval = ComIf_RxIndication(Channel, Data);
				break;
			}
		}
	}
	else
	{
		// If the length is not an even number, then this is not formed properly.
		retval = COMIF_EC_FORM_ERROR;
	}

	return retval;
}

