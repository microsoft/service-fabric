// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Fabric.Description;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.WindowsAzure.ServiceRuntime.Management;
using WEX.TestExecution;
using IImpactDetail = Microsoft.WindowsAzure.ServiceRuntime.Management.Public.IImpactDetail;
using ResourceImpactEnum = Microsoft.WindowsAzure.ServiceRuntime.Management.Public.ResourceImpactEnum;

namespace System.Fabric.InfrastructureService.Test
{
    [TestClass]
    public class ImpactTranslatorTest
    {
        class MockImpactDetail : IImpactDetail
        {
            public ResourceImpactEnum ComputeImpact { get; set; }
            public ResourceImpactEnum DiskImpact { get; set; }
            public long EstimatedImpactDurationInSeconds { get; set; }
            public ResourceImpactEnum NetworkImpact { get; set; }
            public ResourceImpactEnum OsImpact { get; set; }
        }

        private static readonly TraceType TraceType = new TraceType("ImpactTranslatorTest");
        private const string ConfigSectionName = "ISConfig";

        private MockConfigStore configStore;
        private ImpactTranslator translator;

        [TestInitialize]
        public void TestInitialize()
        {
            this.configStore = new MockConfigStore();
            this.translator = new ImpactTranslator(new ConfigSection(TraceType, this.configStore, ConfigSectionName));
        }

        [TestMethod]
        public void ImpactToString()
        {
            var impact = new MockImpactDetail()
            {
                ComputeImpact = ResourceImpactEnum.None,
                DiskImpact = ResourceImpactEnum.Freeze,
                NetworkImpact = ResourceImpactEnum.Reset,
                OsImpact = ResourceImpactEnum.Unknown,
                EstimatedImpactDurationInSeconds = 42,
            };

            var instance = new ImpactedInstance(
                "Role_IN_42",
                new[] { ImpactReason.ApplicationUpdate, ImpactReason.OSUpdate },
                impact);

            Verify.AreEqual(
                "Role_IN_42[ApplicationUpdate,OSUpdate][Compute=None,Disk=Freeze,Network=Reset,OS=Unknown,Duration=42]",
                instance.ToString());

            instance = new ImpactedInstance(
                "Role_IN_42",
                new[] { ImpactReason.ApplicationUpdate, ImpactReason.OSUpdate },
                null);

            Verify.AreEqual(
                "Role_IN_42[ApplicationUpdate,OSUpdate]",
                instance.ToString());

            instance = new ImpactedInstance(
                "Role_IN_42",
                new ImpactReason[0],
                impact);

            Verify.AreEqual(
                "Role_IN_42[][Compute=None,Disk=Freeze,Network=Reset,OS=Unknown,Duration=42]",
                instance.ToString());
        }

        [TestMethod]
        public void UseImpactDetail()
        {
            Verify.IsFalse(translator.UseImpactDetail(JobType.PlatformUpdateJob), "PlatformUpdateJob should not use impact detail");
            Verify.IsFalse(translator.UseImpactDetail(JobType.DeploymentUpdateJob), "DeploymentUpdateJob should not use impact detail");

            configStore.UpdateKey(
                ConfigSectionName,
                string.Format(ImpactTranslator.ConfigKeyFormatUseImpactDetail, JobType.PlatformUpdateJob),
                true.ToString());

            Verify.IsTrue(translator.UseImpactDetail(JobType.PlatformUpdateJob), "PlatformUpdateJob should use impact detail");
            Verify.IsFalse(translator.UseImpactDetail(JobType.DeploymentUpdateJob), "DeploymentUpdateJob should not use impact detail");

            configStore.UpdateKey(
                ConfigSectionName,
                string.Format(ImpactTranslator.ConfigKeyFormatUseImpactDetail, JobType.PlatformUpdateJob),
                false.ToString());

            configStore.UpdateKey(
                ConfigSectionName,
                string.Format(ImpactTranslator.ConfigKeyFormatUseImpactDetail, JobType.DeploymentUpdateJob),
                true.ToString());

            Verify.IsFalse(translator.UseImpactDetail(JobType.PlatformUpdateJob), "PlatformUpdateJob should not use impact detail");
            Verify.IsTrue(translator.UseImpactDetail(JobType.DeploymentUpdateJob), "DeploymentUpdateJob should use impact detail");
        }

        [TestMethod]
        public void Restart()
        {
            var impact = new MockImpactDetail()
            {
                ComputeImpact = ResourceImpactEnum.Reset,
                DiskImpact = ResourceImpactEnum.None,
                NetworkImpact = ResourceImpactEnum.None,
                OsImpact = ResourceImpactEnum.None,
                EstimatedImpactDurationInSeconds = -1,
            };
            Verify.AreEqual(NodeTask.Restart, translator.TranslateImpactDetailToNodeTask(impact));

            impact.EstimatedImpactDurationInSeconds = 0;
            Verify.AreEqual(NodeTask.Restart, translator.TranslateImpactDetailToNodeTask(impact));

            impact.EstimatedImpactDurationInSeconds = 10;
            Verify.AreEqual(NodeTask.Restart, translator.TranslateImpactDetailToNodeTask(impact));

            configStore.UpdateKey(
                ConfigSectionName,
                ImpactTranslator.ConfigKeyRestartDurationThreshold,
                "10");

            impact.EstimatedImpactDurationInSeconds = -1;
            Verify.AreEqual(NodeTask.Restart, translator.TranslateImpactDetailToNodeTask(impact));

            impact.EstimatedImpactDurationInSeconds = 0;
            Verify.AreEqual(NodeTask.Restart, translator.TranslateImpactDetailToNodeTask(impact));

            impact.EstimatedImpactDurationInSeconds = 9;
            Verify.AreEqual(NodeTask.Restart, translator.TranslateImpactDetailToNodeTask(impact));

            impact.EstimatedImpactDurationInSeconds = 10;
            Verify.AreEqual(NodeTask.Restart, translator.TranslateImpactDetailToNodeTask(impact));

            impact.EstimatedImpactDurationInSeconds = long.MaxValue;
            Verify.AreEqual(NodeTask.Relocate, translator.TranslateImpactDetailToNodeTask(impact));
        }

        [TestMethod]
        public void ScriptedTest()
        {
            string scriptPath = Path.Combine(
                Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
                "ImpactTranslation.csv");

            using (StreamReader reader = new StreamReader(scriptPath))
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

            var impact = new MockImpactDetail()
            {
                ComputeImpact = (ResourceImpactEnum)Enum.Parse(typeof(ResourceImpactEnum), fields[0]),
                DiskImpact = (ResourceImpactEnum)Enum.Parse(typeof(ResourceImpactEnum), fields[1]),
                NetworkImpact = (ResourceImpactEnum)Enum.Parse(typeof(ResourceImpactEnum), fields[2]),
                OsImpact = (ResourceImpactEnum)Enum.Parse(typeof(ResourceImpactEnum), fields[3]),
                EstimatedImpactDurationInSeconds = long.Parse(fields[4]),
            };

            string expectedResultString = fields[5];
            string message = string.Format("Line {0}: {1}", lineNumber, line);

            if (expectedResultString.StartsWith("!"))
            {
                Verify.Throws<Exception>(() => translator.TranslateImpactDetailToNodeTask(impact), message);
            }
            else
            {
                NodeTask actualResult = translator.TranslateImpactDetailToNodeTask(impact);
                NodeTask expectedResult = (NodeTask)Enum.Parse(typeof(NodeTask), expectedResultString);

                Verify.AreEqual(expectedResult, actualResult, message);
            }
        }
    }
}