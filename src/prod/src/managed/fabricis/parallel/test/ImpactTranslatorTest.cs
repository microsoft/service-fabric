// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.ParallelTest
{ 
    using System;
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Fabric.InfrastructureService.Parallel;
    using System.Fabric.InfrastructureService.Test;
    using System.Fabric.Repair;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Threading.Tasks;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using RD.Fabric.PolicyAgent;

    [TestClass]
    public class ImpactTranslatorTest
    {
        private static readonly TraceType TraceType = new TraceType("ImpactTranslatorTest");
        private const string ConfigSectionName = "ISConfig";

        private MockConfigStore configStore;
        private ImpactTranslator translator;

        [TestInitialize]
        public void TestInitialize()
        {
            this.configStore = new MockConfigStore();

            var env = new CoordinatorEnvironment(
                "fabric:/System/InfrastructureService",
                new ConfigSection(TraceType, this.configStore, ConfigSectionName),
                string.Empty,
                new MockInfrastructureAgentWrapper());

            this.translator = new ImpactTranslator(env);
        }

        [TestMethod]
        public void ScriptedTest()
        {
            string scriptPath = Path.Combine(
                Path.GetDirectoryName(typeof(ImpactTranslatorTest).GetTypeInfo().Assembly.Location),
                "ImpactTranslation.parallel.csv");

            using (StreamReader reader = new StreamReader(File.OpenRead(scriptPath)))
            {
                int lineNumber = 0;
                string line;
                while ((line = reader.ReadLine()) != null)
                {
                    lineNumber++;
                    ProcessScriptLine(lineNumber, line);
                }
            }
        }

        private void ProcessScriptLine(int lineNumber, string line)
        {
            // Comment/empty lines
            if (line.StartsWith("#") || string.IsNullOrWhiteSpace(line))
            {
                return;
            }

            // Clear configuration
            if (line.StartsWith("!clear"))
            {
                configStore.ClearKeys();
                Console.WriteLine("Config cleared");
                return;
            }

            // Set configuration
            var match = Regex.Match(line, @"^!set (?<key>[^=]+)=(?<value>.*)");
            if (match.Success)
            {
                string configKey = match.Groups["key"].Value;
                string configValue = match.Groups["value"].Value;
                configStore.UpdateKey(ConfigSectionName, configKey, configValue);
                Console.WriteLine("Set config: {0}={1}", configKey, configValue);
                return;
            }

            // Run test case

            var fields = line.Split(',');

            var action = (ImpactActionEnum)Enum.Parse(typeof(ImpactActionEnum), fields[0]);

            var impact = new AffectedResourceImpact()
            {
                ComputeImpact = (Impact)Enum.Parse(typeof(Impact), fields[1]),
                DiskImpact = (Impact)Enum.Parse(typeof(Impact), fields[2]),
                NetworkImpact = (Impact)Enum.Parse(typeof(Impact), fields[3]),
                OSImpact = (Impact)Enum.Parse(typeof(Impact), fields[4]),
                ApplicationConfigImpact = (Impact)Enum.Parse(typeof(Impact), fields[5]),
                EstimatedImpactDurationInSeconds = long.Parse(fields[6]),
            };

            string expectedResultString = fields[7];
            string message = string.Format("Line {0}: {1}", lineNumber, line);

            if (expectedResultString.StartsWith("!"))
            {
                AssertThrowsException<Exception>(() => translator.TranslateImpactDetailToNodeImpactLevel(action, impact), message);
            }
            else
            {
                NodeImpactLevel actualResult = translator.TranslateImpactDetailToNodeImpactLevel(action, impact);
                NodeImpactLevel expectedResult = (NodeImpactLevel)Enum.Parse(typeof(NodeImpactLevel), expectedResultString);

                Assert.AreEqual(expectedResult, actualResult, message);
            }
        }

        /// <summary>
        /// Equivalent for Verify.ThrowsException since Verify isn't supported on CoreCLR (and we don't want to add
        /// ifdef statements in code for handling this)
        /// </summary>
        /// <typeparam name="T">The type of the exception to expect.</typeparam>
        /// <param name="action">The action to execute.</param>
        /// <param name="message">The message to show on the assert if the action fails due to the expected exception.</param>
        /// <remarks>TODO, consider moving this method to a common class (in FabricIS.common).</remarks>
        private static void AssertThrowsException<T>(System.Action action, string message) where T : Exception
        {
            try
            {
                action();
                Assert.Fail(message);
            }
            catch (Exception ex)
            {
                Assert.IsInstanceOfType(ex, typeof(T), message);
            }
        }
    }
}