const STX = 0x7B;
const DLE = 0x7C;
const ETX = 0x7D;

export const NumberChannel = 0;
export const StringChannel = 1;

export const RET_OK = 0;
export const RET_NOK = 1;

export const COMIF_EC_NO_ERROR               = (0); // Not Used
export const COMIF_EC_CHANNEL_BUSY           = (1); // Not Used
export const COMIF_EC_REQUEST_TIMEOUT        = (2); // Not Used
export const COMIF_EC_CHANNEL_NOT_AVAILABLE  = (3); // Not Used
export const COMIF_EC_FORM_ERROR             = (11); // Not Used
export const COMIF_EC_DELIMITER_ERROR        = (12); // Not Used
export const COMIF_EC_INVALID_ID             = (13);
export const COMIF_EC_INVALID_DLC            = (14);
export const COMIF_EC_INVALID_CHK            = (15);
export const COMIF_EC_INVALID_MSG            = (16);
export const COMIF_EC_INVALID_CHANNEL        = (17); // Not Used
export const COMIF_EC_BUFFER_OVERFLOW        = (18); // Not Used
export const COMIF_EC_TRANSMISSION_ABORTED   = (19); // Not Used
export const COMIF_EC_GENERIC_ERROR          = (20); // Not Used

function NeedDelimiter(Data)
{
    var retval = false;

    if(    (Data == STX)
        || (Data == DLE)
        || (Data == ETX)
        )
    {
        retval = true;
    }

    return retval;
}

function GetDebugWords(a, b)
{
    var retval = 0;

    retval |= (b & 0xFF);
    retval |= ((a & 0xFF) << 16);

    return retval;
}

function GetDebugBytes(a, b, c, d)
{
    var retval = 0;

    retval |= (d & 0xFF);
    retval |= ((c & 0xFF) << 8);
    retval |= ((b & 0xFF) << 16);
    retval |= ((a & 0xFF) << 24);

    return retval;
}

export class TxMessage
{
    /* Basic Attributes */
    ID = 0;
    Length = 0;
    Data = new Uint8Array(256); // 1 byte including Checksum

    constructor(id, length)
    {
        this.ID = id;
        this.Length = length;
    }
}

export class RxMessage
{
    /* Basic Attributes */
    ID = 0;
    Length = 0;
    Data = new Uint8Array(256); // 1 byte including Checksum

    EnableDynamicLength = false;

    /* Status Flags */
    ReceptionStarted = false; // If set, then the message has been started receiving
    NewMessageReceived = false; // If set, then the message has been received completely and waiting for the RxCbk to be called
    ErrorInReception = false; // If set, then the message has been received, but there is an error in reception
    CurRxngIdx = 0;

    RxCbk = null;

    constructor(id, length, rxCallback)
    {
        this.ID = id;
        this.Length = length;
        this.RxCbk = rxCallback;
    }
}

export class Channel
{
    Name = "";
    DataTxfrType = NumberChannel;
    TriggerTransmit = null;
    NotifyError = null;
    TxMessages = [];
    RxMessages = [];

    /* Internal Flags */
    #Delimit = false;
    #IsReceiving = false;
    #DLCVerified = false;
    static INVALID_INDEX = 255;
    #RxMsgIndex = Channel.INVALID_INDEX;
    #RxMsgLength = 0;

    constructor(channelName, channelType, transmitFunction, errorNotification)
    {
        this.Name = channelName;
        this.DataTxfrType = channelType;
        this.TriggerTransmit = transmitFunction;
        this.NotifyError = errorNotification;
    }

    RegisterRxMessage(rxMessage)
    {
        if (rxMessage != null)
        {
            for (var msg in this.RxMessages)
            {
                if (msg.ID == rxMessage.ID)
                {
                    return RET_NOK;
                }
            }

            this.RxMessages.push(rxMessage);

            return RET_OK;
        }

        return RET_NOK;
    }

