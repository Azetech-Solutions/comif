using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO.Ports;
using ComIfSerial.Classes;

namespace ComIfSerial.Windows
{
    public partial class ComSettings : Form
    {
        public ComSettings()
        {
            InitializeComponent();

            /* Load the ComboBoxes */
            ComPortComboBox.Items.AddRange(SerialPort.GetPortNames());
            ParityComboBox.Items.Add(Parity.None);
            ParityComboBox.Items.Add(Parity.Even);
            ParityComboBox.Items.Add(Parity.Odd);
            ParityComboBox.Items.Add(Parity.Mark);
            ParityComboBox.Items.Add(Parity.Space);
            StopBitsComboBox.Items.Add(StopBits.None);
            StopBitsComboBox.Items.Add(StopBits.One);
            StopBitsComboBox.Items.Add(StopBits.Two);
            StopBitsComboBox.Items.Add(StopBits.OnePointFive);
            BaudRateComboBox.Items.Add(2400);
            BaudRateComboBox.Items.Add(4800);
            BaudRateComboBox.Items.Add(9600);
            BaudRateComboBox.Items.Add(19200);
            BaudRateComboBox.Items.Add(38400);
            BaudRateComboBox.Items.Add(57600);
            BaudRateComboBox.Items.Add(115200);
            BaudRateComboBox.Items.Add(230400);
            BaudRateComboBox.Items.Add(460800);
            BaudRateComboBox.Items.Add(921600);
        }

        private void ComSettings_Load(object sender, EventArgs e)
        {
            ComPortComboBox.Text = AppEnvironment.settings.ComPort;
            BaudRateComboBox.Text = AppEnvironment.settings.BaudRate.ToString();
            ParityComboBox.Text = AppEnvironment.settings.Parity.ToString();
            StopBitsComboBox.Text = AppEnvironment.settings.StopBits.ToString();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            AppEnvironment.settings.ComPort = ComPortComboBox.Text;
            AppEnvironment.settings.BaudRate = ulong.Parse(BaudRateComboBox.Text);
            AppEnvironment.settings.Parity = ParityComboBox.Text;
            AppEnvironment.settings.StopBits = StopBitsComboBox.Text;

            AppEnvironment.UpdateSettings();

            DialogResult = DialogResult.OK;
        }
    }
}
