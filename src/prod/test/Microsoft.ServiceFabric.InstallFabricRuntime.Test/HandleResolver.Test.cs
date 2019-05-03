// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.InstallFabricRuntime.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.IO;
    using System.Threading;
    using System.Reflection;

    [TestClass]
    public class HandleResolverTest
    {
        private static string BaseDir = new FileInfo(Assembly.GetExecutingAssembly().Location).DirectoryName;

        static readonly string folderPath;
        static readonly string dllPath;
        static readonly string scriptPath;

        static HandleResolverTest()
        {
            folderPath = Path.Combine(HandleResolverTest.BaseDir, "HandleResolver");
            dllPath = Path.Combine(HandleResolverTest.BaseDir, "HandleResolver\\SdkRuntimeCompatibilityCheck.dll");
            scriptPath = Path.Combine(HandleResolverTest.BaseDir, "InstallFabricRuntime.Test.LoadDll.ps1");
        }

        internal Process StartProcess(string processName, string commandLineArgs = null)
        {
            Process process = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = processName,
                    Arguments = commandLineArgs,
                    WindowStyle = ProcessWindowStyle.Hidden
                }
            };

            process.Start();
            return process;
        }

        internal bool ValidateBlockingProcesses(int expected, string path, string[] expectedBlockProceses)
        {
            List<string> procs = HandleResolver.GetLockingProcesses(path);
            if (expected != procs.Count)
            {
                return false;
            }

            if (expected == 0)
            {
                return true; 
            }

            return procs.All(x => expectedBlockProceses.Any(y => y.Equals(x, StringComparison.OrdinalIgnoreCase)));
        }

        [TestMethod]
        [Owner("taxi")]
        public void HandleResolver_CheckPowershellLoadDll()
        {
            Assert.IsTrue(this.ValidateBlockingProcesses(0, folderPath, null));

            Process process = this.StartProcess("powershell.exe", string.Format("\"{0}\" \"{1}\"", scriptPath, dllPath));
            Thread.Sleep(5000);

            string[] knowConsumers = { "powershell.exe", Path.GetFileName(Process.GetCurrentProcess().MainModule.FileName)};

            using (FileStream fs = File.Open(Path.Combine(HandleResolverTest.BaseDir, "HandleResolver\\temp.txt"), FileMode.OpenOrCreate))
            {
                Assert.IsTrue(this.ValidateBlockingProcesses(2, folderPath, knowConsumers));
            }

            Assert.IsTrue(this.ValidateBlockingProcesses(1, folderPath, knowConsumers));

            process.WaitForExit();
            Assert.IsTrue(this.ValidateBlockingProcesses(0, folderPath, null));
        }

        [TestMethod]
        [Owner("taxi")]
        public void HandleResolver_CheckFileOpen()
        {
            Assert.IsTrue(this.ValidateBlockingProcesses(0, folderPath, null));

            using (FileStream fs = File.Open(Path.Combine(HandleResolverTest.BaseDir, "HandleResolver\\temp.txt"), FileMode.OpenOrCreate))
            {
                string[] knowConsumers = { Path.GetFileName(Process.GetCurrentProcess().MainModule.FileName) };
                Assert.IsTrue(this.ValidateBlockingProcesses(1, folderPath, knowConsumers));
            }
            
            Assert.IsTrue(this.ValidateBlockingProcesses(0, folderPath, null));
        }
    }
}