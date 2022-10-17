/*
 * ComIf_Transmit.c
 *
 *  Created on: 08-Apr-2022
 *      Author: Hari
 */

/***************************************************/
/* ComIf Transmission Source File                  */
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

static inline void ConvertByteToString(unsigned char * Str, UBYTE Value)
{
	*Str = (((Value & 0xF0) >> 4) + 0x30);
	*(Str + 1) = ((Value & 0x0F) + 0x30);
}

static inline BOOL ComIf_IsNeedDelimiter(UBYTE Data)
{
	if( (Data == COMIF_STX)
	 || (Data == COMIF_DLE)
	 || (Data == COMIF_ETX)
	)
	{
		return TRUE;
	}
	return FALSE;
}

static inline UBYTE ComIf_AppendByte(UBYTE * DataPtr, UBYTE Data)
{
	UBYTE AppendLength = 0;

	if( (Data == COMIF_STX)
	 || (Data == COMIF_DLE)
	 || (Data == COMIF_ETX)
	)
	{
		// If there are any control characters available in the Data, then add a Delimiter
		DataPtr[AppendLength] = COMIF_DLE;
		AppendLength++;
	}

	DataPtr[AppendLength] = Data;

	return AppendLength + 1; // Add 1 more byte as the return value should hold the length instead of index
}

static inline UBYTE ComIf_AppendString(UBYTE * DataPtr, UBYTE Data)
{
	UBYTE AppendLength = 0;

	if( (Data == COMIF_STX)
	 || (Data == COMIF_DLE)
	 || (Data == COMIF_ETX)
	)
	{
		// If there are any control characters available in the Data, then add a Delimiter
		ConvertByteToString(DataPtr, COMIF_DLE);
		AppendLength += 2;
	}

	ConvertByteToString(&(DataPtr[AppendLength]), Data);
	AppendLength += 2;

	return AppendLength;
}

static inline UWORD ComIf_GetChecksumAndTentativeDataLength(UBYTE ChannelIndex, UBYTE MsgIndex, UBYTE Length, ULONG *Checksum)
{
	ULONG ChecksumMask = 0;
	UWORD TentativeDataLength = 1 + 1 + 1 + 1; // STX, ID, DLC, ETX

	ComIf_ChannelConfigType * Channel = &ComIf_ChannelConfig[ChannelIndex];

	// Once the update shadow buffer return no error, then it means the channel and msg indices were valid
	// So, we can simply get the information from the channel and msg indices

	ComIfTxMessageConfig * TxMsgCfg = &(Channel->TxMessages[MsgIndex]);

	/* Calculate Checksum for the data */
	ComIf_Lock_TxShadowBuffer();
	{
		for(UBYTE i = 0; i < Length; i++)
		{
			UBYTE DataByte = TxMsgCfg->ShadowBuffer[i];

			*Checksum += DataByte;

			TentativeDataLength++;

			if(ComIf_IsNeedDelimiter(DataByte)) { TentativeDataLength++; }
		}
	}
	ComIf_UnLock_TxShadowBuffer();

	ChecksumMask = ComIf_GetChecksumMaskValue(TxMsgCfg->ChecksumLength);

	(*Checksum) = (((~(*Checksum)) + 1) & ChecksumMask);

	for(int i = TxMsgCfg->ChecksumLength - 1; i >= 0; i--)
	{
		UBYTE ChkData = (UB)(((*Checksum) & (((UL)0xFF) << (i * 8))) >> (i * 8));

		TentativeDataLength++;

		if(ComIf_IsNeedDelimiter(ChkData)) { TentativeDataLength++; }
	}

	if(Channel->DataTxfrMethod == COMIF_DATA_TXFR_METHOD_STRING)
	{
		/* If the Data transfer method is STRING, then add 2 chars per byte and a NULL character at the end */
		TentativeDataLength = (TentativeDataLength * 2) + 1;
	}

	return TentativeDataLength;
}



