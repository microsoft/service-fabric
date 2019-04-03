// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Linq;
    using FabricDCA;

    public static class DcaSettings
    {
        private const string DefaultEtlFileProducerSectionName = "ServiceFabricEtlFile";
        private static readonly IDictionary<string, string> DefaultEtlFileTypedProducersSectionNames = new Dictionary<string, string> {
            { EtlProducerValidator.QueryEtl, "ServiceFabricEtlFileQueryable" },
            { EtlProducerValidator.OperationalEtl, "ServiceFabricEtlFileOperational" }
        };
        private const string DefaultCrashDumpProducerSectionName = "ServiceFabricCrashDump";
        private const string DefaultPerfCounterProducerSectionName = "ServiceFabricPerfCounter";
        private const string DefaultAzureTableOperationalConsumerSectionName = "AzureTableServiceFabricEtwOperational";

        public static void AddDcaSettingsIfRequired(List<SettingsTypeSection> settings)
        {
            if (settings == null)
            {
                throw new ArgumentNullException(nameof(settings));
            }

#if !DotNetCoreClrLinux
            // This logic is specific to Windows plugins and how ETW/PerfCtr sessions are created separate from DCA.
            AddOrUpdateDiagnosticsSection(settings);
            AddProducerAndConsumerSections(settings);
#else
            AddAzureTableOperationalConsumerIfNeededLinux(settings);
#endif
        }

        private static void AddOrUpdateDiagnosticsSection(List<SettingsTypeSection> settings)
        {
            var diagnosticsSection = settings.SingleOrDefault(section => section.Name.Equals(Constants.SectionNames.Diagnostics, StringComparison.OrdinalIgnoreCase));
            if (diagnosticsSection == null)
            {
                DeployerTrace.WriteInfo($"{Constants.SectionNames.Diagnostics} section was not found and is being added.");
                diagnosticsSection = new SettingsTypeSection() { Name = Constants.SectionNames.Diagnostics, Parameter = new SettingsTypeSectionParameter[0] };
                settings.Add(diagnosticsSection);
            }

            AddOrUpdateDiagnosticsProducerInstancesParameters(diagnosticsSection);
        }

        private static void AddOrUpdateDiagnosticsProducerInstancesParameters(SettingsTypeSection diagnosticsSection)
        {
            var producers = diagnosticsSection.Parameter.SingleOrDefault(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.ProducerInstances, StringComparison.OrdinalIgnoreCase));
            if (producers == null)
            {
                DeployerTrace.WriteInfo($"{Constants.SectionNames.Diagnostics} section, {FabricValidatorConstants.ParameterNames.ProducerInstances} parameter was not found and is being added.");
                var p = new SettingsTypeSectionParameter
                {
                    Name = FabricValidatorConstants.ParameterNames.ProducerInstances,
                    Value = string.Empty
                };
                diagnosticsSection.Parameter = diagnosticsSection.Parameter.Concat(new[] { p }).ToArray();
            }
        }

        private static void AddProducerAndConsumerSections(List<SettingsTypeSection> settings)
        {
            AddEtlProducerSection(settings);
            AddTypedEtlProducersAndAzureTableConsumersSections(settings);
            AddCrashDumpProducerSection(settings);
            AddPerfCtrProducerSection(settings);
        }

        private static void AddTypedEtlProducersAndAzureTableConsumersSections(List<SettingsTypeSection> settings)
        {
            var queryableProducerSectionName = AddTypedEtlProducerSection(settings, EtlProducerValidator.QueryEtl);
            var operationalProducerSectionName = AddTypedEtlProducerSection(settings, EtlProducerValidator.OperationalEtl);

            AddAzureTableOperationalConsumerIfNeeded(settings, queryableProducerSectionName, operationalProducerSectionName);
        }

        private static string AddProducerSection(
            List<SettingsTypeSection> settings,
            Func<SettingsTypeSection, bool> matchesDesired,
            SettingsTypeSection defaultSection)
        {
            var diagnosticsSection = settings.Single(section => section.Name.Equals(Constants.SectionNames.Diagnostics, StringComparison.OrdinalIgnoreCase));

            var producerSections = diagnosticsSection.Parameter
                .Single(parameter => parameter.Name.Equals(FabricValidatorConstants.ParameterNames.ProducerInstances, StringComparison.OrdinalIgnoreCase))
                .Value.Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries).Select(pluginName => pluginName.Trim()).Select(sn => settings.Single(s => s.Name.Equals(sn, StringComparison.OrdinalIgnoreCase)));

            var producerSectionsThatExistAreEnabledAndHaveProducerTypeParam = producerSections
                .Where(s => s.Parameter.SingleOrDefault(
                                p => p.Name == FabricValidatorConstants.ParameterNames.ProducerType) != null)
                .Where(s => s.Parameter.SingleOrDefault(
                                p => p.Name == FabricValidatorConstants.ParameterNames.IsEnabled) == null ||
                            s.Parameter.Single(p => p.Name == FabricValidatorConstants.ParameterNames.IsEnabled).Value.Equals("true", StringComparison.OrdinalIgnoreCase));

            var existingSection = producerSectionsThatExistAreEnabledAndHaveProducerTypeParam.FirstOrDefault(matchesDesired);
            if (existingSection != null)
            {
                return existingSection.Name;
            }

            DeployerTrace.WriteInfo($"{defaultSection.Name} section was not found and is being added.");
            diagnosticsSection.Parameter
                    .Single(parameter => parameter.Name.Equals(
                        FabricValidatorConstants.ParameterNames.ProducerInstances, StringComparison.OrdinalIgnoreCase)).Value =
                string.Join(", ", producerSections.Select(ps => ps.Name).Union(new[] { defaultSection.Name }));

            // Remove any existing using same name
            settings.RemoveAll(s => s.Name == defaultSection.Name);
            settings.Add(defaultSection);

            return defaultSection.Name;
        }

        private static void AddEtlProducerSection(List<SettingsTypeSection> settings)
        {
            AddProducerSection(settings,
                section =>
                {
                    var pluginType = section.Parameter
                        .SingleOrDefault(p => p.Name == FabricValidatorConstants.ParameterNames.ProducerType).Value;

                    var etlType = section.Parameter
                        .SingleOrDefault(p => p.Name == EtlProducerValidator.WindowsFabricEtlTypeParamName ||
                                              p.Name == EtlProducerValidator.ServiceFabricEtlTypeParamName);

                    return pluginType != null &&
                           (pluginType == DcaStandardPluginValidators.EtlFileProducer || pluginType == DcaStandardPluginValidators.EtlInMemoryProducer) &&
                           (etlType == null || etlType.Value == EtlProducerValidator.DefaultEtl);
                },
                new SettingsTypeSection()
                {
                    Name = DefaultEtlFileProducerSectionName,
                    Parameter = new[]
                    {
                        new SettingsTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.IsEnabled, Value = "true" },
                        new SettingsTypeSectionParameter() { Name = FabricValidatorConstants.ParameterNames.ProducerType, Value = DcaStandardPluginValidators.EtlFileProducer },
                    }
                });
        }

        private static string AddTypedEtlProducerSection(List<SettingsTypeSection> settings, string producerEtlType)
        {
            var newProducerSection = new SettingsTypeSection()
            {
                Name = DefaultEtlFileTypedProducersSectionNames[producerEtlType],
                Parameter = new[]
                {
                    new SettingsTypeSectionParameter { Name = FabricValidatorConstants.ParameterNames.IsEnabled, Value = "true" },
                    new SettingsTypeSectionParameter { Name = FabricValidatorConstants.ParameterNames.ProducerType, Value = DcaStandardPluginValidators.EtlFileProducer },
                    new SettingsTypeSectionParameter { Name = EtlProducerValidator.ServiceFabricEtlTypeParamName, Value = producerEtlType }
                }
            };

            // Set EtlReadInterval to 1 minute if it is Operational producer.
            if (producerEtlType == EtlProducerValidator.OperationalEtl)
            {
                newProducerSection.Parameter = newProducerSection.Parameter.Union(
                    new[] {
                        new SettingsTypeSectionParameter {
                            Name = FabricValidatorConstants.ParameterNames.EtlReadIntervalInMinutes,
                            Value = "1" } })
                    .ToArray();
            }

            return AddProducerSection(settings,
                section =>
                {
                    var pluginType = section.Parameter
                        .SingleOrDefault(p => p.Name == FabricValidatorConstants.ParameterNames.ProducerType);

                    var etlType = section.Parameter
                        .SingleOrDefault(p => p.Name == EtlProducerValidator.WindowsFabricEtlTypeParamName ||
                                              p.Name == EtlProducerValidator.ServiceFabricEtlTypeParamName);

                    return pluginType != null &&
                           (pluginType.Value == DcaStandardPluginValidators.EtlFileProducer || pluginType.Value == DcaStandardPluginValidators.EtlInMemoryProducer) &&
                           etlType != null &&
                           etlType.Value == producerEtlType;
                },
                newProducerSection);
        }

        private static void AddCrashDumpProducerSection(List<SettingsTypeSection> settings)
        {
            AddProducerSection(settings,
                section =>
                {
                    var pluginType = section.Parameter
                        .SingleOrDefault(p => p.Name == FabricValidatorConstants.ParameterNames.ProducerType);

                    var folderType = section.Parameter
                        .SingleOrDefault(p => p.Name == FolderProducerValidator.FolderTypeParamName);

                    return pluginType != null &&
                           pluginType.Value == DcaStandardPluginValidators.FolderProducer &&
                           folderType != null &&
                           (folderType.Value == FolderProducerValidator.WindowsFabricCrashDumps ||
                            folderType.Value == FolderProducerValidator.ServiceFabricCrashDumps);
                },
                new SettingsTypeSection
                {
                    Name = DefaultCrashDumpProducerSectionName,
                    Parameter = new[]
                    {
                        new SettingsTypeSectionParameter { Name = FabricValidatorConstants.ParameterNames.IsEnabled, Value = "true" },
                        new SettingsTypeSectionParameter { Name = FabricValidatorConstants.ParameterNames.ProducerType, Value = DcaStandardPluginValidators.FolderProducer },
                        new SettingsTypeSectionParameter { Name = FolderProducerValidator.FolderTypeParamName, Value = FolderProducerValidator.ServiceFabricCrashDumps }
                    }
                });
        }

        private static void AddPerfCtrProducerSection(List<SettingsTypeSection> settings)
        {
            AddProducerSection(settings,
                section =>
                {
                    var pluginType = section.Parameter
                        .SingleOrDefault(p => p.Name == FabricValidatorConstants.ParameterNames.ProducerType);

                    var folderType = section.Parameter
                        .SingleOrDefault(p => p.Name == FolderProducerValidator.FolderTypeParamName);

                    return pluginType != null &&
                           pluginType.Value == DcaStandardPluginValidators.FolderProducer &&
                           folderType != null &&
                           (folderType.Value == FolderProducerValidator.WindowsFabricPerformanceCounters ||
                            folderType.Value == FolderProducerValidator.ServiceFabricPerformanceCounters);
                },
                new SettingsTypeSection
                {
                    Name = DefaultPerfCounterProducerSectionName,
                    Parameter = new[]
                    {
                        new SettingsTypeSectionParameter { Name = FabricValidatorConstants.ParameterNames.IsEnabled, Value = "true" },
                        new SettingsTypeSectionParameter { Name = FabricValidatorConstants.ParameterNames.ProducerType, Value = DcaStandardPluginValidators.FolderProducer },
                        new SettingsTypeSectionParameter { Name = FolderProducerValidator.FolderTypeParamName, Value = FolderProducerValidator.ServiceFabricPerformanceCounters }
                    }
                });
        }

        private static void AddAzureTableOperationalConsumerIfNeeded(List<SettingsTypeSection> settings, string queryableProducerSectionName, string operationalProducerSectionName)
        {
            if (FindCorrespondingConsumerSection(
                settings,
                operationalProducerSectionName,
                DcaStandardPluginValidators.AzureTableOperationalEventUploader) != null)
            {
                DeployerTrace.WriteInfo($"{Constants.SectionNames.Diagnostics} section, skipping the addition of {DcaStandardPluginValidators.AzureTableOperationalEventUploader} consumer as one exists.");
                return;
            }

            var queryableConsumer = FindCorrespondingConsumerSection(
                settings,
                queryableProducerSectionName,
                DcaStandardPluginValidators.AzureTableQueryableEventUploader);
            if (queryableConsumer == null ||
                !CheckSettingsSectionParameterValueEquals(
                    queryableConsumer,
                    FabricValidatorConstants.ParameterNames.IsEnabled,
                    "true"))
            {
                DeployerTrace.WriteInfo($"{Constants.SectionNames.Diagnostics} section, skipping the addition of {DcaStandardPluginValidators.AzureTableOperationalEventUploader} consumer.");
                return;
            }

            // Now as queryable consumer exists and enabled, we'll add an operational consumer with the storage settings copied from queryable one.
            var queryableConsumerStoreConnectionString = queryableConsumer.Parameter.Single(p => p.Name == FabricValidatorConstants.ParameterNames.StoreConnectionString);
            var operationalConsumer = new SettingsTypeSection
            {
                Name = DefaultAzureTableOperationalConsumerSectionName,
                Parameter = new[]
                    {
                        new SettingsTypeSectionParameter { Name = FabricValidatorConstants.ParameterNames.ConsumerType, Value = DcaStandardPluginValidators.AzureTableOperationalEventUploader },
                        new SettingsTypeSectionParameter { Name = FabricValidatorConstants.ParameterNames.IsEnabled, Value = "true" },
                        new SettingsTypeSectionParameter { Name = FabricValidatorConstants.ParameterNames.ProducerInstance, Value = operationalProducerSectionName },
                        new SettingsTypeSectionParameter {
                            Name = FabricValidatorConstants.ParameterNames.StoreConnectionString,
                            Value = queryableConsumerStoreConnectionString.Value,
                            IsEncrypted = queryableConsumerStoreConnectionString.IsEncrypted },
                        new SettingsTypeSectionParameter {
                            Name = FabricValidatorConstants.ParameterNames.TableNamePrefix,
                            Value = queryableConsumer.Parameter.Single(p => p.Name == FabricValidatorConstants.ParameterNames.TableNamePrefix).Value }
                    }
            };

            var queryableConsumerDataDeletionAgeInDays = queryableConsumer.Parameter.SingleOrDefault(p => p.Name == FabricValidatorConstants.ParameterNames.DataDeletionAgeInDays);
            if (queryableConsumerDataDeletionAgeInDays != null)
            {
                operationalConsumer.Parameter = operationalConsumer.Parameter.Union(
                    new[] {
                        new SettingsTypeSectionParameter {
                            Name = FabricValidatorConstants.ParameterNames.DataDeletionAgeInDays,
                            Value = queryableConsumerDataDeletionAgeInDays.Value } })
                    .ToArray();
            }

            var queryableConsumerDeploymentId = queryableConsumer.Parameter.SingleOrDefault(p => p.Name == FabricValidatorConstants.ParameterNames.DeploymentId);
            if (queryableConsumerDeploymentId != null)
            {
                operationalConsumer.Parameter = operationalConsumer.Parameter.Union(
                        new[] {
                        new SettingsTypeSectionParameter {
                        Name = FabricValidatorConstants.ParameterNames.DeploymentId,
                        Value = queryableConsumerDeploymentId.Value } })
                    .ToArray();
            }

            // Remove any existing using same name
            settings.RemoveAll(s => s.Name == operationalConsumer.Name);
            settings.Add(operationalConsumer);

            var diagnosticsSection = settings.Single(section => section.Name.Equals(Constants.SectionNames.Diagnostics, StringComparison.OrdinalIgnoreCase));
            var consumersParam = diagnosticsSection.Parameter.Single(p => p.Name.Equals(FabricValidatorConstants.ParameterNames.ConsumerInstances, StringComparison.OrdinalIgnoreCase));
            var consumers = consumersParam.Value.Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries).Select(pluginName => pluginName.Trim());
            consumersParam.Value = string.Join(", ", consumers.Union(new string[] { operationalConsumer.Name }));
        }

        private static SettingsTypeSection FindCorrespondingConsumerSection(List<SettingsTypeSection> settings, string producerSectionName, string consumerType)
        {
            var consumerSection = settings.FirstOrDefault(
                s => s.Parameter.Any(
                    p => p.Name == FabricValidatorConstants.ParameterNames.ProducerInstance && p.Value.Equals(producerSectionName, StringComparison.OrdinalIgnoreCase)));

            if (consumerSection == null ||
                !CheckSettingsSectionParameterValueEquals(
                    consumerSection,
                    FabricValidatorConstants.ParameterNames.ConsumerType,
                    consumerType))
            {
                return null;
            }

            var diagnosticsSection = settings.Single(section => section.Name.Equals(Constants.SectionNames.Diagnostics, StringComparison.OrdinalIgnoreCase));
            var consumersParam = diagnosticsSection.Parameter.SingleOrDefault(p => p.Name.Equals(FabricValidatorConstants.ParameterNames.ConsumerInstances, StringComparison.OrdinalIgnoreCase));
            if (consumersParam == null ||
                !consumersParam.Value.Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries)
                    .Select(pluginName => pluginName.Trim())
                    .Any(name => name.Equals(consumerSection.Name, StringComparison.OrdinalIgnoreCase)))
            {
                return null;
            }

            return consumerSection;
        }

