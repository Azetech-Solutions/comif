package in.azetech.comif.msgs;

import in.azetech.comif.RxMessage;

class RxMsg01 extends RxMessage {

    private static final byte ID = (byte)0xD1;
    private static final byte Length = 8;

    public RxMsg01() {
        super(ID, Length);
    }

    @Override
    protected void RxCallback(byte Length, byte[] Data) {

    }
}