UBYTE ComIf_EncodeFromShadowToChannelBuffer(UBYTE ChannelIndex, UBYTE MsgIndex, UBYTE Length, UWORD * OutLength, UBYTE * OutBuffer, UWORD BoundLimit)
{
	UBYTE retval = COMIF_EC_NO_ERROR;

	ULONG Checksum = 0;
	UWORD CurrentIndex = 0;
	UWORD TentativeDataLength = 0;

	ComIf_ChannelConfigType * Channel = &ComIf_ChannelConfig[ChannelIndex];

	// Once the update shadow buffer return no error, then it means the channel and msg indices were valid
	// So, we can simply get the information from the channel and msg indices

	ComIfTxMessageConfig * TxMsgCfg = &(Channel->TxMessages[MsgIndex]);

	/* Calculate DLC */
#if (COMIF_MAX_SUPPORTED_CHKLEN == 1)
	// If only one byte checksum is supported, simply assign the length as the DLC
	UBYTE DLC = Length;
#else
#if ((COMIF_MAX_SUPPORTED_CHKLEN > 4) || (COMIF_MAX_SUPPORTED_DLC > 63))
#error "Invalid ComIf Supported Length configuration!"
#endif
	// If only more than 1 byte checksum is supported, then OR the values to the DLC
	UBYTE DLC = (UB)((Length & 0x3F) | (((TxMsgCfg->ChecksumLength - 1) & 0x03) << 6));
#endif

	TentativeDataLength = ComIf_GetChecksumAndTentativeDataLength(ChannelIndex, MsgIndex, Length, &Checksum);

	/* Build Data Buffer */
	if(Channel->DataTxfrMethod == COMIF_DATA_TXFR_METHOD_STRING)
	{
		/* Check if the channel buffer has enough space to fill in */
		if(TentativeDataLength < BoundLimit)
		{
			ConvertByteToString(&(OutBuffer[CurrentIndex]), COMIF_STX);
			CurrentIndex += 2;

			ConvertByteToString(&(OutBuffer[CurrentIndex]), TxMsgCfg->ID);
			CurrentIndex += 2;

			ConvertByteToString(&(OutBuffer[CurrentIndex]), DLC);
			CurrentIndex += 2;

			ComIf_Lock_TxShadowBuffer();
			{
				for(UBYTE i = 0; i < Length; i++)
				{
					CurrentIndex += ComIf_AppendString(&(OutBuffer[CurrentIndex]), TxMsgCfg->ShadowBuffer[i]);
				}
			}
			ComIf_UnLock_TxShadowBuffer();

			for(int i = TxMsgCfg->ChecksumLength - 1; i >= 0; i--)
			{
				UBYTE ChkData = (UB)((Checksum & (((UL)0xFF) << (i * 8))) >> (i * 8));

				CurrentIndex += ComIf_AppendString(&(OutBuffer[CurrentIndex]), ChkData);
			}

			ConvertByteToString(&(OutBuffer[CurrentIndex]), COMIF_ETX);
			CurrentIndex += 2;

			OutBuffer[CurrentIndex++] = '\0';

			*OutLength = CurrentIndex;
		}
		else
		{
			retval = COMIF_EC_BUFFER_OVERFLOW;

            /* Report Error */
			ComIf_ErrorNotify(Channel, COMIF_DEBUG_DATA_BYTES(0, retval, ChannelIndex, MsgIndex), COMIF_DEBUG_DATA_WORDS(TentativeDataLength, BoundLimit));
		}
	}
	else
	{
		/* If Data Transfer Method is COMIF_DATA_TXFR_METHOD_BYTES */

		if(TentativeDataLength < BoundLimit)
		{
			OutBuffer[CurrentIndex++] = COMIF_STX;
			OutBuffer[CurrentIndex++] = TxMsgCfg->ID;
			OutBuffer[CurrentIndex++] = DLC;

			ComIf_Lock_TxShadowBuffer();
			{
				for(UBYTE i = 0; i < Length; i++)
				{
					CurrentIndex += ComIf_AppendByte(&(OutBuffer[CurrentIndex]), TxMsgCfg->ShadowBuffer[i]);
				}
			}
			ComIf_UnLock_TxShadowBuffer();


			for(int i = TxMsgCfg->ChecksumLength - 1; i >= 0; i--)
			{
				UBYTE ChkData = (UB)((Checksum & (((UL)0xFF) << (i * 8))) >> (i * 8));

				CurrentIndex += ComIf_AppendByte(&(OutBuffer[CurrentIndex]), ChkData);
			}

			OutBuffer[CurrentIndex++] = COMIF_ETX;

			*OutLength = CurrentIndex;
		}
		else
		{
			retval = COMIF_EC_BUFFER_OVERFLOW;

			/* Report Error */
			ComIf_ErrorNotify(Channel, COMIF_DEBUG_DATA_BYTES(0, retval, ChannelIndex, MsgIndex), COMIF_DEBUG_DATA_WORDS(TentativeDataLength, BoundLimit));
		}
	}

	return retval;
}

