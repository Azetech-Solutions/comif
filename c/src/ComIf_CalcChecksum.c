/*
 * ComIf_CalcChecksum.c
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

ULONG ComIf_CalcChecksum(UBYTE * Data, UBYTE Length)
{
	ULONG Checksum = 0;

	UBYTE i = 0;

	ComIf_Lock_Checksum();
	for(i = 0; i < Length; i++)
	{
		Checksum += Data[i];
	}
	ComIf_UnLock_Checksum();

	Checksum = ((~Checksum) + 1);

	return Checksum;
}


ULONG ComIf_GetChecksumMaskValue(UBYTE ChecksumLength)
{
	ULONG Mask = 0xFF; // Default 1 Byte Checksum Mask

	if(ChecksumLength > COMIF_MAX_SUPPORTED_CHKLEN)
	{
		ChecksumLength = COMIF_MAX_SUPPORTED_CHKLEN;
	}

    while(--ChecksumLength)
    {
        Mask <<= 8;
        Mask |= 0xFF;
    }
    
	return Mask;
}
