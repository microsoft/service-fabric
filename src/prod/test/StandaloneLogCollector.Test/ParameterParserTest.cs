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

    using ParameterNames = Microsoft.ServiceFabric.StandaloneLogCollector.ProgramParameterDefinitions.ParameterNames;

    [TestClass]
    public class ParameterParserTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConstructorTest()
        {
            ParameterParser parser = new ParameterParser(
                ProgramParameterDefinitions.ParameterDefinitions,
                ProgramParameterDefinitions.ValidParameterNamePrefixes);
            Assert.AreEqual(ProgramParameterDefinitions.ParameterDefinitions.Count(), parser.Definitions.Count());
            Assert.AreEqual(ProgramParameterDefinitions.ValidParameterNamePrefixes.Count(), parser.ValidPrefixes.Count());

            parser = new ParameterParser(
                ProgramParameterDefinitions.ParameterDefinitions,
                null);
            Assert.AreEqual(ProgramParameterDefinitions.ParameterDefinitions.Count(), parser.Definitions.Count());
            Assert.AreEqual(0, parser.ValidPrefixes.Count());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TryParseTest()
        {
            ParameterParser parser = new ParameterParser(
                ProgramParameterDefinitions.ParameterDefinitions,
                ProgramParameterDefinitions.ValidParameterNamePrefixes);

            Dictionary<string, string> result;

            Assert.IsTrue(parser.TryParse(new string[] { }, out result));
            Assert.IsFalse(result.Any());

            Assert.IsTrue(parser.TryParse(new string[] { "?"}, out result));
            Assert.AreEqual(1, result.Count);
            Assert.AreEqual(true, bool.Parse(result[ParameterNames.QuestionMark]));
            Assert.IsTrue(parser.TryParse(new string[] { "/?" }, out result));
            Assert.AreEqual(1, result.Count);
            Assert.AreEqual(true, bool.Parse(result[ParameterNames.QuestionMark]));
            Assert.IsTrue(parser.TryParse(new string[] { "help" }, out result));
            Assert.AreEqual(1, result.Count);
            Assert.AreEqual(true, bool.Parse(result[ParameterNames.Help]));
            Assert.IsTrue(parser.TryParse(new string[] { "-help" }, out result));
            Assert.AreEqual(1, result.Count);
            Assert.AreEqual(true, bool.Parse(result[ParameterNames.Help]));
            Assert.IsTrue(parser.TryParse(new string[] { "-help", "?" }, out result));
            Assert.AreEqual(2, result.Count);
            Assert.AreEqual(true, bool.Parse(result[ParameterNames.Help]));
            Assert.AreEqual(true, bool.Parse(result[ParameterNames.QuestionMark]));

            Assert.IsTrue(parser.TryParse(new string[] { "-output", "hiahia"}, out result));
            Assert.AreEqual("hiahia", result[ParameterNames.Output]);
            Assert.AreEqual(1, result.Count);

            Assert.IsTrue(parser.TryParse(new string[] { "/startutctime", "01/01/2018" }, out result));
            Assert.AreEqual("01/01/2018", result[ParameterNames.StartUtcTime]);
            Assert.AreEqual(1, result.Count);

            Assert.IsTrue(parser.TryParse(new string[] { "endutctime", "01/01/2018" }, out result));
            Assert.AreEqual("01/01/2018", result[ParameterNames.EndUtcTime]);
            Assert.AreEqual(1, result.Count);

            Assert.IsTrue(parser.TryParse(new string[] { "-includeleaselogs" }, out result));
            Assert.AreEqual(true.ToString(), result[ParameterNames.IncludeLeaseLogs]);
            Assert.AreEqual(1, result.Count);

            Assert.IsTrue(parser.TryParse(new string[] { "-includecrashdumps" }, out result));
            Assert.AreEqual(true.ToString(), result[ParameterNames.IncludeCrashDumps]);
            Assert.AreEqual(1, result.Count);

            Assert.IsTrue(parser.TryParse(new string[] { "-output", "hiahia", "-includeleaselogs", "/startutctime", "01/01/2018", "endutctime", "01/01/2019", "-includecrashdumps" }, out result));
            Assert.AreEqual(5, result.Count);

            Assert.IsTrue(parser.TryParse(new string[] { "-output", "hiahia", "-includeleaselogs", "/startutctime", "01/01/2018", "endutctime", "01/01/2019", "-mode", "upload" }, out result));

            Assert.IsFalse(parser.TryParse(new string[] { "-?", "-includeleaselogs" }, out result));
            Assert.IsFalse(parser.TryParse(new string[] { "-?", "-includecrashdumps" }, out result));
            Assert.IsFalse(parser.TryParse(new string[] { "-output", "hiahia", "-?" }, out result));
        }
    }
}