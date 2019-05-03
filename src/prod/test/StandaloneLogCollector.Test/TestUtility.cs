// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace StandaloneLogCollector.Test
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Security.Cryptography.X509Certificates;
    using System.Reflection;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.StandaloneLogCollector;
    using Microsoft.Tracing.Azure;

    internal static class TestUtility
    {
        internal static string TestDirectory = new FileInfo(Assembly.GetExecutingAssembly().Location).DirectoryName;

        internal static readonly Dictionary<string, string> ExistingTestLogs = new Dictionary<string, string>
            {
                { "/Config", "log1.txt" },
                { "/Firewall", "log2.txt" },
                { "/Registry", "log3.txt" }
            };

        internal static readonly Dictionary<string, string> GeneratedTestLogs = new Dictionary<string, string>
            {
                { "/Generated", "log4.txt" }
            };

        static TestUtility()
        {
            Directory.SetCurrentDirectory(TestUtility.TestDirectory);
        }

        internal static void TestProlog()
        {
            TestUtility.TestProlog(new string[2]{"-mode", "collect"});
        }

        internal static void TestProlog(string[] args)
        {
            Program.Config = null;
            List<string> arguments = new List<string>() { "-AcceptEula" };
            if (args != null)
            {
                arguments.AddRange(args);
            }

            Assert.IsTrue(Program.TryParseArgs(arguments.ToArray()));
        }

        internal static void TestEpilog()
        {
            if (Program.Config != null)
            {
                if (!string.IsNullOrWhiteSpace(Program.Config.OutputDirectoryPath))
                {
                    Directory.Delete(Program.Config.OutputDirectoryPath, recursive: true);
                }

                if (!string.IsNullOrWhiteSpace(Program.Config.WorkingDirectoryPath))
                {
                    Directory.Delete(Program.Config.WorkingDirectoryPath, recursive: true);
                }
            }

            Program.Config = null;
        }

        internal static AzureSasKeyCredential GetAzureCredential()
        {
            string sasConnectionString = "https://liphistorageaccount.blob.core.windows.net/container1?sv=2015-04-05&sr=c&sig=w0tlW6%2BaeHOR%2FIE%2FxASnaPn%2B%2BWvuWD67%2BkmNgKEFLWE%3D&se=2017-02-18T07%3A17%3A41Z&sp=rwdl";
            return AzureSasKeyCredential.CreateAzureSasKeyCredential(sasConnectionString);
        }
    }
}