/**
 * ComIf_Transmit
 *
 * This is the API to trigger transmission to the communication channels.
 *
 * REENTRANCY: This function should be Non-Reentrant for the same Channel.
 *
 * TODO: In Future, This function shall be made Non-Reentrant for the same message.
 *
 * Usage Notes: This function shall be called for a channel in a same task to avoid issues.
 **/
UBYTE ComIf_Transmit(UBYTE ChannelIndex, UBYTE MsgIndex, void * DataPtr, UBYTE Length)
{
	/* Copy the Data to the Message specific shadow buffer before processing it any further */
	/* This function will also acts as a validity checks for the passed arguments */
	UBYTE retval = ComIf_UpdateTxShadowBuffer(ChannelIndex, MsgIndex, DataPtr, Length);

	if(retval == COMIF_EC_NO_ERROR)
	{
		ComIf_ChannelConfigType * Channel = &ComIf_ChannelConfig[ChannelIndex];

		ComIfTxMessageConfig * TxMsgCfg = &(Channel->TxMessages[MsgIndex]);

		UWORD RemainingBufferLength = Channel->TxBufferLength; // The whole buffer is available for now

		UBYTE * ChannelDataBuffer = &Channel->TxBuffer[0];

		UWORD EncodedLength = 0;

		#ifdef COMIF_TX_AGGREGATION_ENABLED
		ComIf_TxAggCtrl_t * TxAggCtrl = Channel->TxAggCtrl;

		UWORD CurrentlyAggregatedBufferLength = 0;

		UBYTE CurrentMessageRetVal = COMIF_EC_NO_ERROR;
		#endif

		ComIf_Lock_Transmit();
		{
			/**
			 * If the channel is already transmitting, simply return.
			 * This flag has two uses:
			 * 1. Acts as a Mutex by avoid updating of the Channel Data Buffer when this function is called re-entrantly for the same channel
			 * 2. Acts as a Blocker to avoid updating of the Channel Data Buffer when the channel is communicating, but not completed yet
			 * So, this check should be in the top of any logic that is to be implemented.
			 **/
			if(Channel->ChannelFlags.IsTransmitting == FALSE)
			{
				Channel->ChannelFlags.IsTransmitting = TRUE;
			}
			else
			{
				/* Return Channel BUSY if the channel is already transmitting */
				return COMIF_EC_CHANNEL_BUSY;
			}
		}
		ComIf_UnLock_Transmit();

		#ifdef COMIF_TX_AGGREGATION_ENABLED
		/**
		 * If Aggregation support is enabled for the message and channel, then simply add for aggregation
		 * and send NO-ERROR
		 **/
		if((Channel->ChannelFlags.IsAggregationEnabled) && (TxAggCtrl != NULL))
		{
			if((TxMsgCfg->TxFlags.EnableAggregation) && (TxMsgCfg->TxFlags.ForceTransmit == FALSE))
			{
				ULONG DummyChecksum = 0;
				UWORD TentativeDataLength = 0;

				// Get the tentative data length for the message to be aggregated
				TentativeDataLength = ComIf_GetChecksumAndTentativeDataLength(ChannelIndex, MsgIndex, TxMsgCfg->Length, &DummyChecksum);

				// If the new message is within the buffer limit
				if((TxAggCtrl->CurrentBufferLength + TentativeDataLength) < TxAggCtrl->BufferLimit)
				{
					// Aggregate the message
					TxMsgCfg->TxFlags.PendingForTransmission = TRUE;

					// Set the channel flags to true
					Channel->ChannelFlags.HasAggregatedMsgs = TRUE;

					TxAggCtrl->CurrentBufferLength += TentativeDataLength;

					// If the Message is aggregated, then it means it is not being transmitted, so clear the Channel Transmitting flag
					Channel->ChannelFlags.IsTransmitting = FALSE;

					return COMIF_EC_NO_ERROR;
				}
				else
				{
					// If the new message exceeds the buffer limit, then simply trigger the message with the aggregation
				}
			}
		}

		/* In no single call to this function, a message will be aggregated and transmitted! */

		/* Check if the Channel has any messages aggregated */
		if(Channel->ChannelFlags.HasAggregatedMsgs)
		{
			// If there are any aggregated messages, then prepend those messages before appending the currently passed message
			for(UBYTE i = 0; i < Channel->TotalTxMessages; i++)
			{
				ComIfTxMessageConfig * AggTxMsgCfg = &(Channel->TxMessages[i]);

				if(AggTxMsgCfg->TxFlags.PendingForTransmission)
				{
					// Aggregated messages do not change the data length dynamically, so no need to handle the Length argument passed
					retval = ComIf_EncodeFromShadowToChannelBuffer(ChannelIndex, i, AggTxMsgCfg->Length, &EncodedLength, ChannelDataBuffer, RemainingBufferLength);

					// If the data is appended, then update the flags accordingly
					if(retval == COMIF_EC_NO_ERROR)
					{
						// NOTE: Do not clear the pending for transmission flag here. If the aggregation failed, then we need to retry in next cycle

						// Set the AggregatedInBuffer flag to TRUE
						AggTxMsgCfg->TxFlags.AggregatedInBuffer = TRUE;

						// Add the Encoded length to the aggregate buffer length
						CurrentlyAggregatedBufferLength += EncodedLength;

						// Update the Buffer pointer to point at the end of the newly added message
						ChannelDataBuffer += EncodedLength;

						// Update the remaining buffer size to reduce the newly added message length
						RemainingBufferLength -= EncodedLength;

						// Reset the local variable
						EncodedLength = 0;
					}
					else
					{
						// If there is an error, most likely, the channel buffer is full, in such case, retry the transmission in next cycle
						// Do nothing for now
					}
				}
			}
		}
		#endif

		retval = ComIf_EncodeFromShadowToChannelBuffer(ChannelIndex, MsgIndex, Length, &EncodedLength, ChannelDataBuffer, RemainingBufferLength);

		#ifdef COMIF_TX_AGGREGATION_ENABLED

		// If Aggregation is enabled, there is a possibility that some aggregated messages might be prepended in the Channel data buffer

		// Add the aggregated message length to the encoded length to ensure the transmit function transmits all the data
		EncodedLength += CurrentlyAggregatedBufferLength;

		// During aggregation, this pointer might be changed multiple times to point at the end of the new messages being added
		// Reset the value to the initial position so that the transmit function copied the data from the start
		ChannelDataBuffer = &Channel->TxBuffer[0];

		#endif

		if(retval == COMIF_EC_NO_ERROR)
		{
			retval = Channel->Transmit(EncodedLength, ChannelDataBuffer);

			// Channel data buffer has been transmitted properly, then set the PendingConfirmation flag for the sent message
			if(retval == COMIF_EC_NO_ERROR)
			{
				TxMsgCfg->TxFlags.PendingForConfirmation = TRUE;
			}
		}
	#ifdef COMIF_TX_AGGREGATION_ENABLED
		else if(retval == COMIF_EC_BUFFER_OVERFLOW)
		{
			/**
			 * While aggregating is enabled, there is a possibility that the buffer might be full, so the new message might not get
			 * enough space to copy it's data, in such case, we need to send the already aggregated messages in the channel data buffer
			 * to clear the buffer space, so that in the next call, the message will be transmitted
			 * Also, we must ensure that the retval for the actual message index being requested for channel returns BUFFER_OVERFLOW
			 * This will intimate the user to re-try the transmission again. Or else, the user might wait and not get any confirmation at all
			 **/

			CurrentMessageRetVal = retval;

			// Try sending the buffer over the channel
			retval = Channel->Transmit(EncodedLength, ChannelDataBuffer);
		}
	#endif
		else
		{
			// Do Nothing
		}


		/* If there are problems in transmitting the data buffer in the channel */
		if(retval != COMIF_EC_NO_ERROR)
		{
			/* Clear the Transmitting flag as there is an error */
			Channel->ChannelFlags.IsTransmitting = FALSE;

			/**
			 * If there are any errors in the transmission of aggregated messages, then do not update the contents AggregatedInBuffer flag
			 * or clear the buffer, keep the aggregated messages in the buffer and attempt retrying in the next task cycle
			 **/

			/* Report Error */
			ComIf_ErrorNotify(Channel, retval, COMIF_DEBUG_DATA_WORDS(ChannelIndex, MsgIndex));
		}
	#ifdef COMIF_TX_AGGREGATION_ENABLED
		else
		{
			// All messages were transmitted successfully
			// For the currently requested frame, then set the confirmation pending flag!

			/* Loop all messages to get the aggregated ones */
			for(UB i = 0; i < Channel->TotalTxMessages; i++)
			{
				ComIfTxMessageConfig * AggTxMsgCfg = &(Channel->TxMessages[i]);

				if(AggTxMsgCfg->TxFlags.AggregatedInBuffer == TRUE)
				{
					// Clear the Aggregated in Buffer flag
					AggTxMsgCfg->TxFlags.AggregatedInBuffer = FALSE;

					// Clear the Transmission pending flag
					AggTxMsgCfg->TxFlags.PendingForTransmission = FALSE;

					// Set the pending for confirmation Flag
					AggTxMsgCfg->TxFlags.PendingForConfirmation = TRUE;
				}
			}

			// If all the messages aggregated has been transmitted successfully, then reset the counters
			if(CurrentlyAggregatedBufferLength >= TxAggCtrl->CurrentBufferLength)
			{
				// Reload the timer
				TxAggCtrl->TxCycleCounter = TxAggCtrl->TxCycle;

				// Reset the buffer limit
				TxAggCtrl->CurrentBufferLength = 0;

				// Reset the Channel Flags
				Channel->ChannelFlags.HasAggregatedMsgs = FALSE;
			}
			else
			{
				// If there are still some pending messages to be transmitted, then subtract the currently aggregated length from the total length and retry next cycle
				TxAggCtrl->CurrentBufferLength -= CurrentlyAggregatedBufferLength;
			}

			/**
			 * Only update the current message return value of BUFFER_OVERFLOW when all the aggregated messages were
			 * transmitted successfully, and there are no other errors except this overflow error.
			 * For all other errors, the retval variable will already hold the respective flags, buffer overflow is the least
			 * priority of them all
			 **/
			retval = CurrentMessageRetVal;
		}
	#endif
	}
    else
    {
    	/* If atleast the channel is valid, then report the channel specific error */
    	if(ChannelIndex < C_ComIfChannel_TOTAL)
    	{
    		ComIf_ChannelConfigType * Channel = &ComIf_ChannelConfig[ChannelIndex];

    		/* Report Error */
    		ComIf_ErrorNotify(Channel, retval, COMIF_DEBUG_DATA_WORDS(ChannelIndex, MsgIndex));
    	}
    }

	return retval;
}


