package ComIf;

import java.util.ArrayList;
import java.util.List;

public abstract class RxMessage extends Message
{
    public boolean EnableDynamicLength = false;

    /* Status Flags */
    public boolean ReceptionStarted = false; // If set, then the message has been started receiving
    public boolean NewMessageReceived = false; // If set, then the message has been received completely and waiting for the RxCbk to be called
    public boolean ErrorInReception = false; // If set, then the message has been received, but there is an error in reception
    public byte CurRxngIdx = 0;

    protected abstract void RxCallback(byte Length, byte[] Data);

    public RxMessage(String name, byte id, byte length)
    {
        Name = name;
        ID = id;
        Length = length;
    }
}