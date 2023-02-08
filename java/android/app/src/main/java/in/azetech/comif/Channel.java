package in.azetech.comif;

import java.util.ArrayList;
import java.util.List;

public class Channel
{
    public class Delimiters
    {
        public static final byte STX = ((byte)0x7B);  // Start of Text
        public static final byte DLE = ((byte)0x7C);  // Data Link Escape
        public static final byte ETX = ((byte)0x7D);  // End of Text
    }

    public enum ChannelType
    {
        Number,
        String
    }

    public enum ReturnValue
    {
        OK,
        NOK
    }

    public class ErrorCodes
    {
        public static final byte COMIF_EC_NO_ERROR               = (0); // Not Used
        public static final byte COMIF_EC_CHANNEL_BUSY           = (1); // Not Used
        public static final byte COMIF_EC_REQUEST_TIMEOUT        = (2); // Not Used
        public static final byte COMIF_EC_CHANNEL_NOT_AVAILABLE  = (3); // Not Used
        public static final byte COMIF_EC_FORM_ERROR             = (11); // Not Used
        public static final byte COMIF_EC_DELIMITER_ERROR        = (12); // Not Used
        public static final byte COMIF_EC_INVALID_ID             = (13);
        public static final byte COMIF_EC_INVALID_DLC            = (14);
        public static final byte COMIF_EC_INVALID_CHK            = (15);
        public static final byte COMIF_EC_INVALID_MSG            = (16);
        public static final byte COMIF_EC_INVALID_CHANNEL        = (17); // Not Used
        public static final byte COMIF_EC_BUFFER_OVERFLOW        = (18); // Not Used
        public static final byte COMIF_EC_TRANSMISSION_ABORTED   = (19); // Not Used
        public static final byte COMIF_EC_GENERIC_ERROR          = (20); // Not Used
    }

    public static class CommonAPIs
    {
        public static boolean NeedDelimiter(byte Data)
        {
            boolean retval = false;

            if(    (Data == (byte)Delimiters.STX)
                || (Data == (byte)Delimiters.DLE)
                || (Data == (byte)Delimiters.ETX)
                )
            {
                retval = true;
            }

            return retval;
        }

        public static int GetDebugWords(byte a, byte b)
        {
            int retval = 0;

            retval |= (b & 0xFF);
            retval |= ((a & 0xFF) << 16);

            return retval;
        }

        public static int GetDebugBytes(byte a, byte b, byte c, byte d)
        {
            int retval = 0;

            retval |= (d & 0xFF);
            retval |= ((c & 0xFF) << 8);
            retval |= ((b & 0xFF) << 16);
            retval |= ((a & 0xFF) << 24);

            return retval;
        }
    }

    /***********************************************************************/
    /***********************************************************************/

    public String Name = "";

    public ChannelType DataTxfrType = ChannelType.Number;

    public interface TransmitFunctionCallback
    {
        public void TransmitFunction(short Length, Byte[] Data);
    }
    private TransmitFunctionCallback TriggerTransmit = null;

    public interface ErrorNotificationListener
    {
        public void ErrorNotification(int Debug0, int Debug1);
    }
    private ErrorNotificationListener NotifyError = null;
    
    public List<TxMessage> TxMessages = new ArrayList<TxMessage>();
    public List<RxMessage> RxMessages = new ArrayList<RxMessage>();

    /* Internal Flags */
    private boolean Delimit = false;
    private boolean IsReceiving = false;
    private boolean DLCVerified = false;
    private static final int INVALID_INDEX = 255;
    private int RxMsgIndex = INVALID_INDEX;
    private int RxMsgLength = 0;

    public Channel(String channelName, ChannelType channelType, TransmitFunctionCallback transmitFunction, ErrorNotificationListener errorNotification)
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
            for (RxMessage msg : RxMessages)
            {
                if (msg.ID == rxMessage.ID)
                {
                    return ReturnValue.NOK;
                }
            }

            RxMessages.add(rxMessage);

