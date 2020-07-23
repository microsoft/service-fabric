// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;

    public class LogManUtil
    {
        private const string sessionStartFormatString = "Start {0} -p {1} {2} -o {3} -mode Circular -max 500 -bs 32 -nb 120 320 -ets";
        private const string sessionStopFormatString = "Stop {0} -ets";

        private const string windowsFabricProviderId = "{cbd93bc2-71e5-4566-b3a7-595d8eeca6e8}";
        
        
        private static readonly List<Tuple<string, string>> Providers = new List<Tuple<string, string>>()
        {
            new Tuple<string, string>("winfab", windowsFabricProviderId),
        };

        public static void StartTracing(string folder)
        {
            foreach (var item in LogManUtil.Providers)
            {
                string etlOutPath = Path.Combine(folder, item.Item1 + ".etl");
                string args = string.Format(CultureInfo.InvariantCulture, sessionStartFormatString, item.Item1, item.Item2, string.Empty, etlOutPath);

                ExecuteProcess("Logman.exe", args);
            }
        }

        public static void StopTracing()
        {
            foreach (var item in LogManUtil.Providers)
            {
                string args = string.Format(CultureInfo.InvariantCulture, sessionStopFormatString, item.Item1);

                ExecuteProcess("Logman.exe", args);
            }
        }

        private static void ExecuteProcess(string filename, string args)
        {
            Trace.WriteLine("Executing " + filename + " " + args);
            using (Process p = new Process())
            {
                p.StartInfo.FileName = filename;
                p.StartInfo.Arguments = args;
                p.StartInfo.RedirectStandardError = true;
                p.StartInfo.RedirectStandardOutput = true;
                p.StartInfo.UseShellExecute = false;
                p.EnableRaisingEvents = true;
                p.ErrorDataReceived += delegate(object sender, DataReceivedEventArgs eventArgs)
                {
                    Trace.WriteLine(eventArgs.Data);
                };

                p.OutputDataReceived += delegate(object sender, DataReceivedEventArgs eventArgs)
                {
                    Trace.WriteLine(eventArgs.Data);
                };

                p.Start();
                p.BeginErrorReadLine();
                p.BeginOutputReadLine();
                p.WaitForExit();
                Trace.WriteLine("Exit Code: " + p.ExitCode);
            }
        }
    }
}