    Transmit(txMessage)
    {
        if ((txMessage == null) || (this.TriggerTransmit == null))
        {
            return RET_NOK;
        }

        var data = new Uint8Array(256);
        var Checksum = 0;
        var FrameLength = 0;

        data[FrameLength] = STX;
        FrameLength++;

        data[FrameLength] = txMessage.ID;
        FrameLength++;

        data[FrameLength] = txMessage.Length;
        FrameLength++;

        for (var i = 0; i < txMessage.Length; i++)
        {
            var myByte = txMessage.Data[i];
            if (NeedDelimiter(myByte))
            {
                data[FrameLength] = DLE;
                FrameLength++;
            }

            data[FrameLength] = myByte;
            FrameLength++;

            Checksum += myByte;
        }

        var ChkByte = (((~Checksum) + 1) & 0xFF);

        if (NeedDelimiter(ChkByte))
        {
            data[FrameLength] = DLE;
            FrameLength++;
        }

        data[FrameLength] = ChkByte;
        FrameLength++;

        data[FrameLength] = ETX;
        FrameLength++;

        if (this.DataTxfrType == StringChannel)
        {
            var str = [];
            var index = 0;

            for(var i = 0; i < FrameLength; i++)
            {
                str[index++] = ((((data[i] & 0xF0) >> 4) + 0x30));
                str[index++] = (((data[i] & 0x0F) + 0x30));
            }

            // For String Channel, send the data as string
            this.TriggerTransmit(index, new Buffer.from(str).toString());
        }
        else // if (this.DataTxfrType == NumberChannel)
        {
            // For Number Channel, send the data as UInt8 array
            this.TriggerTransmit(FrameLength, data);
        }

        return RET_OK;
    }