            return ReturnValue.OK;
        }

        return ReturnValue.NOK;
    }

    public ReturnValue RegisterTxMessage(TxMessage txMessage)
    {
        if (txMessage != null)
        {
            for (TxMessage msg : TxMessages)
            {
                if (msg.ID == txMessage.ID)
                {
                    return ReturnValue.NOK;
                }
            }

            TxMessages.add(txMessage);

            return ReturnValue.OK;
        }

        return ReturnValue.NOK;
    }

    public TxMessage GetTxMessageInstance(byte ID)
    {
        for (TxMessage msg : TxMessages)
        {
            if (msg.ID == ID)
            {
                return msg;
            }
        }
        
        return null;
    }

    public RxMessage GetRxMessageInstance(byte ID)
    {
        for (RxMessage msg : RxMessages)
        {
            if (msg.ID == ID)
            {
                return msg;
            }
        }

        return null;
    }

    public TxMessage GetTxMessageInstance(String Name)
    {
        for (TxMessage msg : TxMessages)
        {
            if (msg.Name.equals(Name))
            {
                return msg;
            }
        }

        return null;
    }

    public RxMessage GetRxMessageInstance(String Name)
    {
        for (RxMessage msg : RxMessages)
        {
            if (msg.Name.equals(Name))
            {
                return msg;
            }
        }
        
        return null;
    }

    public boolean TriggerTransmitForScheduledMessages()
    {
        boolean IsAtleastOneMessageScheduled = false;

        for (TxMessage msg : TxMessages)
        {
            if(msg.IsTxScheduled) {
                // If Tx Is Scheduled, then trigger the tranmission
                Transmit(msg);
                
                IsAtleastOneMessageScheduled = true;
            }
        }
        
        return IsAtleastOneMessageScheduled;
    }

    public ReturnValue Transmit(TxMessage txMessage)
    {
        if ((txMessage == null) || (TriggerTransmit == null))
        {
            return ReturnValue.NOK;
        }

        List<Byte> data = new ArrayList<Byte>();
        short Checksum = 0;
        short FrameLength = 0;

        // Give a Tx Callback to get the updated data
        txMessage.TxCallback(txMessage);

        data.add((byte)Delimiters.STX);
        FrameLength++;

        data.add(txMessage.ID);
        FrameLength++;

        data.add(txMessage.Length);
        FrameLength++;

        for (int i = 0; i < txMessage.Length; i++)
        {
            byte myByte = txMessage.Data[i];
            if (CommonAPIs.NeedDelimiter(myByte))
            {
                data.add((byte)Delimiters.DLE);
                FrameLength++;
            }

            data.add(myByte);
            FrameLength++;

            Checksum += myByte;
        }

        byte ChkByte = (byte)(((~Checksum) + 1) & 0xFF);

        if (CommonAPIs.NeedDelimiter(ChkByte))
        {
            data.add((byte)Delimiters.DLE);
            FrameLength++;
        }

        data.add(ChkByte);
        FrameLength++;

        data.add((byte)Delimiters.ETX);
        FrameLength++;

        if (DataTxfrType == ChannelType.String)
        {
            List<Byte> str = new ArrayList<Byte>();

            for(byte b : data)
            {
                str.add((byte)(((b & 0xF0) >> 4) + 0x30));
                str.add((byte)((b & 0x0F) + 0x30));
            }

            FrameLength *= 2;

            TriggerTransmit.TransmitFunction(FrameLength, str.toArray(new Byte[str.size()]));
        }
        else // if (DataTxfrType == ChannelType.Number)
        {
            TriggerTransmit.TransmitFunction(FrameLength, data.toArray(new Byte[data.size()]));
        }

        // Once Triggered the transmission, clear the flag
        txMessage.IsTxScheduled = false;

        return ReturnValue.OK;
    }

    private void ResetRxInfo(boolean IsError)
    {
        IsReceiving = false;
        DLCVerified = false;

        if(RxMsgIndex != INVALID_INDEX)
        {
            RxMessage currentRxMsg = RxMessages.get(RxMsgIndex);

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

        RxMessage RxMsg = RxMessages.get(RxMsgIndex);

        if ((RxMsg.CurRxngIdx) == (RxMsgLength + 1 /* ChecksumLength */))
        {
            // If the Maximum Message Buffer Length reached, then do not process further and report error
            retval = ReturnValue.NOK;

            /* Report Error */
            NotifyError.ErrorNotification((int)ErrorCodes.COMIF_EC_INVALID_MSG, CommonAPIs.GetDebugWords(RxMsg.CurRxngIdx, DataByte));

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
                    NotifyError.ErrorNotification((int)ErrorCodes.COMIF_EC_INVALID_MSG, CommonAPIs.GetDebugWords((byte)Delimiters.STX, (byte)RxMsgIndex));

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
                    RxMessage RxMsg = RxMessages.get(RxMsgIndex);
                    int DataLength = RxMsgLength;
                    byte ChecksumLength = (byte)1;

                    // Calculate Checksum if all the bytes were received
                    // If the receive checksum is ignored, but the transmitted still sends the CHK, then the CurRxngIdx will be greater
                    // Even though we receive more data, we only should pass the actual data length to the user

                    if ((RxMsg.CurRxngIdx) == (DataLength + ChecksumLength))
                    {
                        byte ReceivedChecksum = RxMsg.Data[DataLength + 0]; // Last index is the checksum index

                        short CalcChecksum = 0;

                        for (int i = 0; i < DataLength; i++) // Exclude the checksum byte
                        {
                            CalcChecksum += RxMsg.Data[i];
                        }

                        CalcChecksum = (byte)(((~CalcChecksum) + 1) & 0xFF);

                        if (ReceivedChecksum == CalcChecksum)
                        {
                            /* A Valid  Message is being received */

                            /* Send RxCbk */
                            RxMsg.RxCallback((byte)DataLength, RxMsg.Data);

                            /* Reset the Rx Info as No Error */
                            ResetRxInfo(false);
                        }
                        else
                        {
                            /* Reset the Rx Info as Error */
                            ResetRxInfo(true);

                            /* Report Error */
                            NotifyError.ErrorNotification((byte)ErrorCodes.COMIF_EC_INVALID_CHK, CommonAPIs.GetDebugBytes(RxMsg.ID, (byte)0, ReceivedChecksum, (byte)CalcChecksum));
                        }
                    }
                    else
                    {
                        /* If the received information is less than the configured one, then possibly the data might be lost */

                        /* Report Error */
                        NotifyError.ErrorNotification((byte)ErrorCodes.COMIF_EC_INVALID_MSG, CommonAPIs.GetDebugBytes(Delimiters.ETX, RxMsg.CurRxngIdx, (byte)(DataLength & 0xFF), (byte)ChecksumLength));

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
                        for(int i = 0; i < RxMessages.size(); i++)
                        {
                            if(RxMessages.get(i).ID == DataByte)
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
                            NotifyError.ErrorNotification((byte)ErrorCodes.COMIF_EC_INVALID_ID, DataByte);
                        }
                    }
                    else
                    {
                        // ID is available
                        if (DLCVerified == false)
                        {
                            // If the DLC is not verified, then this byte could be the DLC byte
                            RxMessage rxMessage = RxMessages.get(RxMsgIndex);

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
                                NotifyError.ErrorNotification((byte)ErrorCodes.COMIF_EC_INVALID_DLC, CommonAPIs.GetDebugBytes(rxMessage.ID, rxMessage.Length, (byte)(rxMessage.EnableDynamicLength ? 1 : 0), DataByte));
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
        for(int i = 0; i < DataBytes.length; i++)
        {
            RxIndication(DataBytes[i]);
        }

        return ReturnValue.OK;
    }

    public ReturnValue RxIndication(String DataString)
    {
        char[] arr = DataString.toCharArray();

        int Length = arr.length;
        int i = 0;

        if ((Length % 2) == 0)
        {
            while (Length > 0)
            {
                byte Data = 0;

                Data = (byte)((((byte)arr[i] - 0x30) << 4) | ((byte)arr[i + 1] - 0x30));

                RxIndication(Data);

                i += 2; // Move to next step
                Length -= 2;

                /* If the String has an odd number length, then send the last byte also and exit */
                /* This should not actually happen, but still sent to record the exact error instead of ignoring */
                if (arr[ i + 1] == '\0')
                {
                    Data = (byte)((arr[i] - 0x30) << 4);
                    RxIndication(Data);
                    break;
                }
            }

            return ReturnValue.OK;
        }
        else
        {
            // If the length is not an even number, then this is not formed properly.
            return ReturnValue.NOK;
        }
    }
}
