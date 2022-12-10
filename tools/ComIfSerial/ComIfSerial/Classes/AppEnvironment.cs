using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ComIfSerial.Classes
{
    public static class AppEnvironment
    {
        public static string Path = @"C:\Azetech\ComIfSerial";

        private static string SettingsFile = Path + @"\settings.json";

        public static Settings settings = null;

        public static void Initialize()
        {
            if (!Directory.Exists(Path))
            {
                Directory.CreateDirectory(Path);

                Log.Message("Creating Application Environment...");
            }

            settings = new Settings();

            if (!File.Exists(SettingsFile))
            {
                CreateSettingsFile();
            }

            Log.Message("Reading Application Settings...");

            ReadSettingsFile();
        }

        private static void CreateSettingsFile()
        {
            Log.Debug("Writing Settings File...");

            string output = JsonConvert.SerializeObject(settings);

            if (File.Exists(SettingsFile))
            {
                File.Delete(SettingsFile);
            }

            StreamWriter sw = File.CreateText(SettingsFile);
            sw.Write(output);
            sw.Close();
        }

        private static void ReadSettingsFile()
        {
            if (File.Exists(SettingsFile))
            {
                try
                {
                    string settings_readout = File.ReadAllText(SettingsFile);
                    settings = JsonConvert.DeserializeObject<Settings>(settings_readout);

                    Log.Info("User settings read successfully!");
                }
                catch (Exception ex)
                {
                    Log.Error("Exception while reading settings file: " + ex.Message.ToString());
                }
            }
        }

        public static void UpdateSettings()
        {
            CreateSettingsFile();
        }
    }
}
