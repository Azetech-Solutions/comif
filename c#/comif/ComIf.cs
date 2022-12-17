using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ComIf
{
    public enum Delimiters
    {
        STX = 0x7B,  // Start of Text
        DLE = 0x7C,  // Data Link Escape
        ETX = 0x7D  // End of Text
    }

    public enum ChannelType
    {
        Number = 0,
        String = 1
    }

    public enum ReturnValue
    {
        OK = 0,
        NOK = 1,
    }

    public enum ErrorCodes
    {
        COMIF_EC_NO_ERROR               = 0, // Not Used
        COMIF_EC_CHANNEL_BUSY           = 1, // Not Used
        COMIF_EC_REQUEST_TIMEOUT        = 2, // Not Used
        COMIF_EC_CHANNEL_NOT_AVAILABLE  = 3, // Not Used
        COMIF_EC_FORM_ERROR             = 11, // Not Used
        COMIF_EC_DELIMITER_ERROR        = 12, // Not Used
        COMIF_EC_INVALID_ID             = 13,
        COMIF_EC_INVALID_DLC            = 14,
        COMIF_EC_INVALID_CHK            = 15,
        COMIF_EC_INVALID_MSG            = 16,
        COMIF_EC_INVALID_CHANNEL        = 17, // Not Used
        COMIF_EC_BUFFER_OVERFLOW        = 18, // Not Used
        COMIF_EC_TRANSMISSION_ABORTED   = 19, // Not Used
        COMIF_EC_GENERIC_ERROR          = 20 // Not Used
    }

    public static class CommonAPIs
    {
        public static bool NeedDelimiter(byte Data)
        {
            bool retval = false;

            if(    (Data == (byte)Delimiters.STX)
                || (Data == (byte)Delimiters.DLE)
                || (Data == (byte)Delimiters.ETX)
              )
            {
                retval = true;
            }

            return retval;
        }

        public static UInt32 GetDebugWords(byte a, byte b)
        {
            UInt32 retval = b;

            retval |= (((UInt32)a) << 16);

            return retval;
        }

        public static UInt32 GetDebugBytes(byte a, byte b, byte c, byte d)
        {
            UInt32 retval = d;

            retval |= (((UInt32)c) << 8);
            retval |= (((UInt32)b) << 16);
            retval |= (((UInt32)a) << 24);

            return retval;
        }
    }

    public class TxMessage
    {
        /* Basic Attributes */
        public byte ID = 0;
        public byte Length = 0;
        public byte[] Data = new byte[256]; // 1 byte including Checksum

        public TxMessage(byte id, byte length)
        {
            ID = id;
            Length = length;
        }
    }

    public class RxMessage
    {
        /* Basic Attributes */
        public byte ID = 0;
        public byte Length = 0;
        public byte[] Data = new byte[256]; // 1 byte including Checksum

        public bool EnableDynamicLength = false;

        /* Status Flags */
        public bool ReceptionStarted = false; // If set, then the message has been started receiving
        public bool NewMessageReceived = false; // If set, then the message has been received completely and waiting for the RxCbk to be called
        public bool ErrorInReception = false; // If set, then the message has been received, but there is an error in reception
        public byte CurRxngIdx = 0;

        /* Callbacks */
        public delegate void RxCallback(byte Length, byte[] Data);
        public RxCallback RxCbk = null;

        public RxMessage(byte id, byte length, RxCallback rxCallback)
        {
            ID = id;
            Length = length;
            RxCbk = rxCallback;
        }
    }

    public class Channel
    {
        public string Name = "";

        public ChannelType DataTxfrType = ChannelType.Number;

        public delegate void TransmitFunction(UInt16 Length, byte[] Data);
        private readonly TransmitFunction TriggerTransmit = null;

        public delegate void ErrorNotification(UInt32 Debug0, UInt32 Debug1);
        private readonly ErrorNotification NotifyError = null;
        
        public List<TxMessage> TxMessages = new List<TxMessage>();
        public List<RxMessage> RxMessages = new List<RxMessage>();

        /* Internal Flags */
        private bool Delimit = false;
        private bool IsReceiving = false;
        private bool DLCVerified = false;
        private const int INVALID_INDEX = 255;
        private int RxMsgIndex = INVALID_INDEX;
        private int RxMsgLength = 0;

        public Channel(string channelName, ChannelType channelType, TransmitFunction transmitFunction, ErrorNotification errorNotification)
        {
            Name = channelName;
            DataTxfrType = channelType;
            TriggerTransmit = transmitFunction;
            NotifyError = errorNotification;
        }

        public ReturnValue RegisterRxMessage(RxMessage rxMessage)
        {
            if (rxMessage != null)
            {
                foreach (RxMessage msg in RxMessages)
                {
                    if (msg.ID == rxMessage.ID)
                    {
                        return ReturnValue.NOK;
                    }
                }

                RxMessages.Add(rxMessage);

                return ReturnValue.OK;
            }

            return ReturnValue.NOK;
        }

        public ReturnValue Transmit(TxMessage txMessage)
        {
            if ((txMessage == null) || (TriggerTransmit == null))
            {
                return ReturnValue.NOK;
            }

            List<byte> data = new List<byte>();
            UInt16 Checksum = 0;
            UInt16 FrameLength = 0;

            data.Add((byte)Delimiters.STX);
            FrameLength++;

            data.Add(txMessage.ID);
            FrameLength++;

            data.Add(txMessage.Length);
            FrameLength++;

            for (int i = 0; i < txMessage.Length; i++)
            {
                byte myByte = txMessage.Data[i];
                if (CommonAPIs.NeedDelimiter(myByte))
                {
                    data.Add((byte)Delimiters.DLE);
                    FrameLength++;
                }

                data.Add(myByte);
                FrameLength++;

                Checksum += myByte;
            }

            byte ChkByte = (byte)(((~Checksum) + 1) & 0xFF);

            if (CommonAPIs.NeedDelimiter(ChkByte))
            {
                data.Add((byte)Delimiters.DLE);
                FrameLength++;
            }

            data.Add(ChkByte);
            FrameLength++;

            data.Add((byte)Delimiters.ETX);
            FrameLength++;

            if (DataTxfrType == ChannelType.String)
            {
                List<byte> str = new List<byte>();

                foreach(byte b in data)
                {
                    str.Add((byte)(((b & 0xF0) >> 4) + 0x30));
                    str.Add((byte)((b & 0x0F) + 0x30));
                }

                FrameLength *= 2;

                TriggerTransmit(FrameLength, str.ToArray());
            }
            else // if (DataTxfrType == ChannelType.Number)
            {
                TriggerTransmit(FrameLength, data.ToArray());
            }

            return ReturnValue.OK;
        }

        private void ResetRxInfo(bool IsError)
        {
            IsReceiving = false;
            DLCVerified = false;

            if(RxMsgIndex != INVALID_INDEX)
            {
                RxMessage currentRxMsg = RxMessages[RxMsgIndex];

                currentRxMsg.CurRxngIdx = 0;
                currentRxMsg.ErrorInReception = IsError;
                currentRxMsg.NewMessageReceived = !IsError;
                currentRxMsg.ReceptionStarted = false;
            }

            RxMsgIndex = INVALID_INDEX;
            RxMsgLength = 0;
        }

        public ReturnValue StoreDataByte(byte DataByte)
        {
            ReturnValue retval = ReturnValue.OK;

            RxMessage RxMsg = RxMessages[RxMsgIndex];

            if ((RxMsg.CurRxngIdx) == (RxMsgLength + 1 /* ChecksumLength */))
            {
                // If the Maximum Message Buffer Length reached, then do not process further and report error
                retval = ReturnValue.NOK;

                /* Report Error */
                NotifyError((uint)ErrorCodes.COMIF_EC_INVALID_MSG, CommonAPIs.GetDebugWords(RxMsg.CurRxngIdx, DataByte));

                // Reset the Reception information
                ResetRxInfo(true);
            }
            else
            {
                RxMsg.Data[RxMsg.CurRxngIdx] = DataByte;
                RxMsg.CurRxngIdx++;
            }

            return retval;
        }


        public ReturnValue RxIndication(byte DataByte)
        {
            ReturnValue retval = ReturnValue.OK;

            /* Check for Commands */
            if (Delimit == false)
            {
                if (DataByte == (byte)Delimiters.STX)
                {
                    if((IsReceiving == true) && (RxMsgIndex != INVALID_INDEX))
                    {
                        NotifyError((uint)ErrorCodes.COMIF_EC_INVALID_MSG, CommonAPIs.GetDebugWords((byte)Delimiters.STX, (byte)RxMsgIndex));

                        ResetRxInfo(true);
                    }
                    else
                    {
                        IsReceiving = true;
                    }

                    /* Wait for the ID to be received */
                    RxMsgIndex = INVALID_INDEX;
                }
                else if (IsReceiving == true)
                {
                    // Only Delimit the data bytes and Checksum, ID and DLC are not part of the de-limitation
                    if ((DataByte == (byte)Delimiters.DLE) && (DLCVerified == true))
                    {
                        Delimit = true;
                    }
                    else if (DataByte == (byte)Delimiters.ETX)
                    {
                        // If End of Transmission is received, then calculate checksum and give Rx Indication
                        RxMessage RxMsg = RxMessages[RxMsgIndex];
                        int DataLength = RxMsgLength;
                        int ChecksumLength = 1;

                        // Calculate Checksum if all the bytes were received
                        // If the receive checksum is ignored, but the transmitted still sends the CHK, then the CurRxngIdx will be greater
                        // Even though we receive more data, we only should pass the actual data length to the user

                        if ((RxMsg.CurRxngIdx) == (DataLength + ChecksumLength))
                        {
                            byte ReceivedChecksum = RxMsg.Data[DataLength + 0]; // Last index is the checksum index

                            UInt16 CalcChecksum = 0;

                            for (int i = 0; i < DataLength; i++) // Exclude the checksum byte
                            {
                                CalcChecksum += RxMsg.Data[i];
                            }

                            CalcChecksum = (byte)(((~CalcChecksum) + 1) & 0xFF);

                            if (ReceivedChecksum == CalcChecksum)
                            {
                                /* A Valid  Message is being received */

                                /* Send RxCbk */
                                RxMsg.RxCbk?.Invoke((byte)DataLength, RxMsg.Data);

                                /* Reset the Rx Info as No Error */
                                ResetRxInfo(false);
                            }
                            else
                            {
                                /* Reset the Rx Info as Error */
                                ResetRxInfo(true);

                                /* Report Error */
                                NotifyError((byte)ErrorCodes.COMIF_EC_INVALID_CHK, CommonAPIs.GetDebugBytes(RxMsg.ID, 0, ReceivedChecksum, (byte)CalcChecksum));
                            }
                        }
                        else
                        {
                            /* If the received information is less than the configured one, then possibly the data might be lost */

                            /* Report Error */
                            NotifyError((byte)ErrorCodes.COMIF_EC_INVALID_MSG, CommonAPIs.GetDebugBytes((byte)Delimiters.ETX, RxMsg.CurRxngIdx, (byte)DataLength, (byte)ChecksumLength));

                            /* Reset the Rx Info as Error */
                            ResetRxInfo(true);
                        }
                    }
                    else
                    {
                        /* If the currentRxMsg is null, then the reception is waiting for the ID */
                        if (RxMsgIndex == INVALID_INDEX)
                        {
                            // This is the ID Byte
                            for(int i = 0; i < RxMessages.Count; i++)
                            {
                                if(RxMessages[i].ID == DataByte)
                                {
                                    RxMsgIndex = i;
                                    break;
                                }
                            }

                            /* If we can't able to identify the Message in the Handle */
                            if (RxMsgIndex == INVALID_INDEX)
                            {
                                /* Then, reset the reception interfaces */
                                ResetRxInfo(true);

                                /* Report Error */
                                NotifyError((byte)ErrorCodes.COMIF_EC_INVALID_ID, DataByte);
                            }
                        }
                        else
                        {
                            // ID is available
                            if (DLCVerified == false)
                            {
                                // If the DLC is not verified, then this byte could be the DLC byte
                                RxMessage rxMessage = RxMessages[RxMsgIndex];

                                if((rxMessage.EnableDynamicLength == true) || (rxMessage.Length == DataByte))
                                {
                                    DLCVerified = true;
                                    RxMsgLength = DataByte;
                                }
                                else
                                {
                                    // If the received DLC does not match with the configured DLC, then report error and stop reception
                                    ResetRxInfo(true);

                                    /* Report Error */
                                    NotifyError((byte)ErrorCodes.COMIF_EC_INVALID_DLC, CommonAPIs.GetDebugBytes(rxMessage.ID, rxMessage.Length, (byte)(rxMessage.EnableDynamicLength ? 1 : 0), DataByte));
                                }
                            }
                            else
                            {
                                // If ID and DLC verified, then the receiving bytes are the data bytes only
                                // Store the Data
                                StoreDataByte(DataByte);
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
                if (RxMsgIndex != INVALID_INDEX)
                {
                    // Store the Data
                    StoreDataByte(DataByte);
                }

                /* Once stored, clear the delimit flag */
                Delimit = false;
            }

            return retval;
        }

        public ReturnValue RxIndication(byte[] DataBytes)
        {
            ReturnValue retval = ReturnValue.OK;

            for(int i = 0; i < DataBytes.Length; i++)
            {
                retval |= RxIndication(DataBytes[i]);
            }

            return retval;
        }

        public ReturnValue RxIndication(string DataString)
        {
            ReturnValue retval = ReturnValue.OK;

            char[] arr = DataString.ToCharArray();

            int Length = arr.Length;
            int i = 0;

            if ((Length % 2) == 0)
            {
                while (Length > 0)
                {
                    byte Data = 0;

                    Data = (byte)((((byte)arr[i] - 0x30) << 4) | ((byte)arr[i + 1] - 0x30));

                    retval |= RxIndication(Data);

                    i += 2; // Move to next step
                    Length -= 2;

                    /* If the string has an odd number length, then send the last byte also and exit */
                    /* This should not actually happen, but still sent to record the exact error instead of ignoring */
                    if (arr[ i + 1] == '\0')
                    {
                        Data = (byte)((arr[i] - 0x30) << 4);
                        retval |= RxIndication(Data);
                        break;
                    }
                }
            }
            else
            {
                // If the length is not an even number, then this is not formed properly.
                retval = ReturnValue.NOK;
            }

            return retval;
        }
    }
}
