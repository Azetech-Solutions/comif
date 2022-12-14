using ComIf;
using System.Diagnostics;

// See https://aka.ms/new-console-template for more information
Console.WriteLine("Example Code to integrate the ComIf Classes");


#region "Custom Project Functions"

void NumberChannelTransmit(UInt16 Length, byte[] Data)
{
    Console.Write("--> Tx :");
    for (int i = 0; i < Length; i++)
    {
        Console.Write(" " + Data[i].ToString("X"));
    }
    Console.WriteLine();
}

void NumberChannelErrorNotification(UInt32 Debug0, UInt32 Debug1)
{
    Console.WriteLine("--> Error : " + Debug0.ToString() + " 0x" + Debug1.ToString("X"));
}

void NumberChannelRxCbk(byte Length, byte[] Data)
{
    Console.Write("--> Rx :");
    for (int i = 0; i < Length; i++)
    {
        Console.Write(" " + Data[i].ToString("X"));
    }
    Console.WriteLine();
}

#endregion

/* Creating the ComIf Channel */
Channel numberChannel = new Channel("NumberChannel", ChannelType.Number, NumberChannelTransmit, NumberChannelErrorNotification);

// Creating and Sending a Tx Message
TxMessage txMessage = new TxMessage(0x12, 8);
numberChannel.Transmit(txMessage);

// Creating and Registering an Rx Message
RxMessage rxMessage = new RxMessage(0x13, 8, NumberChannelRxCbk);
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
RxDataBytes = new byte[] { 0x7B, 0x13, 0x08, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0x7D };
numberChannel.RxIndication(RxDataBytes);

// Sending Invalid Message --> By sending ETX as differnt value
RxDataBytes = new byte[] { 0x7B, 0x13, 0x08, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF };
numberChannel.RxIndication(RxDataBytes);
