package ComIf.msgs;

import ComIf.RxMessage;

class RxMsg01 extends RxMessage {

    private static final String Name = "RxMsg01";
    private static final byte ID = (byte)0xD1;
    private static final byte Length = 8;

    public RxMsg01() {
        super(Name, ID, Length);
    }

    @Override
    protected void RxCallback(byte Length, byte[] Data) {

    }
}
