󯻿using System;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;

static class FhdLauncher
{
    [STAThread]
    static int Main()
    {
        try
        {
            string dir = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
            string script = Path.Combine(dir, @"installer\FHD_Setup.ps1");
            if (!File.Exists(script))
            {
                MessageBox.Show("Не найден installer\\FHD_Setup.ps1 рядом с EXE. Распакуйте весь архив.", "FHD MODS", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return 2;
            }
            var process = new Process();
            process.StartInfo = new ProcessStartInfo
            {
                FileName = "powershell.exe",
                Arguments = "-NoLogo -NoProfile -STA -ExecutionPolicy Bypass -File \"" + script + "\"",
                UseShellExecute = false,
                CreateNoWindow = true,
                WorkingDirectory = dir
            };
            process.Start();
            process.WaitForExit();
            return process.ExitCode;
        }
        catch (Exception e)
        {
            MessageBox.Show(e.Message, "FHD MODS", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }
    }
}