#if DotNetCoreClrLinux
        // Adds in case Queryable Table uploader is present
        private static void AddAzureTableOperationalConsumerIfNeededLinux(List<SettingsTypeSection> settings)
        {
            // TODO - Following code will be removed once fully transitioned to structured traces in Linux
            bool isStructuredTracesEnabled = GetIsStructuredTracesEnabled(settings);
            if (isStructuredTracesEnabled == false)
            {
                return;
            }

            SettingsTypeSection operationalConsumer = CreateOperationalTableUploaderSectionFromQueryableTable(settings);
            if (operationalConsumer == null)
            {
                return;
            }

            SettingsTypeSection diagnosticsSection = GetSectionWithName(Constants.SectionNames.Diagnostics, settings);
            if (diagnosticsSection == null)
            {
                return;
            }

            SettingsTypeSectionParameter consumersParam = GetParameterWithName(FabricValidatorConstants.ParameterNames.ConsumerInstances, diagnosticsSection);
            if (consumersParam == null)
            {
                return;
            }

            // Updating ConsumerInstances in Diagnostics Section
            var consumers = consumersParam.Value.Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries).Select(pluginName => pluginName.Trim());
            consumersParam.Value = string.Join(", ", consumers.Union(new string[] { operationalConsumer.Name }));

            // Adding operational table uploader Section
            settings.Add(operationalConsumer);
        }

        private static SettingsTypeSection CreateOperationalTableUploaderSectionFromQueryableTable(List<SettingsTypeSection> settings)
        {
            SettingsTypeSection azureTableQueryableCsvUploaderConsumerSection = GetFirstConsumerOfType(settings, DcaStandardPluginValidators.AzureTableQueryableCsvUploader);
            SettingsTypeSection azureTableOperationalEventUploaderConsumerSection = GetFirstConsumerOfType(settings, DcaStandardPluginValidators.AzureTableOperationalEventUploader);

            // don't add in case there is no queryable table uploader or there is an existing operational table uploader defined.
            if (azureTableQueryableCsvUploaderConsumerSection == null ||
                azureTableOperationalEventUploaderConsumerSection != null)
            {
                return null;
            }

            SettingsTypeSectionParameter queryableConsumerTableNamePrefix = GetParameterWithName(FabricValidatorConstants.ParameterNames.TableNamePrefix, azureTableQueryableCsvUploaderConsumerSection);
            SettingsTypeSectionParameter queryableConsumerStoreConnectionString = GetParameterWithName(FabricValidatorConstants.ParameterNames.StoreConnectionString, azureTableQueryableCsvUploaderConsumerSection);
            SettingsTypeSectionParameter queryableConsumerProducerInstance = GetParameterWithName(FabricValidatorConstants.ParameterNames.ProducerInstance, azureTableQueryableCsvUploaderConsumerSection);

            if (queryableConsumerTableNamePrefix == null ||
                queryableConsumerStoreConnectionString == null ||
                queryableConsumerProducerInstance == null)
            {
                DeployerTrace.WriteError($"Unable to create Operational trace from Queryable Table Uploader. Not all required parameters defined.\n" +
                    "{FabricValidatorConstants.ParameterNames.ProducerInstance} : {queryableConsumerProducerInstance != null}\n" +
                    "{FabricValidatorConstants.ParameterNames.StoreConnectionString} : {queryableConsumerStoreConnectionString != null}\n" +
                    "{FabricValidatorConstants.ParameterNames.TableNamePrefix} : {QueryableConsumerTableNamePrefix != null}");
                return null;
            }

            var operationalProducerSectionValue = queryableConsumerProducerInstance.Value;

            var operationalConsumer = new SettingsTypeSection
            {
                Name = DefaultAzureTableOperationalConsumerSectionName,
                Parameter = new[]
                    {
                        new SettingsTypeSectionParameter { Name = FabricValidatorConstants.ParameterNames.ConsumerType, Value = DcaStandardPluginValidators.AzureTableOperationalEventUploader },
                        new SettingsTypeSectionParameter { Name = FabricValidatorConstants.ParameterNames.IsEnabled, Value = "true" },
                        new SettingsTypeSectionParameter { Name = FabricValidatorConstants.ParameterNames.ProducerInstance, Value = operationalProducerSectionValue },
                        new SettingsTypeSectionParameter {
                            Name = FabricValidatorConstants.ParameterNames.StoreConnectionString,
                            Value = queryableConsumerStoreConnectionString.Value,
                            IsEncrypted = queryableConsumerStoreConnectionString.IsEncrypted },
                        new SettingsTypeSectionParameter {
                            Name = FabricValidatorConstants.ParameterNames.TableNamePrefix,
                            Value = queryableConsumerTableNamePrefix.Value }
                    }
            };

            var queryableConsumerDataDeletionAgeInDays = GetParameterWithName(FabricValidatorConstants.ParameterNames.DataDeletionAgeInDays, azureTableQueryableCsvUploaderConsumerSection);
            if (queryableConsumerDataDeletionAgeInDays != null)
            {
                operationalConsumer.Parameter = operationalConsumer.Parameter.Union(
                    new[] {
                        new SettingsTypeSectionParameter {
                            Name = FabricValidatorConstants.ParameterNames.DataDeletionAgeInDays,
                            Value = queryableConsumerDataDeletionAgeInDays.Value } })
                    .ToArray();
            }

            var queryableConsumerDeploymentId = GetParameterWithName(FabricValidatorConstants.ParameterNames.DeploymentId, azureTableQueryableCsvUploaderConsumerSection);
            if (queryableConsumerDeploymentId != null)
            {
                operationalConsumer.Parameter = operationalConsumer.Parameter.Union(
                    new[] {
                        new SettingsTypeSectionParameter {
                            Name = FabricValidatorConstants.ParameterNames.DeploymentId,
                            Value = queryableConsumerDeploymentId.Value } })
                    .ToArray();
            }

            return operationalConsumer;
        }

        private static SettingsTypeSection GetFirstConsumerOfType(List<SettingsTypeSection> settings, string consumerTypeName)
        {
            return settings.FirstOrDefault(
                s => s.Parameter.Any(
                    p => p.Name == FabricValidatorConstants.ParameterNames.ConsumerType &&
                         p.Value == consumerTypeName));
        }

        private static SettingsTypeSection GetSectionWithName(string sectionName, List<SettingsTypeSection> settings)
        {
            return settings.SingleOrDefault(section => section.Name.Equals(sectionName, StringComparison.OrdinalIgnoreCase));
        }

        private static SettingsTypeSectionParameter GetParameterWithName(string parameterName, SettingsTypeSection section)
        {
            return section.Parameter.FirstOrDefault(p => p.Name.Equals(parameterName, StringComparison.OrdinalIgnoreCase));
        }

        // TODO - Following code will be removed once fully transitioned to structured traces in Linux
        private static bool GetIsStructuredTracesEnabled(List<SettingsTypeSection> settings)
        {
            SettingsTypeSection commonSection = GetSectionWithName(FabricValidatorConstants.SectionNames.Common, settings);
            if (commonSection == null)
            {
                return false;
            }

            return CheckSettingsSectionParameterValueEquals(commonSection, FabricValidatorConstants.ParameterNames.Common.LinuxStructuredTracesEnabled, "true");
        }
#endif

        private static bool CheckSettingsSectionParameterValueEquals(SettingsTypeSection section, string parameterName, string value)
        {
            return section != null &&
                section.Parameter.SingleOrDefault(p => p.Name == parameterName) != null &&
                section.Parameter.Single(p => p.Name == parameterName).Value.Trim().Equals(value, StringComparison.OrdinalIgnoreCase);
        }
    }
}