    ResetRxInfo(IsError)
    {
        this.#IsReceiving = false;
        this.#DLCVerified = false;

        if(this.#RxMsgIndex != Channel.INVALID_INDEX)
        {
            let currentRxMsg = this.RxMessages[this.#RxMsgIndex];

            currentRxMsg.CurRxngIdx = 0;
            currentRxMsg.ErrorInReception = IsError;
            currentRxMsg.NewMessageReceived = !IsError;
            currentRxMsg.ReceptionStarted = false;
        }

        this.#RxMsgIndex = Channel.INVALID_INDEX;
        this.#RxMsgLength = 0;
    }

    StoreDataByte(DataByte)
    {
        var retval = RET_OK;

        let RxMsg = this.RxMessages[this.#RxMsgIndex];

        if ((RxMsg.CurRxngIdx) == (this.#RxMsgLength + 1 /* ChecksumLength */))
        {
            // If the Maximum Message Buffer Length reached, then do not process further and report error
            retval = RET_NOK;

            /* Report Error */
            this.NotifyError(COMIF_EC_INVALID_MSG, GetDebugWords(RxMsg.CurRxngIdx, DataByte));

            // Reset the Reception information
            this.ResetRxInfo(true);
        }
        else
        {
            RxMsg.Data[RxMsg.CurRxngIdx] = DataByte;
            RxMsg.CurRxngIdx++;
        }

        return retval;
    }

    RxIndication(DataByte)
    {
        var retval = RET_OK;

        /* Check for Commands */
        if (this.#Delimit == false)
        {
            if (DataByte == STX)
            {
                if((this.#IsReceiving == true) && (this.#RxMsgIndex != Channel.INVALID_INDEX))
                {
                    this.NotifyError(COMIF_EC_INVALID_MSG, GetDebugWords(STX, this.#RxMsgIndex));

                    this.ResetRxInfo(true);
                }
                else
                {
                    this.#IsReceiving = true;
                }

                /* Wait for the ID to be received */
                this.#RxMsgIndex = Channel.INVALID_INDEX;
            }
            else if (this.#IsReceiving == true)
            {
                // Only Delimit the data bytes and Checksum, ID and DLC are not part of the de-limitation
                if ((DataByte == DLE) && (this.#DLCVerified == true))
                {
                    this.#Delimit = true;
                }
                else if (DataByte == ETX)
                {
                    // If End of Transmission is received, then calculate checksum and give Rx Indication
                    let RxMsg = this.RxMessages[this.#RxMsgIndex];
                    var DataLength = this.#RxMsgLength;
                    var ChecksumLength = 1;

                    // Calculate Checksum if all the bytes were received
                    // If the receive checksum is ignored, but the transmitted still sends the CHK, then the CurRxngIdx will be greater
                    // Even though we receive more data, we only should pass the actual data length to the user

                    if ((RxMsg.CurRxngIdx) == (DataLength + ChecksumLength))
                    {
                        var ReceivedChecksum = RxMsg.Data[DataLength + 0]; // Last index is the checksum index

                        var CalcChecksum = 0;

                        for (var i = 0; i < DataLength; i++) // Exclude the checksum byte
                        {
                            CalcChecksum += RxMsg.Data[i];
                        }

                        CalcChecksum = (((~CalcChecksum) + 1) & 0xFF);

                        if (ReceivedChecksum == CalcChecksum)
                        {
                            /* A Valid  Message is being received */

                            /* Send RxCbk */
                            if(RxMsg.RxCbk != null)
                            {
                                RxMsg.RxCbk(DataLength, RxMsg.Data);
                            }

                            /* Reset the Rx Info as No Error */
                            this.ResetRxInfo(false);
                        }
                        else
                        {
                            /* Reset the Rx Info as Error */
                            this.ResetRxInfo(true);

                            /* Report Error */
                            this.NotifyError(COMIF_EC_INVALID_CHK, GetDebugBytes(RxMsg.ID, 0, ReceivedChecksum, CalcChecksum));
                        }
                    }
                    else
                    {
                        /* If the received information is less than the configured one, then possibly the data might be lost */

                        /* Report Error */
                        this.NotifyError(COMIF_EC_INVALID_MSG, GetDebugBytes(ETX, RxMsg.CurRxngIdx, (DataLength & 0xFF), ChecksumLength));

                        /* Reset the Rx Info as Error */
                        this.ResetRxInfo(true);
                    }
                }
                else
                {
                    /* If the currentRxMsg is null, then the reception is waiting for the ID */
                    if (this.#RxMsgIndex == Channel.INVALID_INDEX)
                    {
                        // This is the ID Byte
                        for(var i = 0; i < this.RxMessages.length; i++)
                        {
                            if(this.RxMessages[i].ID == DataByte)
                            {
                                this.#RxMsgIndex = i;
                                break;
                            }
                        }

                        /* If we can't able to identify the Message in the Handle */
                        if (this.#RxMsgIndex == Channel.INVALID_INDEX)
                        {
                            /* Then, reset the reception interfaces */
                            this.ResetRxInfo(true);

                            /* Report Error */
                            this.NotifyError(COMIF_EC_INVALID_ID, DataByte);
                        }
                    }
                    else
                    {
                        // ID is available
                        if (this.#DLCVerified == false)
                        {
                            // If the DLC is not verified, then this byte could be the DLC byte
                            let rxMessage = this.RxMessages[this.#RxMsgIndex];

                            if((rxMessage.EnableDynamicLength == true) || (rxMessage.Length == DataByte))
                            {
                                this.#DLCVerified = true;
                                this.#RxMsgLength = DataByte;
                            }
                            else
                            {
                                // If the received DLC does not match with the configured DLC, then report error and stop reception
                                this.ResetRxInfo(true);

                                /* Report Error */
                                this.NotifyError(COMIF_EC_INVALID_DLC, GetDebugBytes(rxMessage.ID, rxMessage.Length, (rxMessage.EnableDynamicLength ? 1 : 0), DataByte));
                            }
                        }
                        else
                        {
                            // If ID and DLC verified, then the receiving bytes are the data bytes only
                            // Store the Data
                            this.StoreDataByte(DataByte);
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
            if (this.#RxMsgIndex != Channel.INVALID_INDEX)
            {
                // Store the Data
                this.StoreDataByte(DataByte);
            }

            /* Once stored, clear the delimit flag */
            this.#Delimit = false;
        }

        return retval;
    }

    RxIndication_Bytes(DataBytes)
    {
        for(var i = 0; i < DataBytes.length; i++)
        {
            this.RxIndication(DataBytes[i]);
        }

        return RET_OK;
    }

    RxIndication_String(DataString)
    {
        // it is assumed that the rx indication shall be in the form of ASCII string
        var Length = DataString.length / 2;
        var uintArray = new Uint8Array(Length); // Convert into a uint8 array

        for(var i = 0, index = 0; i < DataString.length; i++, index++)
        {
            var val = ((DataString.charCodeAt(i) - 0x30) << 4);
            val |= (DataString.charCodeAt([i + 1]) - 0x30);
            i++;
            uintArray[index] = val;
        }

        return this.RxIndication_Bytes(uintArray);
    }
}
