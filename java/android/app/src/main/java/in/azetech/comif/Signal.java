package in.azetech.comif;

public class Signal {

    public enum DataType
    {
        INT8,
        UINT8,
        INT16,
        UINT16,
        INT32,
        UINT32;
        // UINT8_ARRAY --> UINT8 Array data type is not supported in Android

        public static DataType fromName(String Name) {
            DataType type = UINT32;
            switch (Name) {
                case "INT8" : type = INT8; break;
                case "UINT8" : type = UINT8; break;
                case "INT16" : type = INT16; break;
                case "UINT16" : type = UINT16; break;
                case "INT32" : type = INT32; break;
                default: break;
            }
            return type;
        }
    }

    public enum TransferPropertyType {
        None,
        OnChange,
        OnUpdate;

        public static TransferPropertyType fromName(String Name) {
            TransferPropertyType propertyType = None;

            switch (Name) {
                case "OnChange" : propertyType = OnChange; break;
                case "OnUpdate" : propertyType = OnUpdate; break;
            }

            return propertyType;
        }
    }

    public enum EndiannessType {
        Big_Endian,
        Little_Endian;

        public static EndiannessType fromName(String Name) {
            if(Name.toUpperCase().equals("LITTLE")) {
                return Little_Endian;
            }
            return Big_Endian;
        }
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