/* This will be called when the last message transmissions was successful */
void ComIf_TxConfirmation(UBYTE ChannelIndex)
{
	if(ChannelIndex < C_ComIfChannel_TOTAL)
	{
		ComIf_ChannelConfigType * Channel = &ComIf_ChannelConfig[ChannelIndex];

		ComIf_Lock_Transmit();
		Channel->ChannelFlags.IsTransmitting = FALSE;
		ComIf_UnLock_Transmit();

		for(UB i = 0; i < Channel->TotalTxMessages; i++)
		{
			ComIfTxMessageConfig * TxMsgCfg = &(Channel->TxMessages[i]);

			if(TxMsgCfg->TxFlags.PendingForConfirmation == TRUE)
			{
				TxMsgCfg->TxFlags.PendingForConfirmation = FALSE;

				if(TxMsgCfg->TxConfCallback != NULL)
				{
					TxMsgCfg->TxConfCallback();
				}
			}
		}
	}
}

/* This will be called when the current transmissions are aborted */
void ComIf_TransmissionAborted(UBYTE ChannelIndex)
{
	if(ChannelIndex < C_ComIfChannel_TOTAL)
	{
		ComIf_ChannelConfigType * Channel = &ComIf_ChannelConfig[ChannelIndex];

		ComIf_Lock_Transmit();
		Channel->ChannelFlags.IsTransmitting = FALSE;
		ComIf_UnLock_Transmit();

		for(UB i = 0; i < Channel->TotalTxMessages; i++)
		{
			ComIfTxMessageConfig * TxMsgCfg = &(Channel->TxMessages[i]);

			if(TxMsgCfg->TxFlags.PendingForConfirmation == TRUE)
			{
				TxMsgCfg->TxFlags.PendingForConfirmation = FALSE;
			}
		}
	}
}

