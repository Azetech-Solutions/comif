/*
 * ComIF.h
 *
 *  Created on: Apr 15, 2018
 *      Author: Surendar
 */

#ifndef __COMIF_H__
#define __COMIF_H__

/**********************************************************/
/* Header Inclusions                                      */
/**********************************************************/

#include "Platform_Types.h"
#include "ComIf_Config_Elements.h"
#include "ComIf_Config.h"

/**********************************************************/
/* Macro Definitions                                      */
/**********************************************************/

#define COMIF_STX                    0x7B  // Start of Text
#define COMIF_DLE                    0x7C  // Data Link Escape
#define COMIF_ETX                    0x7D  // End of Text

#define COMIF_ACK                    1
#define COMIF_NAK                    2

/*--------------------------------------------------------*/
/*               Critical Section Macros                  */
/*--------------------------------------------------------*/

#define COMIF_ENTER_CRITICAL_SECTION()      // TODO: Semaphore/Mutex or Interrupt Locks
#define COMIF_EXIT_CRITICAL_SECTION()       // TODO: Semaphore/Mutex or Interrupt Locks


/**
 * TxShadowBuffer Locks:
 * ---------------------
 * Enable these locks when the ComIf_Transmit or ComIf_UpdateTxShadowBuffer for the same Message in the same channel
 * has a potential to run in parallel, or possible to pre-empt each other or via an ISR, as per the core and task scheduling.
 *
 * These locks are not needed if all the messages in all the channels has been scheduled to call these functions in
 * a single task context.
 **/
#define ComIf_Lock_TxShadowBuffer()
#define ComIf_UnLock_TxShadowBuffer()

/**
 * Checksum Locks:
 * ---------------
 * Enable these locks when the CalcChecksum function call is not surrounded by the respective buffer locks
 * If at-least one instance has the lock surrounded, then this lock should not be enabled.
 * All other instance of the calls must be surrounded by the respective locks
 **/
#define ComIf_Lock_Checksum()
#define ComIf_UnLock_Checksum()

/**
 * Transmit Locks:
 * ---------------------
 * Enable these locks when the ComIf_Transmit has a chance to get called for the same Channel re-entrantly.
 *
 * These locks are not needed if all the messages in a channel has been scheduled to call these functions in
 * a single task context.
 **/
#define ComIf_Lock_Transmit()
#define ComIf_UnLock_Transmit()


/* Debug Data Configuration */
#define COMIF_DEBUG_DATA_BYTES(a, b, c, d)     ( (((UL)a) << 24) | (((UL)b) << 16) | (((UL)c) << 8) | d)
#define COMIF_DEBUG_DATA_WORDS(a, b)           ( (((UL)a) << 16) | b)

/**********************************************************/
/* Type Definitions                                       */
/**********************************************************/

/**********************************************************/
/* Global Variable Declarations                           */
/**********************************************************/

/**********************************************************/
/* Inline Function Definitions                            */
/**********************************************************/

/**********************************************************/
/* Function Declaration                                   */
/**********************************************************/

extern UBYTE ComIf_Transmit(UBYTE ChannelIndex, UBYTE MsgIndex, void * DataPtr, UBYTE Length);
extern UBYTE ComIf_EncodeFromShadowToChannelBuffer(UBYTE ChannelIndex, UBYTE MsgIndex, UBYTE Length, UWORD * OutLength, UBYTE * OutBuffer, UWORD BoundLimit);
extern UBYTE ComIf_RxIndication(UBYTE ChannelIndex, UBYTE DataByte);
extern UBYTE ComIf_RxIndication_ASCIIString(UBYTE Channel, char * Message, UWORD Length);
extern ULONG ComIf_CalcChecksum(UBYTE * Data, UBYTE Length);
extern ULONG ComIf_GetChecksumMaskValue(UBYTE ChecksumLength);
extern void  ComIf_TxConfirmation(UBYTE ChannelIndex);
extern void ComIf_TransmissionAborted(UBYTE ChannelIndex);
extern UBYTE * ComIf_GetTxShadowBuffer(UBYTE ChannelIndex, UBYTE MsgIndex);
extern UBYTE ComIf_UpdateTxShadowBuffer(UBYTE ChannelIndex, UBYTE MsgIndex, void * DataPtr, UBYTE Length);
extern UBYTE ComIf_Transmit_ShadowBuffer(UBYTE ChannelIndex, UBYTE MsgIndex, UBYTE Length);
extern UBYTE ComIf_GetTxMsgHandle(ComIf_ChannelType Channel, UBYTE ID);
extern UBYTE ComIf_GetRxMsgHandle(ComIf_ChannelType Channel, UBYTE ID);
extern void ComIf_ErrorNotify(ComIf_ChannelConfigType * Channel, ULONG Debug0, ULONG Debug1);
extern void ComIf_TxMainFunction(void);

/**********************************************************/
/* Sub Module and Configuration Headers Inclusions        */
/**********************************************************/

#include "ComIf_Config.h"

#endif /* __COMIF_H__ */
