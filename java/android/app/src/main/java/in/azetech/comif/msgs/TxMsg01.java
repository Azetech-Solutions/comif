package in.azetech.comif.msgs;

import in.azetech.comif.TxMessage;

public class TxMsg01 extends TxMessage {

    private static final String Name = "RxMsg01";
    private static final byte ID = (byte)0x1D;
    private static final byte Length = 8;

    public TxMsg01() {
        super(Name, ID, Length);
    }

    @Override
    protected void TxCallback(TxMessage txMessage) {

    }
}
