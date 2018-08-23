// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.Tracing;
    using System.Fabric.Dca.Validator;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.Linq;
    using System.IO;
    using System.Reflection;
    using System.Fabric.Strings;
    using System.Security;

    internal class DCASettingsValidator : ITraceWriter
    {
        public const int MinRecommendedUploadedInterval = 5;
        public const int MaxUploadConcurrencyCount = 64;

        public bool IsDCAEnabled { get; private set; }
        public bool IsLocalLogDeletionEnabled { get; private set; }
        public bool IsDCAFileStoreEnabled { get; private set; }
        public bool IsDCATableStoreEnabled { get; private set; }
        public bool IsAppLogCollectionEnabled { get; private set; }

        public SecureString DiagnosticsFileStoreConnectionString { get; private set; }
        public SecureString DiagnosticsTableStoreConnectionString { get; private set; }

        public string LogContainer { get; private set; }
        public string CrashDumpContainer { get; private set; }
        public string DiagnosticsTableStoreName { get; private set; }
        public int AppLogDirectoryQuotaInMB { get; private set; }

        public HashSet<string> DCAPluginSections { get; private set; }

        public DCASettingsValidator(
            Dictionary<string, Dictionary<string, ConfigDescription>> configurations,
            Dictionary<string, Dictionary<string, SettingsOverridesTypeSectionParameter>> settingsMap,
            string storeName)
        {
            this.configurations = configurations;
            this.settingsMap = settingsMap;
            this.atLeastOnePluginEnabled = false;
            this.storeName = storeName;

            this.localLogSettings = settingsMap.ContainsKey(FabricValidatorConstants.SectionNames.LocalLogStore) ? settingsMap[FabricValidatorConstants.SectionNames.LocalLogStore] : null;
            this.fileStoreSettings = settingsMap.ContainsKey(FabricValidatorConstants.SectionNames.DiagnosticFileStore) ? settingsMap[FabricValidatorConstants.SectionNames.DiagnosticFileStore] : null;
            this.tableStoreSettings = settingsMap.ContainsKey(FabricValidatorConstants.SectionNames.DiagnosticTableStore) ? settingsMap[FabricValidatorConstants.SectionNames.DiagnosticTableStore] : null;
            this.diagnosticsSettings = settingsMap.ContainsKey(FabricValidatorConstants.SectionNames.Diagnostics) ? settingsMap[FabricValidatorConstants.SectionNames.Diagnostics] : null;

            this.ConsumerList = new List<PluginInfo>();
            this.producerList = new List<PluginInfo>();
            this.producerInstanceNames = new List<string>();
            this.producerInstanceToType = new Dictionary<string, string>();
            this.standardConsumerTypes = new List<string>();
            this.DCAPluginSections = new HashSet<string>();
        }

        private bool V1StyleSettingsSpecified()
        {
            return (((this.localLogSettings != null) &&
                     (this.localLogSettings.ContainsKey(FabricValidatorConstants.ParameterNames.LocalLogDeletionEnabled)))
                        ||
                    ((this.fileStoreSettings != null) &&
                     (this.fileStoreSettings.ContainsKey(FabricValidatorConstants.ParameterNames.IsEnabled)))
                        ||
                    ((this.tableStoreSettings != null) &&
                     (this.tableStoreSettings.ContainsKey(FabricValidatorConstants.ParameterNames.IsEnabled))));
        }

        enum FilterComponents
        {
            TaskName = 0,
            EventType,
            Level,

            // This is not a component. It is a value that represents the total
            // count of components
            ComponentCount
        }

        private void ValidateFilterValue(Dictionary<string, SettingsOverridesTypeSectionParameter> section, string parameterName, string sectionName)
        {
            if (section.ContainsKey(parameterName))
            {
                var parameter = section[parameterName];
                List<string> taskEventPairs = new List<string>();
                bool defaultFilterFound = false;
                string[] filters = parameter.Value.Split(',');

                foreach (string filter in filters)
                {
                    if (filter.Trim().Equals(FabricValidatorConstants.DefaultTag, StringComparison.OrdinalIgnoreCase))
                    {
                        defaultFilterFound = true;
                        continue;
                    }

                    char[] filterSeparators = { '.', ':' };
                    string[] filterParts = filter.Split(filterSeparators);
                    if (filterParts.Length != ((int)FilterComponents.ComponentCount))
                    {
                        WriteError(FabricValidatorUtility.TraceTag, StringResources.Error_LogFilterNotInCorrectFormat, filter, sectionName);
                        throw new ArgumentException(string.Format(StringResources.Error_LogFilterNotInCorrectFormat, filter, sectionName));
                    }

                    string taskName = filterParts[(int)FilterComponents.TaskName].Trim();
                    string eventType = filterParts[(int)FilterComponents.EventType].Trim();
                    if ((taskName.Equals("*")) &&
                        (!eventType.Equals("*")))
                    {
                        WriteError(FabricValidatorUtility.TraceTag, StringResources.Error_LogFilterNotInCorrectFormat2, filter, sectionName);
                        throw new ArgumentException(string.Format(StringResources.Error_LogFilterNotInCorrectFormat2, filter, sectionName));
                    }

                    string taskEventPair = taskName + "." + eventType;
                    string existingPair = taskEventPairs.FirstOrDefault(s => (s.Equals(taskEventPair, StringComparison.Ordinal)));
                    if (!String.IsNullOrEmpty(existingPair))
                    {
                        WriteError(FabricValidatorUtility.TraceTag, StringResources.Error_TaskEventPairMoreThanOnce, taskEventPair, sectionName);
                        throw new ArgumentException(string.Format(StringResources.Error_TaskEventPairMoreThanOnce, taskEventPair, sectionName));
                    }
                    else
                    {
                        taskEventPairs.Add(taskEventPair);
                    }

                    try
                    {
                        int level = Int32.Parse(filterParts[(int)FilterComponents.Level], CultureInfo.InvariantCulture);
                    }
                    catch (FormatException)
                    {
                        WriteError(FabricValidatorUtility.TraceTag, StringResources.Error_LevelMustBeInteger, filter, sectionName);
                        throw new ArgumentException(string.Format(StringResources.Error_LevelMustBeInteger, filter, sectionName));
                    }
                    catch (OverflowException)
                    {
                        WriteError(FabricValidatorUtility.TraceTag, StringResources.Error_LevelInLogFilterOverflow, filter, sectionName);
                        throw new ArgumentException(string.Format(StringResources.Error_LevelInLogFilterOverflow, filter, sectionName));
                    }
                }

                if (!defaultFilterFound)
                {
                    WriteWarning(
                        FabricValidatorUtility.TraceTag,
                        StringResources.Error_NotIncludeDefaultDCAFilterString,
                        parameterName,
                        sectionName,
                        FabricValidatorConstants.DefaultTag);
                }
            }
        }

        class DefaultValueInfo
        {
            public bool IsSectionEnabled;
            public string SectionName;
            public string ParameterName;
            public string DefaultValue;
            public Dictionary<string, SettingsOverridesTypeSectionParameter> Section;
        }

        private void PerformDefaultValueChecks()
        {
            DefaultValueInfo[] defaultValueInfoList = new DefaultValueInfo[]
            {
                new DefaultValueInfo() 
                {
                    Section = this.fileStoreSettings,
                    IsSectionEnabled =  this.IsDCAFileStoreEnabled,
                    SectionName = FabricValidatorConstants.SectionNames.DiagnosticFileStore, 
                    ParameterName = FabricValidatorConstants.ParameterNames.LogContainerName, 
                    DefaultValue = FabricValidatorConstants.DefaultFileStoreTraceLocation
                },
                new DefaultValueInfo() 
                {
                    Section = this.fileStoreSettings,
                    IsSectionEnabled =  this.IsDCAFileStoreEnabled,
                    SectionName = FabricValidatorConstants.SectionNames.DiagnosticFileStore, 
                    ParameterName = FabricValidatorConstants.ParameterNames.CrashDumpContainerName, 
                    DefaultValue = FabricValidatorConstants.DefaultFileStoreCrashDumpLocation
                },
                new DefaultValueInfo() 
                {
                    Section = this.tableStoreSettings,
                    IsSectionEnabled =  this.IsDCATableStoreEnabled,
                    SectionName = FabricValidatorConstants.SectionNames.DiagnosticTableStore, 
                    ParameterName = FabricValidatorConstants.ParameterNames.TableName, 
                    DefaultValue = FabricValidatorConstants.DefaultTableStoreTableName
                }
            };

            foreach (DefaultValueInfo defaultValueInfo in defaultValueInfoList)
            {
                if (defaultValueInfo.IsSectionEnabled)
                {
                    if (!defaultValueInfo.Section.ContainsKey(defaultValueInfo.ParameterName))
                    {
                        WriteWarning(
                            FabricValidatorUtility.TraceTag,
                            "Parameter '{0}' was not specified in section '{1}'. The default value '{2}' will be used for that parameter.",
                            defaultValueInfo.ParameterName,
                            defaultValueInfo.SectionName,
                            defaultValueInfo.DefaultValue);
                    }
                }
            }
        }

        public void ValidateDCASettings()
        {
            DoWarningLevelValidation();
            ValidateDCASettingsValueTypes();
        }

        private void ValidateDCASettingsValueTypes()
        {
            this.IsAppLogCollectionEnabled = DCASettingsValidator.IsEnabled(this.fileStoreSettings, FabricValidatorConstants.ParameterNames.IsAppLogCollectionEnabled);
            if (this.IsAppLogCollectionEnabled)
            {
                this.DiagnosticsFileStoreConnectionString = this.fileStoreSettings.ContainsKey(FabricValidatorConstants.ParameterNames.StoreConnectionString) ?
                    this.fileStoreSettings[FabricValidatorConstants.ParameterNames.StoreConnectionString].GetSecureValue(this.storeName) :
                    null;

                string[] parameters = new string[] { FabricValidatorConstants.ParameterNames.UploadIntervalInMinutes };
                ValidateSpecifiedValuesInSectionAreGreaterThanZero(this.fileStoreSettings, parameters, FabricValidatorConstants.SectionNames.DiagnosticFileStore);

                this.AppLogDirectoryQuotaInMB = this.fileStoreSettings.ContainsKey(FabricValidatorConstants.ParameterNames.AppLogDirectoryQuotaInMB) ?
                    int.Parse(this.fileStoreSettings[FabricValidatorConstants.ParameterNames.AppLogDirectoryQuotaInMB].Value, CultureInfo.InvariantCulture) :
                    0;
                if (this.AppLogDirectoryQuotaInMB < 0)
                {
                    WriteError(FabricValidatorUtility.TraceTag, StringResources.Error_SectionParameterShouldBeGreaterThanZero, FabricValidatorConstants.SectionNames.DiagnosticFileStore, FabricValidatorConstants.ParameterNames.AppLogDirectoryQuotaInMB);
                    throw new ArgumentException(string.Format(StringResources.Error_SectionParameterShouldBeGreaterThanZero, FabricValidatorConstants.SectionNames.DiagnosticFileStore, FabricValidatorConstants.ParameterNames.AppLogDirectoryQuotaInMB));
                }
            }
        }

        private static void ValidateSpecifiedValuesInSectionAreGreaterThanZero(Dictionary<string, SettingsOverridesTypeSectionParameter> section, string[] parameters, string sectionName)
        {
            foreach (string parameterName in parameters)
            {
                ValueValidator.VerifyIntValueGreaterThanInput(section, parameterName, 0, sectionName, EventLevel.Error);
            }
        }

        private void DoWarningLevelValidation()
        {
            if (!this.IsDCAEnabled)
            {
                WriteWarning(FabricValidatorUtility.TraceTag, "The Fabric Data Collection Agent is disabled for this deployment.");
            }
            else
            {
                foreach (PluginInfo producer in this.producerList)
                {
                    if ((null != producer.Validator) && (!producer.Validator.Enabled))
                    {
                        WriteWarning(
                            FabricValidatorUtility.TraceTag,
                            "The Fabric Data Collection Agent is enabled, but producer {0} is not enabled.",
                            producer.Instance);
                    }
                }

                foreach (PluginInfo consumer in this.ConsumerList)
                {
                    if ((null != consumer.Validator) && (!consumer.Validator.Enabled))
                    {
                        WriteWarning(
                            FabricValidatorUtility.TraceTag,
                            "The Fabric Data Collection Agent is enabled, but consumer {0} is not enabled.",
                            consumer.Instance);
                    }
                }
            }
        }

        private static bool IsEnabled(Dictionary<string, SettingsOverridesTypeSectionParameter> section, string parameterNameForEnableFlag)
        {
            return section != null
                && section.ContainsKey(parameterNameForEnableFlag)
                && Boolean.Parse(section[parameterNameForEnableFlag].Value);
        }

        public void WriteError(string traceTag, string format, params object[] args)
        {
            FabricValidator.TraceSource.WriteError(traceTag, format, args);
        }

        public void WriteWarning(string traceTag, string format, params object[] args)
        {
            FabricValidatorUtility.WriteWarningInVSOutputFormat(traceTag, format, args);
        }

        public void WriteInfo(string traceTag, string format, params object[] args)
        {
            FabricValidator.TraceSource.WriteInfo(traceTag, format, args);
        }

        public bool ValidateDCAPluginSettings()
        {
            if (this.diagnosticsSettings == null)
            {
                return true;
            }

            bool success;
            success = ValidateProducerList();
            success = ValidateConsumerList() && success;

            foreach (string consumerType in this.standardConsumerTypes)
            {
                // Remove this section from the settings map because standard 
                // validation rules do not apply to it and custom validation has
                // already been performed.
                this.DCAPluginSections.Add(consumerType);
            }

            this.IsDCAEnabled = this.atLeastOnePluginEnabled;
            return success;
        }

        // Information about plugins
        internal class PluginInfo
        {
            internal string Instance;
            internal string AssemblyName;
            internal string TypeName;
            internal IDcaSettingsValidator Validator;
        }

        private bool ValidateNonConflictingSectionName(string referencingSection, string referencingParameter, string sectionToValidate)
        {
            if (this.configurations.ContainsKey(sectionToValidate))
            {
                WriteError(
                    FabricValidatorUtility.TraceTag,
                    "Section '{0}', parameter '{1}' references a section named '{2}', which conflicts with a well-known section name.",
                    referencingSection,
                    referencingParameter,
                    sectionToValidate);
                return false;
            }
            return true;
        }

        private bool ValidateSectionPresent(string referencingSection, string referencingParameter, string sectionToValidate)
        {
            if (!this.settingsMap.ContainsKey(sectionToValidate))
            {
                WriteError(
                    FabricValidatorUtility.TraceTag,
                    "Section '{0}', parameter '{1}' references a non-existent section named '{2}'.",
                    referencingSection,
                    referencingParameter,
                    sectionToValidate);
                return false;
            }
            return true;
        }

        private bool ValidateAllParameterNames(string sectionName, string[] supportedParameterNames)
        {
            bool success = true;
            foreach (SettingsOverridesTypeSectionParameter param in this.settingsMap[sectionName].Values)
            {
                if (null == Array.Find(supportedParameterNames, param.Name.Equals))
                {
                    WriteError(
                        FabricValidatorUtility.TraceTag,
                        "Invalid parameter name '{0}' found in section '{1}'.",
                        param.Name,
                        sectionName);
                    success = false;
                }
            }
            return success;
        }

        private bool ValidateRequiredParametersPresent(string sectionName, string[] requiredParameters)
        {
            bool success = true;
            foreach (string requiredParameter in requiredParameters)
            {
                if (!this.settingsMap[sectionName].ContainsKey(requiredParameter))
                {
                    WriteError(
                        FabricValidatorUtility.TraceTag,
                        "The required parameter '{0}' has not been specified in section '{1}'.",
                        requiredParameter,
                        sectionName);
                    success = false;
                }
            }
            return success;
        }

        private bool ValidateProducerList()
        {
            if (!this.diagnosticsSettings.ContainsKey(FabricValidatorConstants.ParameterNames.ProducerInstances))
            {
                return true;
            }

            string producerListAsString = this.diagnosticsSettings[FabricValidatorConstants.ParameterNames.ProducerInstances].Value;
            if (String.IsNullOrEmpty(producerListAsString))
            {
                WriteError(
                    FabricValidatorUtility.TraceTag,
                    "Section '{0}', parameter '{1}' value should be a non-empty string.",
                    FabricValidatorConstants.SectionNames.Diagnostics,
                    FabricValidatorConstants.ParameterNames.ProducerInstances);
                return false;
            }

            string[] producers = producerListAsString.Split(',');
            if (producers.Length == 0)
            {
                WriteError(
                    FabricValidatorUtility.TraceTag,
                    "No producer instances specified in section '{0}', parameter '{1}'.",
                    FabricValidatorConstants.SectionNames.Diagnostics,
                    FabricValidatorConstants.ParameterNames.ProducerInstances);
                return false;
            }

            bool success = true;
            foreach (string producerAsIs in producers)
            {
                string producer = producerAsIs.Trim();
                if (null != this.producerInstanceNames.Find(producer.Equals))
                {
                    WriteError(
                        FabricValidatorUtility.TraceTag,
                        "Producer instance '{0}' has been specified more than once in section '{1}', parameter '{2}'.",
                        producer,
                        FabricValidatorConstants.SectionNames.Diagnostics,
                        FabricValidatorConstants.ParameterNames.ProducerInstances);
                    success = false;
                    continue;
                }
                else
                {
                    this.producerInstanceNames.Add(producer);
                }

                if (!ValidateProducerInstance(producer))
                {
                    success = false;
                    continue;
                }
            }

            return success;
        }

        private bool ValidateProducerInstance(string producer)
        {
            if (!ValidateNonConflictingSectionName(
                     FabricValidatorConstants.SectionNames.Diagnostics,
                     FabricValidatorConstants.ParameterNames.ProducerInstances,
                     producer))
            {
                return false;
            }

            if (!ValidateSectionPresent(
                     FabricValidatorConstants.SectionNames.Diagnostics,
                     FabricValidatorConstants.ParameterNames.ProducerInstances,
                     producer))
            {
                return false;
            }

            try
            {
                if (!ValidateRequiredParametersPresent(producer, producerSectionStandardParameters))
                {
                    return false;
                }

                string producerType = this.settingsMap[producer][FabricValidatorConstants.ParameterNames.ProducerType].Value;
                producerType = producerType.Trim();

                IDcaSettingsValidator customValidator;
                if (!ValidateProducerType(producer, producerType, out customValidator))
                {
                    return false;
                }

                if (null == customValidator)
                {
                    if (!ValidateAllParameterNames(producer, producerSectionStandardParameters))
                    {
                        return false;
                    }

                    // The setting for whether or not a plugin is enabled is specified
                    // via a custom parameter. If the plugin does not have any custom
                    // parameters, we'll just assume that it is enabled.
                    this.atLeastOnePluginEnabled = true;
                    return true;
                }
                else
                {
                    return ValidateCustomParameters(producer, producerSectionStandardParameters, customValidator);
                }
            }
            finally
            {
                // Remove this section from the settings map because standard validation 
                // rules do not apply to it and custom validation has already been performed.
                this.DCAPluginSections.Add(producer);
            }
        }

        private bool ValidateProducerType(string producer, string producerType, out IDcaSettingsValidator customValidator)
        {
            customValidator = null;

            string assemblyName;
            string typeName;
            if (DcaStandardPluginValidators.ProducerValidators.ContainsKey(producerType))
            {
                assemblyName = DcaStandardPluginValidators.ProducerValidators[producerType].AssemblyName;
                typeName = DcaStandardPluginValidators.ProducerValidators[producerType].TypeName;
            }
            else
            {
                WriteError(
                    FabricValidatorUtility.TraceTag,
                    "Producer type '{0}' specified in section '{1}', parameter '{2}' is not supported.",
                    producerType,
                    producer,
                    FabricValidatorConstants.ParameterNames.ProducerType);
                return false;
            }
            
            object producerObject = CreatePluginObject(producer, assemblyName, typeName);
            if (null == producerObject)
            {
                return false;
            }

            customValidator = producerObject as IDcaSettingsValidator;
            if (null == customValidator)
            {
                WriteError(
                    FabricValidatorUtility.TraceTag,
                    "Object of type {0} in assembly {1} does not implement the IDcaSettingsValidator interface.",
                    typeName,
                    assemblyName);
                return false;
            }

            PluginInfo pluginInfo = new PluginInfo();
            pluginInfo.Instance = producer;
            pluginInfo.AssemblyName = assemblyName;
            pluginInfo.TypeName = typeName;
            pluginInfo.Validator = customValidator;
            this.producerList.Add(pluginInfo);
            this.producerInstanceToType[producer] = producerType;
            return true;
        }

        private bool ValidateConsumerList()
        {
            if (!this.diagnosticsSettings.ContainsKey(FabricValidatorConstants.ParameterNames.ConsumerInstances))
            {
                return true;
            }

            string consumerListAsString = this.diagnosticsSettings[FabricValidatorConstants.ParameterNames.ConsumerInstances].Value;
            if (String.IsNullOrEmpty(consumerListAsString))
            {
                WriteError(
                    FabricValidatorUtility.TraceTag,
                    "Section '{0}', parameter '{1}' value should be a non-empty string.",
                    FabricValidatorConstants.SectionNames.Diagnostics,
                    FabricValidatorConstants.ParameterNames.ConsumerInstances);
                return false;
            }

            string[] consumers = consumerListAsString.Split(',');
            if (consumers.Length == 0)
            {
                WriteError(
                    FabricValidatorUtility.TraceTag,
                    "No consumer instances specified in section '{0}', parameter '{1}'.",
                    FabricValidatorConstants.SectionNames.Diagnostics,
                    FabricValidatorConstants.ParameterNames.ConsumerInstances);
                return false;
            }

            bool success = true;
            List<string> consumerInstanceNames = new List<string>();
            foreach (string consumerAsIs in consumers)
            {
                string consumer = consumerAsIs.Trim();
                if (null != consumerInstanceNames.Find(consumer.Equals))
                {
                    WriteError(
                        FabricValidatorUtility.TraceTag,
                        "Consumer instance '{0}' has been specified more than once in section '{1}', parameter '{2}'.",
                        consumer,
                        FabricValidatorConstants.SectionNames.Diagnostics,
                        FabricValidatorConstants.ParameterNames.ConsumerInstances);
                    success = false;
                    continue;
                }
                else
                {
                    consumerInstanceNames.Add(consumer);
                }

                if (!ValidateConsumerInstance(consumer))
                {
                    success = false;
                    continue;
                }
            }

            return success;
        }

        private bool ValidateConsumerInstance(string consumer)
        {
            if (!ValidateNonConflictingSectionName(
                     FabricValidatorConstants.SectionNames.Diagnostics,
                     FabricValidatorConstants.ParameterNames.ConsumerInstances,
                     consumer))
            {
                return false;
            }

            if (!ValidateSectionPresent(
                     FabricValidatorConstants.SectionNames.Diagnostics,
                     FabricValidatorConstants.ParameterNames.ConsumerInstances,
                     consumer))
            {
                return false;
            }

            try
            {
                if (!ValidateRequiredParametersPresent(consumer, consumerSectionStandardParameters))
                {
                    return false;
                }

                string consumerType = this.settingsMap[consumer][FabricValidatorConstants.ParameterNames.ConsumerType].Value;
                consumerType = consumerType.Trim();

                IDcaSettingsValidator customValidator;
                if (!ValidateConsumerType(consumer, consumerType, out customValidator))
                {
                    return false;
                }

                bool consumerEnabled = false;
                if (null == customValidator)
                {
                    if (!ValidateAllParameterNames(consumer, consumerSectionStandardParameters))
                    {
                        return false;
                    }

                    // The setting for whether or not a plugin is enabled is contained
                    // in the plugin's custom section. If the plugin does not have a 
                    // custom section, we'll just assume that it is enabled.
                    consumerEnabled = true;
                    this.atLeastOnePluginEnabled = true;
                }
                else
                {
                    if (!ValidateCustomParameters(consumer, consumerSectionStandardParameters, customValidator))
                    {
                        return false;
                    }
                    consumerEnabled = customValidator.Enabled;
                }

                if (consumerEnabled)
                {
                    string producer = this.settingsMap[consumer][FabricValidatorConstants.ParameterNames.ProducerInstance].Value;
                    producer = producer.Trim();
                    if (null == this.producerInstanceNames.Find(producer.Equals))
                    {
                        WriteError(
                            FabricValidatorUtility.TraceTag,
                            "Section '{0}' referenced in section '{1}', parameter '{2}' cannot be found.",
                            producer,
                            consumer,
                            FabricValidatorConstants.ParameterNames.ProducerInstance);
                        return false;
                    }

                    if (this.producerInstanceToType.ContainsKey(producer))
                    {
                        string producerType = this.producerInstanceToType[producer];
                        if (DcaStandardPluginValidators.ProducerValidators.ContainsKey(producerType) &&
                            DcaStandardPluginValidators.ConsumerValidators.ContainsKey(consumerType))
                        {
                            if (null == Array.Find(DcaStandardPluginValidators.ProducerConsumerCompat[producerType], consumerType.Equals))
                            {
                                WriteError(
                                    FabricValidatorUtility.TraceTag,
                                    "Section '{0}', parameter '{1}' pairs a consumer of type {2} with an incompatible producer of type {3}.",
                                    consumer,
                                    FabricValidatorConstants.ParameterNames.ProducerInstance,
                                    consumerType,
                                    producerType);
                                return false;
                            }
                        }
                    }
                }
                return true;
            }
            finally
            {
                // Remove this section from the settings map because standard validation 
                // rules do not apply to it and custom validation has already been performed.
                this.DCAPluginSections.Add(consumer);
            }
        }

        private bool ValidateConsumerType(string consumer, string consumerType, out IDcaSettingsValidator customValidator)
        {
            customValidator = null;
            string assemblyName;
            string typeName;
            bool success = true;

            if (DcaStandardPluginValidators.ConsumerValidators.ContainsKey(consumerType))
            {
                assemblyName = DcaStandardPluginValidators.ConsumerValidators[consumerType].AssemblyName;
                typeName = DcaStandardPluginValidators.ConsumerValidators[consumerType].TypeName;
            }
            else
            {
                if (this.configurations.ContainsKey(consumerType))
                {
                    WriteError(
                        FabricValidatorUtility.TraceTag,
                        "Consumer type {0} referenced in section '{1}', parameter '{2}' conflicts with a well-known section name.",
                        consumerType,
                        consumer,
                        FabricValidatorConstants.ParameterNames.ConsumerType);
                    return false;
                }

                if (!this.settingsMap.ContainsKey(consumerType))
                {
                    WriteError(
                        FabricValidatorUtility.TraceTag,
                        "Consumer type {0} referenced in section '{1}', parameter '{2}' is not recognized.",
                        consumerType,
                        consumer,
                        FabricValidatorConstants.ParameterNames.ConsumerType);
                    return false;
                }

                try
                {
                    success = ValidateAllParameterNames(consumerType, consumerTypeSectionParameters) && success;

                    if (!ValidateRequiredParametersPresent(consumerType, consumerTypeSectionParameters))
                    {
                        return false;
                    }

                    assemblyName = this.settingsMap[consumerType][FabricValidatorConstants.ParameterNames.PluginValidationAssembly].Value;
                    assemblyName = assemblyName.Trim();
                    if (!assemblyName.Equals(Path.GetFileName(assemblyName)))
                    {
                        WriteError(
                            FabricValidatorUtility.TraceTag,
                            "Assembly name '{0}' in section '{1}' is not in the correct format. The assembly name should not include the path.",
                            assemblyName,
                            consumerType);
                        return false;
                    }

                    typeName = this.settingsMap[consumerType][FabricValidatorConstants.ParameterNames.PluginValidationType].Value;
                    typeName = typeName.Trim();
                }
                finally
                {
                    if (null == this.standardConsumerTypes.Find(consumerType.Equals))
                    {
                        // Add this section to the consumer types list. Later, we'll
                        // remove each member of the list from the settings map because
                        // standard validation rules do not apply to it and custom 
                        // validation has already been performed. 
                        // We don't want to remove it from the settings map right now 
                        // because other consumer instances might be of the same consumer
                        // type, so we may still come across more sections that refer to
                        // this consumer type section.
                        this.standardConsumerTypes.Add(consumerType);
                    }
                }
            }

            object consumerObject = CreatePluginObject(consumer, assemblyName, typeName);
            if (null == consumerObject)
            {
                return false;
            }

            customValidator = consumerObject as IDcaSettingsValidator;
            if (null == customValidator)
            {
                WriteError(
                    FabricValidatorUtility.TraceTag,
                    "Object of type {0} in assembly {1} does not implement the IDcaSettingsValidator interface.",
                    typeName,
                    assemblyName);
                return false;
            }

            if (success)
            {
                PluginInfo pluginInfo = new PluginInfo();
                pluginInfo.Instance = consumer;
                pluginInfo.AssemblyName = assemblyName;
                pluginInfo.TypeName = typeName;
                pluginInfo.Validator = customValidator;
                this.ConsumerList.Add(pluginInfo);
            }

            return success;
        }

        private object CreatePluginObject(string plugin, string assemblyName, string typeName)
        {
            Assembly assembly;
            try
            {
                assembly = Assembly.Load(new AssemblyName(assemblyName));
            }
            catch (Exception e)
            {
                WriteError(
                    FabricValidatorUtility.TraceTag,
                    "Exception occured while loading assembly {0} to create the plugin object for plugin {1}. Exception information: {2}.",
                    assemblyName,
                    plugin,
                    e);
                return null;
            }
            
            Type type;
            try
            {
                type = assembly.GetType(typeName);
                if (null == type)
                {
                    throw new TypeLoadException(String.Format(CultureInfo.CurrentCulture, StringResources.Error_TypeLoadAssembly,
                                                 typeName, assembly));
                }
            }
            catch (Exception e)
            {
                WriteError(
                    FabricValidatorUtility.TraceTag,
                    "Exception occured while retrieving type {0} in assembly {1} to create the validator for section {2}. Exception information: {3}.",
                    typeName,
                    assemblyName,
                    plugin,
                    e);
                return null;
            }
            
            object pluginObject;
            try
            {
                pluginObject = Activator.CreateInstance(type);
            }
            catch (Exception e)
            {
                WriteError(
                    FabricValidatorUtility.TraceTag,
                    "Exception occured while creating an object of type {0} (assembly {1}) to validate section {2}. Exception information: {3}.",
                    typeName,
                    assemblyName,
                    plugin,
                    e);
                return null;
            }

            return pluginObject;
        }

        private bool ValidateCustomParameters(string sectionName, string[] standardParameters, IDcaSettingsValidator validator)
        {
            Dictionary<string, SettingsOverridesTypeSectionParameter> section = this.settingsMap[sectionName];
            Dictionary<string, string> parameters = new Dictionary<string, string>();
            Dictionary<string, SecureString> encryptedParameters = new Dictionary<string, SecureString>();
            foreach (string paramName in section.Keys)
            {
                if (null != Array.Find(standardParameters, paramName.Equals))
                {
                    continue;
                }
                else if (section[paramName].IsEncrypted)
                {
                    encryptedParameters[paramName] = section[paramName].GetSecureValue(this.storeName);
                }
                else
                {
                    parameters[paramName] = section[paramName].Value;
                }
            }

            bool success = validator.Validate((ITraceWriter)this, sectionName, parameters, encryptedParameters);
            this.atLeastOnePluginEnabled = this.atLeastOnePluginEnabled || validator.Enabled;
            return success;
        }

        public static void UpdateDCAPluginSections(Dictionary<string, SettingsOverridesTypeSectionParameter> diagnosticsSection, HashSet<string> dcaPluginSections)
        {
            AddPluginSections(dcaPluginSections, diagnosticsSection, FabricValidatorConstants.ParameterNames.ProducerInstances);
            AddPluginSections(dcaPluginSections, diagnosticsSection, FabricValidatorConstants.ParameterNames.ConsumerInstances);
        }

        private static void AddPluginSections(HashSet<string> dcaPluginSections, Dictionary<string, SettingsOverridesTypeSectionParameter> diagnosticsSettings, string pluginListSettingName)
        {
            if (!diagnosticsSettings.ContainsKey(pluginListSettingName))
            {
                return;
            }

            string pluginListAsString = diagnosticsSettings[pluginListSettingName].Value;
            if (String.IsNullOrEmpty(pluginListAsString))
            {
                return;
            }

            string[] plugins = pluginListAsString.Split(',');
            if (plugins.Length == 0)
            {
                return;
            }

            foreach (var plugin in plugins)
            {
                string pluginSectionName = plugin.Trim();
                dcaPluginSections.Add(pluginSectionName);
            }
        }

        private bool atLeastOnePluginEnabled;
        private Dictionary<string, SettingsOverridesTypeSectionParameter> localLogSettings;
        private Dictionary<string, SettingsOverridesTypeSectionParameter> fileStoreSettings;
        private Dictionary<string, SettingsOverridesTypeSectionParameter> tableStoreSettings;
        private Dictionary<string, SettingsOverridesTypeSectionParameter> diagnosticsSettings;
        private string storeName;
        internal List<PluginInfo> ConsumerList { get; private set; }
        private List<PluginInfo> producerList;
        private List<string> producerInstanceNames;
        private Dictionary<string, string> producerInstanceToType;
        private List<string> standardConsumerTypes;
        private Dictionary<string, Dictionary<string, ConfigDescription>> configurations;
        private Dictionary<string, Dictionary<string, SettingsOverridesTypeSectionParameter>> settingsMap;
        private static string[] consumerSectionStandardParameters =
            { 
                FabricValidatorConstants.ParameterNames.ConsumerType,
                FabricValidatorConstants.ParameterNames.ProducerInstance
            };
        private static string[] consumerTypeSectionParameters =
            { 
                FabricValidatorConstants.ParameterNames.PluginAssembly,
                FabricValidatorConstants.ParameterNames.PluginType,
                FabricValidatorConstants.ParameterNames.PluginValidationAssembly,
                FabricValidatorConstants.ParameterNames.PluginValidationType
            };
        private static string[] producerSectionStandardParameters =
            { 
                FabricValidatorConstants.ParameterNames.ProducerType,
            };
    }
}