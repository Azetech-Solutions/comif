using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ComIfSerial.Classes
{
    class Frame
    {
        public Frame()
        {
            dateTime = DateTime.Now;
            Data = new byte[65]; // 1 byte including Checksum
        }

        public DateTime dateTime;
        public byte ID = 0;
        public byte DLC = 0;
        public byte[] Data = new byte[65]; // 1 byte including Checksum

        public override string ToString()
        {
            // To be updated 
            return base.ToString();
        }
    }
}
