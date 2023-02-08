package in.azetech.comif;

public class Signal {

    public enum DataType
    {
        INT8,
        UINT8,
        INT16,
        UINT16,
        INT32,
        UINT32
        // UINT8_ARRAY --> UINT8 Array data type is not supported in Android
    }

    public enum TransferPropertyType {
        None,
        OnChange,
        OnUpdate
    }

    public enum EndiannessType {
        Big_Endian,
        Little_Endian
    }

    /* Basic Attributes */
    public String Name = "";
    public DataType Type = DataType.UINT8;
    public int StartBitPosition = -1;
    public int Length = -1;
    public TransferPropertyType TransferProperty = TransferPropertyType.None;
    public EndiannessType Endianness = EndiannessType.Big_Endian;

    public Signal(String name, int length, int startBitPosition, DataType type, TransferPropertyType transferProperty, EndiannessType endianness) {
        Name = name;
        Length = length;
        StartBitPosition = startBitPosition;
        Type = type;
        TransferProperty = transferProperty;
        Endianness = endianness;
    }
}
