// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using System.Collections.Generic;
    using System.Fabric.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Linq;
    using FabricDCA;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class DcaSettingsTest
    {
        private const int NumberOfSections = 6;
        private const string TestEtlProducerSectionName = "etlProducer";
        private const string TestQueryEtlProducerSectionName = "query";
        private const string TestOperationalEtlProducerSectionName = "operational";
        private const string TestCrashDumpProducerSectionName = "crashDump";
        private const string TestPerfCtrProducerSectionName = "perfCtr";

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void EmptySettingsTest()
        {
            var settings = new List<SettingsTypeSection>();
            DcaSettings.AddDcaSettingsIfRequired(settings);
            ValidateCorrectSettings(settings);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void JustDiagnosticsSectionTest()
        {
            var settings = new List<SettingsTypeSection>
            {
                new SettingsTypeSection
                {
                    Name = Constants.SectionNames.Diagnostics,
                    Parameter = new SettingsTypeSectionParameter[0]
                }
            };
            DcaSettings.AddDcaSettingsIfRequired(settings);
            ValidateCorrectSettings(settings);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void PartialSettingsTest()
        {
            var settings = new List<SettingsTypeSection>
            {
                new SettingsTypeSection
                {
                    Name = Constants.SectionNames.Diagnostics,
                    Parameter = new[]
                    {
                        new SettingsTypeSectionParameter
                        {
                            Name = FabricValidatorConstants.ParameterNames.ProducerInstances,
                            Value = TestEtlProducerSectionName
                        }
                    }
                },
                new SettingsTypeSection
                {
                    Name = TestEtlProducerSectionName,
                    Parameter = new []
                    {
                        new SettingsTypeSectionParameter
                        {
                            Name = EtlProducerValidator.WindowsFabricEtlTypeParamName,
                            Value = EtlProducerValidator.DefaultEtl
                        },
                        new SettingsTypeSectionParameter
                        {
                            Name = FabricValidatorConstants.ParameterNames.ProducerType,
                            Value = DcaStandardPluginValidators.EtlFileProducer
                        },
                        new SettingsTypeSectionParameter
                        {
                            Name = FabricValidatorConstants.ParameterNames.IsEnabled,
                            Value = "true"
                        }
                    }
                }
            };
            DcaSettings.AddDcaSettingsIfRequired(settings);
            ValidateCorrectSettings(settings);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void PartialDisabledSettingsTest()
        {
            var settings = new List<SettingsTypeSection>
            {
                new SettingsTypeSection
                {
                    Name = Constants.SectionNames.Diagnostics,
                    Parameter = new[]
                    {
                        new SettingsTypeSectionParameter
                        {
                            Name = FabricValidatorConstants.ParameterNames.ProducerInstances,
                            Value = TestEtlProducerSectionName
                        }
                    }
                },
                new SettingsTypeSection
                {
                    Name = TestEtlProducerSectionName,
                    Parameter = new []
                    {
                        new SettingsTypeSectionParameter
                        {
                            Name = EtlProducerValidator.WindowsFabricEtlTypeParamName,
                            Value = EtlProducerValidator.DefaultEtl
                        },
                        new SettingsTypeSectionParameter
                        {
                            Name = FabricValidatorConstants.ParameterNames.ProducerType,
                            Value = DcaStandardPluginValidators.EtlFileProducer
                        },
                        new SettingsTypeSectionParameter
                        {
                            Name = FabricValidatorConstants.ParameterNames.IsEnabled,
                            Value = "false"
                        }
                    }
                }
            };
            DcaSettings.AddDcaSettingsIfRequired(settings);
            ValidateCorrectSettings(settings, 7);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void CompleteSettingsTest()
        {
            var settings = new List<SettingsTypeSection>
            {
                new SettingsTypeSection
                {
                    Name = Constants.SectionNames.Diagnostics,
                    Parameter = new[]
                    {
                        new SettingsTypeSectionParameter
                        {
                            Name = FabricValidatorConstants.ParameterNames.ProducerInstances,
                            Value = string.Join(
                                ", ",
                                TestEtlProducerSectionName,
                                TestQueryEtlProducerSectionName,
                                TestOperationalEtlProducerSectionName,
                                TestPerfCtrProducerSectionName,
                                TestCrashDumpProducerSectionName)
                        }
                    }
                },
                new SettingsTypeSection
                {
                    Name = TestEtlProducerSectionName,
                    Parameter = new []
                    {
                        new SettingsTypeSectionParameter
                        {
                            Name = EtlProducerValidator.WindowsFabricEtlTypeParamName,
                            Value = EtlProducerValidator.DefaultEtl
                        },
                        new SettingsTypeSectionParameter
                        {
                            Name = FabricValidatorConstants.ParameterNames.ProducerType,
                            Value = DcaStandardPluginValidators.EtlFileProducer
                        },
                        new SettingsTypeSectionParameter
                        {
                            Name = FabricValidatorConstants.ParameterNames.IsEnabled,
                            Value = "true"
                        }
                    }
                },
                new SettingsTypeSection
                {
                    Name = TestQueryEtlProducerSectionName,
                    Parameter = new []
                    {
                        new SettingsTypeSectionParameter
                        {
                            Name = EtlProducerValidator.WindowsFabricEtlTypeParamName,
                            Value = EtlProducerValidator.QueryEtl
                        },
                        new SettingsTypeSectionParameter
                        {
                            Name = FabricValidatorConstants.ParameterNames.ProducerType,
                            Value = DcaStandardPluginValidators.EtlFileProducer
                        }
                    }
                },
                new SettingsTypeSection
                {
                    Name = TestOperationalEtlProducerSectionName,
                    Parameter = new []
                    {
                        new SettingsTypeSectionParameter
                        {
                            Name = EtlProducerValidator.WindowsFabricEtlTypeParamName,
                            Value = EtlProducerValidator.OperationalEtl
                        },
                        new SettingsTypeSectionParameter
                        {
                            Name = FabricValidatorConstants.ParameterNames.ProducerType,
                            Value = DcaStandardPluginValidators.EtlFileProducer
                        }
                    }
                },
                new SettingsTypeSection
                {
                    Name = TestCrashDumpProducerSectionName,
                    Parameter = new []
                    {
                        new SettingsTypeSectionParameter
                        {
                            Name = FolderProducerValidator.FolderTypeParamName,
                            Value = FolderProducerValidator.ServiceFabricCrashDumps
                        },
                        new SettingsTypeSectionParameter
                        {
                            Name = FabricValidatorConstants.ParameterNames.ProducerType,
                            Value = DcaStandardPluginValidators.FolderProducer
                        },
                        new SettingsTypeSectionParameter
                        {
                            Name = FabricValidatorConstants.ParameterNames.IsEnabled,
                            Value = "true"
                        }
                    }
                },
                new SettingsTypeSection
                {
                    Name = TestPerfCtrProducerSectionName,
                    Parameter = new []
                    {
                        new SettingsTypeSectionParameter
                        {
                            Name = FolderProducerValidator.FolderTypeParamName,
                            Value = FolderProducerValidator.ServiceFabricPerformanceCounters
                        },
                        new SettingsTypeSectionParameter
                        {
                            Name = FabricValidatorConstants.ParameterNames.ProducerType,
                            Value = DcaStandardPluginValidators.FolderProducer
                        },
                        new SettingsTypeSectionParameter
                        {
                            Name = FabricValidatorConstants.ParameterNames.IsEnabled,
                            Value = "true"
                        }
                    }
                }
            };
            DcaSettings.AddDcaSettingsIfRequired(settings);
            ValidateCorrectSettings(settings);
        }

        private static void ValidateCorrectSettings(List<SettingsTypeSection> settings, int expectedNumberOfSections = NumberOfSections)
        {
            Assert.AreEqual(expectedNumberOfSections, settings.Count, "All diagnostics sections should be added.");
            Assert.IsNotNull(settings.SingleOrDefault(s => s.Name == Constants.SectionNames.Diagnostics), "Settings has Diagnostics section.");
            Assert.IsNotNull(settings.SingleOrDefault(s => s.Name == Constants.SectionNames.Diagnostics)
                ?.Parameter.SingleOrDefault(p => p.Name == FabricValidatorConstants.ParameterNames.ProducerInstances), "Settings has producer list.");
            Assert.AreEqual(expectedNumberOfSections - 1, settings.Single(s => s.Name == Constants.SectionNames.Diagnostics)
                .Parameter.Single(p => p.Name == FabricValidatorConstants.ParameterNames.ProducerInstances)
                .Value.Split(new[]{','}, StringSplitOptions.RemoveEmptyEntries).Count(), "Producer list has entries for each section.");
        }
    }
}