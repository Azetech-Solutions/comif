using ComIfSerial.Classes;
using ComIfSerial.Windows;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ComIfSerial
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();

            InitializeBasics();
        }

        #region "Basics"

        public void InitializeBasics()
        {
            // Register the rich text box for the log
            Log.Register(richTextBox1);

            // Initialize Environment
            AppEnvironment.Initialize();

            // Update the log level as per the user settings
            SetLogLevel(AppEnvironment.settings.LogLevel);
        }

        #endregion

        #region "Log related Events"

        private void SetLogLevel(Log.LogLevel logLevel)
        {
            noneToolStripMenuItem.Checked = false;
            warningToolStripMenuItem.Checked = false;
            infoToolStripMenuItem.Checked = false;
            errorToolStripMenuItem.Checked = false;
            debugToolStripMenuItem.Checked = false;

            switch (logLevel)
            {
                case Log.LogLevel.Debug: debugToolStripMenuItem.Checked = true; break;
                case Log.LogLevel.Error: errorToolStripMenuItem.Checked = true; break;
                case Log.LogLevel.Warning: warningToolStripMenuItem.Checked = true; break;
                case Log.LogLevel.Info: infoToolStripMenuItem.Checked = true; break;
                case Log.LogLevel.None:
                default:
                    noneToolStripMenuItem.Checked = true;
                    logLevel = Log.LogLevel.None;
                    break;

            }

            Log.SetLogLevel(logLevel);

            AppEnvironment.settings.LogLevel = logLevel;
        }

        private void noneToolStripMenuItem_Click(object sender, EventArgs e)
        {
            SetLogLevel(Log.LogLevel.None);
            AppEnvironment.UpdateSettings();
        }

        private void infoToolStripMenuItem_Click(object sender, EventArgs e)
        {
            SetLogLevel(Log.LogLevel.Info);
            AppEnvironment.UpdateSettings();
        }

        private void warningToolStripMenuItem_Click(object sender, EventArgs e)
        {
            SetLogLevel(Log.LogLevel.Warning);
            AppEnvironment.UpdateSettings();
        }

        private void errorToolStripMenuItem_Click(object sender, EventArgs e)
        {
            SetLogLevel(Log.LogLevel.Error);
            AppEnvironment.UpdateSettings();
        }

        private void debugToolStripMenuItem_Click(object sender, EventArgs e)
        {
            SetLogLevel(Log.LogLevel.Debug);
            AppEnvironment.UpdateSettings();
        }

        private void clearLogToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Log.Clear();
        }

        #endregion

        #region "Serial Communication related"

        private static SerialPort serialPort = new SerialPort();

        private bool isMeasurementRunning = false;

        private void RecordData()
        {
            if (AppEnvironment.settings.ComPort != "")
            {
                /* Start the Measurement */
                try
                {
                    if (serialPort.IsOpen)
                    {
                        Thread.Sleep(50);

                        serialPort.DiscardInBuffer();
                        serialPort.Close(); // Close the serial port if already opened
                    }

                    /* First Configure the Serial Port */
                    serialPort = new SerialPort(AppEnvironment.settings.ComPort, (int)AppEnvironment.settings.BaudRate, Parity.None, 8, StopBits.One);
                    serialPort.Handshake = Handshake.None;
                    serialPort.RtsEnable = true;
                    // serialPort.DtrEnable = true;

                    serialPort.ErrorReceived += SerialPortErrorHandler;
                    serialPort.DataReceived += SerialPortDataReceivedHandler;

                    /* Open the connection */
                    serialPort.Open();

                    startStopToolStripMenuItem.Text = "Stop";

                    Log.Message("ComIf Serial Started...");

                    isMeasurementRunning = true;
                }
                catch (Exception ex)
                {
                    Log.Error(ex.Message);
                }
            }
            else
            {
                Log.Error("Please select the Com port to proceed with the Diagnosing!");
            }
        }

        private void SerialPortErrorHandler(object sender, SerialErrorReceivedEventArgs e)
        {
            Log.Error("Serial Port Error : " + e.EventType.ToString());
            // StopRecording(); --> DO NOT STOP Recording, as there might be a possibility that the Serial port is open in between the transmission, results in Frame Error
        }

        Frame tempFrame = null;
        byte previousDataByte = 0;
        bool delimit = false;
        uint rxLength = 0;

        private void SerialPortDataReceivedHandler(object sender, SerialDataReceivedEventArgs e)
        {
            int BytesToRead = serialPort.BytesToRead;

            if (BytesToRead > 0)
            {
                byte[] buffer = new byte[BytesToRead];

                serialPort.Read(buffer, 0, BytesToRead);

                foreach (byte data in buffer)
                {
                    if (tempFrame != null) // There is a new frame needed to be parsed
                    {
                        if (tempFrame.ID == 0)
                        {
                            tempFrame.ID = data;
                        }
                        else if (tempFrame.DLC == 0)
                        {
                            tempFrame.DLC = data;
                        }
                        else if (delimit == true)
                        {
                            tempFrame.Data[rxLength++] = data;
                            delimit = false;
                        }
                        else if (data == 0x7C) // Received a delimiter
                        {
                            delimit = true;
                        }
                        else if (data == 0x7D)
                        {
                            // End of Frame
                            AddFrame(tempFrame);
                            tempFrame = null;
                            rxLength = 0;
                        }
                        else
                        {
                            tempFrame.Data[rxLength++] = data;
                        }
                    }
                    else
                    {
                        if ((data == 0x7B) && (previousDataByte != 0x7C))
                        {
                            tempFrame = new Frame();
                            delimit = false;
                            rxLength = 0;
                        }
                    }

                    previousDataByte = data;
                }
            }
        }

        public void StopRecording()
        {
            if (serialPort.IsOpen)
            {
                Thread.Sleep(50);

                serialPort.Close();
            }

            startStopToolStripMenuItem.Text = "Start";

            Log.Message("ComIf Serial Stopped!");

            isMeasurementRunning = false;
        }

        #endregion

        private void cOMSettingsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ComSettings comSettings = new ComSettings();

            if (comSettings.ShowDialog() == DialogResult.OK)
            {
                Log.Message("Serial Port Configured : " + AppEnvironment.settings.ComPort + " @" + AppEnvironment.settings.BaudRate.ToString() + " Baudrate");
            }
        }

        private void startToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if(isMeasurementRunning == false)
            {
                RecordData();
            }
            else
            {
                StopRecording();
            }
        }

        private void AddFrame(Frame frame)
        {
            // Display it in the log text box
        }
    }
}
