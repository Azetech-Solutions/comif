package ComIf;

public class Example
{
    private static void NumberChannelTransmit(short Length, Byte[] Data)
    {
        System.out.print("--> Tx :");
        for (int i = 0; i < Length; i++)
        {
            System.out.print(" " + String.format("%02X", Data[i]));
        }
        System.out.println();
    }

    private static void NumberChannelErrorNotification(int Debug0, int Debug1)
    {
        System.out.println("--> Error : " + String.format("%d", Debug0) + " 0x" + String.format("%02X", Debug1));
    }

    private static void NumberChannelRxCbk(byte Length, byte[] Data)
    {
        System.out.print("--> Rx :");
        for (int i = 0; i < Length; i++)
        {
            System.out.print(" " + String.format("%02X", Data[i]));
        }
        System.out.println();
    }

    public static void main(String[] args)
    {
        System.out.println("Example Code to integrate the ComIf Package");

        /* Creating the ComIf Channel */
        Channel numberChannel = new Channel("NumberChannel",
                                            Channel.ChannelType.Number,
                                            new Channel.TransmitFunctionCallback() {
                                                public void TransmitFunction(short Length, Byte[] Data)
                                                {
                                                    NumberChannelTransmit(Length, Data);
                                                }
                                            },
                                            new Channel.ErrorNotificationListener() {
                                                public void ErrorNotification(int Debug0, int Debug1)
                                                {
                                                    NumberChannelErrorNotification(Debug0, Debug1);
                                                }
                                            }
                                            );

        // Creating and Sending a Tx Message
        Channel.TxMessage txMessage = numberChannel.new TxMessage((byte)0x12, (byte)8);
        numberChannel.Transmit(txMessage);

        // Creating and Registering an Rx Message
        Channel.RxMessage rxMessage = numberChannel.new RxMessage((byte)0x13, (byte)8, new Channel.RxMessage.RxCallbackListener(){
                                                                public void RxCallback(byte Length, byte[] Data)
                                                                {
                                                                    NumberChannelRxCbk(Length, Data);
                                                                }
                                                            });
        numberChannel.RegisterRxMessage(rxMessage);

        // Sending an Rx Message to the channel
        byte[] RxDataBytes = { 0x7B, 0x13, 0x08, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7D};
        numberChannel.RxIndication(RxDataBytes);

        // Sending Invalid ID --> By sending ID with non-registering value
        RxDataBytes = new byte[] { 0x7B, 0x12, 0x08, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7D };
        numberChannel.RxIndication(RxDataBytes);

        // Sending Invalid DLC 
        RxDataBytes = new byte[] { 0x7B, 0x13, 0x06, 0, 0, 0, 0, 0, 0, 0, 0x7D };
        numberChannel.RxIndication(RxDataBytes);

        // Sending invalid checksum
        RxDataBytes = new byte[] { 0x7B, 0x13, 0x08, 0, 0, 0, 0, 0, 0, 0, 0, (byte)0xFF, 0x7D };
        numberChannel.RxIndication(RxDataBytes);

        // Sending Invalid Message --> By sending ETX as differnt value
        RxDataBytes = new byte[] { 0x7B, 0x13, 0x08, 0, 0, 0, 0, 0, 0, 0, 0, 0, (byte)0xFF };
        numberChannel.RxIndication(RxDataBytes);
    }
}