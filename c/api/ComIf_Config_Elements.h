/*
 * ComIf_Config_Elements.h
 */

#ifndef __COMIF_CONFIG_ELEMENTS_H__
#define __COMIF_CONFIG_ELEMENTS_H__

/* Input Method = Interrupt means the Com will directly receive the Interrupt calls */
#define COMIF_INPUT_METHOD_INTERRUPT            0

/* Input Method = Polling means the Com have to poll the value in the main function */
#define COMIF_INPUT_METHOD_POLLING              1

/* Input Method = Accumulated means multiple data bytes will be available while polling */
#define COMIF_INPUT_METHOD_ACCUMULATED          2

/**----------------------------------------------
 * ComIf Data Transfer Methods
 *
 * If it is configured as BYTES, then the data will simply transmitted as Bytes and Rx Indication will simply copy the data.
 * If it is configured as STRING, then the data will be converted into string before transmission and decoded back during reception.
 **----------------------------------------------*/
#define COMIF_DATA_TXFR_METHOD_BYTES       0u
#define COMIF_DATA_TXFR_METHOD_STRING      1u

/**----------------------------------------------
 * ComIf Error Codes
 **----------------------------------------------*/

#define COMIF_EC_NO_ERROR               0u

#define COMIF_EC_CHANNEL_BUSY           1u
#define COMIF_EC_REQUEST_TIMEOUT        2u
#define COMIF_EC_CHANNEL_NOT_AVAILABLE  3u

#define COMIF_EC_FORM_ERROR             11u
#define COMIF_EC_DELIMITER_ERROR        12u
#define COMIF_EC_INVALID_ID             13u
#define COMIF_EC_INVALID_DLC            14u
#define COMIF_EC_INVALID_CHK            15u
#define COMIF_EC_INVALID_MSG            16u
#define COMIF_EC_INVALID_CHANNEL        17u
#define COMIF_EC_BUFFER_OVERFLOW        18u
#define COMIF_EC_TRANSMISSION_ABORTED   19u

#define COMIF_EC_GENERIC_ERROR          20u


#endif /* __COMIF_CONFIG_ELEMENTS_H__ */
