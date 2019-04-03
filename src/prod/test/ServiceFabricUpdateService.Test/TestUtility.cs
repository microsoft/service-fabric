// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ServiceFabricUpdateService.Test
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Security.Cryptography.X509Certificates;
    using System.Reflection;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ServiceFabricUpdateService;

    internal static class TestUtility
    {
        internal static string TestDirectory = new FileInfo(Assembly.GetExecutingAssembly().Location).DirectoryName;
        
        static TestUtility()
        {
            Directory.SetCurrentDirectory(TestUtility.TestDirectory);
        }

        public static void UninstallService(string svcName)
        {
            string scExePath = Environment.ExpandEnvironmentVariables(@"%windir%\system32\sc.exe");
            ProcessStartInfo start = new ProcessStartInfo()
            {
                FileName = scExePath,
                Arguments = "delete " + svcName,
                WindowStyle = ProcessWindowStyle.Hidden,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            using (Process process = Process.Start(start))
            {
                process.WaitForExit();
            }
        }
    }
}