void ComIf_TxMainFunction(void)
{
	/* This cyclic main function is needed only for Tx Aggregation */
#ifdef COMIF_TX_AGGREGATION_ENABLED

	/* Loop All Channels */
	for(ComIf_ChannelType ChannelIndex = 0; ChannelIndex < C_ComIfChannel_TOTAL; ChannelIndex++)
	{
		ComIf_ChannelConfigType * Channel = &ComIf_ChannelConfig[ChannelIndex];

		ComIf_TxAggCtrl_t * TxAggCtrl = Channel->TxAggCtrl;

		if((Channel->ChannelFlags.IsAggregationEnabled) && (TxAggCtrl != NULL))
		{
			/* Decrement the Aggregation Counter */
			if(TxAggCtrl->TxCycleCounter > 0)
			{
				TxAggCtrl->TxCycleCounter--;
			}

			/* If Aggregation counter expired */
			if(TxAggCtrl->TxCycleCounter == 0)
			{
				/* Check if the channel has any messages aggregated */
				if(Channel->ChannelFlags.HasAggregatedMsgs)
				{
					/**
					 * Then, take the first aggregated message as a reference and send that to ComIf_Transmit function with Force transmit flag set to TRUE.
					 * This will trigger the transmission of all of the aggregated messages.
					 * To avoid multiple transmission of same message, clear the Pending transmission flag of the referenced message.
					 **/
					UB MsgIndex = P_COMIF_INVALID_HANDLE;
					ComIfTxMessageConfig * TxMsgCfg = NULL;

					for(UBYTE i = 0; i < Channel->TotalTxMessages; i++)
					{
						TxMsgCfg = &(Channel->TxMessages[i]);

						if(TxMsgCfg->TxFlags.PendingForTransmission == TRUE)
						{
							MsgIndex = i;
							break;
						}
					}

					// If the first aggregated message is got, then set the flags and trigger the transmission
					if((MsgIndex != P_COMIF_INVALID_HANDLE) && (TxMsgCfg != NULL))
					{
						UBYTE retval = COMIF_EC_GENERIC_ERROR;
						ULONG DummyChecksum = 0;
						UWORD TentativeDataLength = 0;

						// FIXME: Is lock is needed?

						// Clear the Pending for transmission flag
						TxMsgCfg->TxFlags.PendingForTransmission = FALSE;

						// Set the Force Transmit flag
						TxMsgCfg->TxFlags.ForceTransmit = TRUE;

						// Get the tentative data length for this message
						TentativeDataLength = ComIf_GetChecksumAndTentativeDataLength(ChannelIndex, MsgIndex, TxMsgCfg->Length, &DummyChecksum);

						// And reduce the tentative data length from the current buffer length. This is to avoid empty or invalid data being transmitted in the channel
						// This will also ensure that all the messages were transmitted and the flags will be cleared appropriately
						TxAggCtrl->CurrentBufferLength -= TentativeDataLength;

						// Call the transmit function with it's properties
						retval = ComIf_Transmit(ChannelIndex, MsgIndex, TxMsgCfg->ShadowBuffer, TxMsgCfg->Length);

						// If there are any errors in the transmission, then attempt retrying in the next main function cycle
						if(retval != COMIF_EC_NO_ERROR)
						{
							// Set the Pending for transmission to TRUE to invoke this message in next cycle
							TxMsgCfg->TxFlags.PendingForTransmission = TRUE;

							// Clear the Force Transmit flag
							TxMsgCfg->TxFlags.ForceTransmit = FALSE;

							// Add the Tentative Data Length to the current buffer length
							TxAggCtrl->CurrentBufferLength += TentativeDataLength;
						}
					}
				}
			}
		}
	}
#endif
}
