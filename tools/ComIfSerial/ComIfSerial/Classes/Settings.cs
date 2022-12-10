using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ComIfSerial.Classes
{
    public class Settings
    {
        public Log.LogLevel LogLevel { get; set; } = Log.LogLevel.Info;
        public string ComPort { get; set; } = "";
        public ulong BaudRate { get; set; } = 57600;
        public string Parity { get; set; } = "None";
        public string StopBits { get; set; } = "None";
    }
}
