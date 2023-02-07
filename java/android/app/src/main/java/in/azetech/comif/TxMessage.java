package in.azetech.comif;

public abstract class TxMessage
{
    /* Basic Attributes */
    public byte ID = 0;
    public byte Length = 0;
    public byte[] Data = new byte[256]; // 1 byte including Checksum

    public boolean IsTxScheduled = false;

    /* Interface */
    protected abstract void TxCallback(TxMessage txMessage);

    public TxMessage(byte id, byte length)
    {
        ID = id;
        Length = length;
        IsTxScheduled = false;
    }
}
