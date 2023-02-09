package ComIf;

import java.util.ArrayList;
import java.util.List;

public abstract class Message {

    /* Basic Attributes */
    public String Name = "";
    public byte ID = 0;
    public byte Length = 0;
    public byte[] Data = new byte[256]; // 1 byte including Checksum

    public List<Signal> Signals = new ArrayList<>();

    public boolean RegisterSignal(Signal signal) {
        boolean isOkayToAdd = true;

        for (Signal mySignal : Signals) {
            if (mySignal.Name.equals(signal.Name)) {
                isOkayToAdd = false;
                break;
            }
        }

        if(isOkayToAdd) {
            Signals.add(signal);
        }

        return isOkayToAdd;
    }

    public boolean UpdateSignalToBuffer(String Name, int value) {
        boolean retval = true;
        Signal signal = null;

        for (Signal mySignal : Signals) {
            if (mySignal.Name.equals(Name)) {
                signal = mySignal;
                break;
            }
        }

        if(signal != null) {
            byte BytePosition = (byte)(signal.StartBitPosition / 8);
            byte BitPostion = (byte)(signal.StartBitPosition % 8);

            int Mask = ComIf_GetBitMaskValue(signal.Length);

            // TODO: Implement the correct logic
            Data[BytePosition] &= ~(Mask << BitPostion);
            Data[BytePosition] |= (byte)((value & Mask) << BitPostion);
        }
        else {
            retval = false;
        }

        return retval;
    }

    public int GetSignalFromBuffer(String Name) {
        int value = 0;
        Signal signal = null;

        for (Signal mySignal : Signals) {
            if (mySignal.Name.equals(Name)) {
                signal = mySignal;
                break;
            }
        }

        if(signal != null) {
            byte BytePosition = (byte)(signal.StartBitPosition / 8);
            byte BitPostion = (byte)(signal.StartBitPosition % 8);

            int Mask = ComIf_GetBitMaskValue(signal.Length);

            // TODO: Implement the correct logic
            value = (int)((Data[BytePosition] & (Mask << BitPostion)) >> BitPostion);
        }

        return value;
    }

    private int ComIf_GetBitMaskValue(int BitLength)
    {
        int Mask = 0x01; // Default 1 Bit Mask

        if(BitLength > 32)
        {
            BitLength = 32;
        }

        while((--BitLength) > 0)
        {
            Mask <<= 1;
            Mask |= 0x01;
        }

        return Mask;
    }
}
