package in.azetech.comif;

public abstract class RxMessage
{
    /* Basic Attributes */
    public byte ID = 0;
    public byte Length = 0;
    public byte[] Data = new byte[256]; // 1 byte including Checksum

    public boolean EnableDynamicLength = false;

    /* Status Flags */
    public boolean ReceptionStarted = false; // If set, then the message has been started receiving
    public boolean NewMessageReceived = false; // If set, then the message has been received completely and waiting for the RxCbk to be called
    public boolean ErrorInReception = false; // If set, then the message has been received, but there is an error in reception
    public byte CurRxngIdx = 0;

    protected abstract void RxCallback(byte Length, byte[] Data);

    public RxMessage(byte id, byte length)
    {
        ID = id;
        Length = length;
    }
}