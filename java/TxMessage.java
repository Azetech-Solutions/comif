package ComIf;

import java.util.ArrayList;
import java.util.List;

public abstract class TxMessage extends Message
{
    public boolean IsTxScheduled = false;

    /* Interface */
    protected abstract void TxCallback(TxMessage txMessage);

    public TxMessage(String name, byte id, byte length) {
        Name = name;
        ID = id;
        Length = length;
        IsTxScheduled = false;
    }
}
