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
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.StandaloneLogCollector;

    [TestClass]
    public class ProgramTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TryParseArgsTest()
        {
            Assert.IsFalse(this.InternalTryParseArgsTest(new string[] { "heihei", "hiahia" }));
            Assert.IsFalse(this.InternalTryParseArgsTest(new string[] { "/?" }));
            Assert.IsFalse(this.InternalTryParseArgsTest(new string[] { "help" }));

            string existingDirPath = TestUtility.TestDirectory + "\\exist";
            Directory.CreateDirectory(existingDirPath);
            Assert.IsFalse(this.InternalTryParseArgsTest(new string[] { "-output", existingDirPath, "-mode", "collect" }));
            Directory.Delete(existingDirPath, true);

            Assert.IsTrue(this.InternalTryParseArgsTest(new string[] { "-mode", "collect"}));
            Assert.IsTrue(this.InternalTryParseArgsTest(new string[] { "startutctime", "01/26/2017", "endutctime", "01/27/2017", "-mode", "collect" }));

            string nonExistingDirPath = TestUtility.TestDirectory + "\\nonexist";
            Assert.IsTrue(this.InternalTryParseArgsTest(new string[] { "-output", nonExistingDirPath, "-mode", "collect"}));

            nonExistingDirPath = "%systemdrive%\\nonexist";
            Assert.IsTrue(this.InternalTryParseArgsTest(new string[] { "/output", nonExistingDirPath, "-mode", "collect" }));

            nonExistingDirPath = TestUtility.TestDirectory + "\\1\\2\\3\\nonexist";
            Assert.IsTrue(this.InternalTryParseArgsTest(new string[] { "output", nonExistingDirPath, "-mode", "collect" }));
            Directory.Delete(TestUtility.TestDirectory + "\\1", true);
        }

        internal bool InternalTryParseArgsTest(string[] args)
        {
            Program.Config = null;

            List<string> argList = args == null ? new List<string>() : args.ToList();
            argList.Add("-accepteula");

            bool result = Program.TryParseArgs(argList.ToArray());
            if (result)
            {
                Assert.IsNotNull(Program.Config.OutputDirectoryPath);
                Assert.IsNotNull(Program.Config.WorkingDirectoryPath);
            }
            else
            {
                Assert.IsNull(Program.Config);
            }

            TestUtility.TestEpilog();
            return result;
        }
    }
}