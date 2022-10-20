// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System.Diagnostics;
    using System.Globalization;
    using System.Threading;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    public class ProcessUtility
    {
        public static void KillRunningFabricExe()
        {
            bool killed = false;
            foreach (var fabricExeProcess in Process.GetProcessesByName("Fabric"))
            {
                try
                {
                    killed = true;
                    if (fabricExeProcess.MainModule.FileName.Contains("SFT"))
                    {
                        LogHelper.Log("Killing fabric.exe {0}", fabricExeProcess.Id);
                        fabricExeProcess.Kill();
                    }
                }
                catch (Exception)
                {
                }
            }

            if (killed)
            {
                // wait for them to die
                Thread.Sleep(500);
            }
        }

        
        internal static void RunAndWaitForExit(string command, string args, int additionalTimeToWait)
        {
            LogHelper.Log(string.Format(CultureInfo.InvariantCulture, "Launching: {0} with {1}", command, args));

            var p = Process.Start(command, args);
            p.WaitForExit();

            Assert.AreEqual(0, p.ExitCode);

            Thread.Sleep(additionalTimeToWait);
        }
    }
}