package in.azetech.comif.msgs;

import in.azetech.comif.TxMessage;

public class TxMsg01 extends TxMessage {

    private static final byte ID = (byte)0x1D;
    private static final byte Length = 8;

    public TxMsg01() {
        super(ID, Length);
    }

    @Override
    protected void TxCallback(TxMessage txMessage) {

    }
}
