using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ComIfSerial.Classes
{
    public static class Log
    {
        private static RichTextBox richTextBox = null;

        public enum LogLevel
        {
            None,
            Error,
            Warning,
            Info,
            Debug
        };

        private static LogLevel logLevel = LogLevel.None;

        public static void Register(RichTextBox rtb)
        {
            richTextBox = rtb;
            logLevel = LogLevel.Debug;
        }

        public static void SetLogLevel(LogLevel level)
        {
            logLevel = level;
        }

        public static void Clear()
        {
            if (richTextBox != null)
            {
                richTextBox.Text = "";
                richTextBox.ScrollToCaret();
            }
        }

        public static void Write(string Message, Color color, bool prefixTimeStamp = true)
        {
            if (logLevel == LogLevel.None)
            {
                return;
            }

            if (richTextBox != null)
            {
                if (prefixTimeStamp)
                {
                    Message = "[" + DateTime.Now.ToShortTimeString() + "]" + Message;
                }

                try
                {
                    if (richTextBox.InvokeRequired)
                    {
                        richTextBox.Invoke(new MethodInvoker(() =>
                        {
                            richTextBox.AppendText(Message);
                            richTextBox.Select((richTextBox.TextLength - (Message.Length - 1)), Message.Length);
                            richTextBox.SelectionColor = color;
                            richTextBox.Select(richTextBox.TextLength, 0);
                            richTextBox.ScrollToCaret();
                        }));
                    }
                    else
                    {
                        richTextBox.AppendText(Message);
                        richTextBox.Select((richTextBox.TextLength - (Message.Length - 1)), Message.Length);
                        richTextBox.SelectionColor = color;
                        richTextBox.Select(richTextBox.TextLength, 0);
                        richTextBox.ScrollToCaret();
                    }
                }
                catch
                {
                    // Do nothing
                }
            }
        }

        public static void WriteLine(string Message, Color color, bool prefixTimeStamp = true)
        {
            Write(Message + Environment.NewLine, color, prefixTimeStamp);
        }

        public static void Message(string Message)
        {
            if (logLevel >= LogLevel.Info)
            {
                WriteLine(" " + Message, Color.Black);
            }
        }

        public static void Info(string Message)
        {
            if (logLevel >= LogLevel.Info)
            {
                WriteLine("[I] " + Message, Color.Black);
            }
        }

        public static void Warning(string Message)
        {
            if (logLevel >= LogLevel.Warning)
            {
                WriteLine("[W] " + Message, Color.OrangeRed);
            }
        }

        public static void Error(string Message)
        {
            if (logLevel >= LogLevel.Error)
            {
                WriteLine("[E] " + Message, Color.Red);
            }
        }

        public static void Success(string Message)
        {
            if (logLevel >= LogLevel.Error)
            {
                WriteLine(" " + Message, Color.Green);
            }
        }

        public static void Debug(string Message)
        {
            if (logLevel >= LogLevel.Debug)
            {
                WriteLine("[D] " + Message, Color.Navy);
            }
        }
    }
}
