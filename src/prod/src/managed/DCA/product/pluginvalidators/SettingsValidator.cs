// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Globalization;
    using System.Linq;
    using System.Security;
    using System.Fabric.Dca.Validator;

    // This class implements validation for DCA consumer settings in the cluster
    // manifest
    public abstract class ClusterManifestSettingsValidator
    {
        // Constants
        protected const string defaultTraceType = "FabricDeployer";

        // Object used for tracing
        protected ITraceWriter traceSource;

        // Tag used to represent the source of the log message
        protected string logSourceId;
        
        // Method that validates a setting in the cluster manifest
        protected delegate bool SettingValidator();

        // Method that handles the absence of a setting from the cluster manifest
        protected delegate bool SettingAbsenceHandler();

        // Handler methods for cluster manifest settings
        protected class Handler
        {
            internal SettingValidator Validator;
            internal SettingAbsenceHandler AbsenceHandler;

            internal Handler()
            {
                this.Validator = null;
                this.AbsenceHandler = null;
            }
        }

        // Name of the section in the cluster manifest that contains these settings
        protected string sectionName;

        // Unencrypted parameters in the cluster manifest and their values
        protected Dictionary<string, string> parameters;

        // Encrypted parameters in the cluster manifest and their values
        protected Dictionary<string, SecureString> encryptedParameters;

        // Dictionary of handler methods for the cluster manifest settings that
        // are of interest to us
        protected Dictionary<string, Handler> settingHandlers;
        protected Dictionary<string, Handler> encryptedSettingHandlers;
        
        public bool Enabled { get; private set; }

        abstract protected string isEnabledParamName { get; }

        protected void WriteWarningInVSOutputFormat(string type, string format, params object[] args)
        {
            this.traceSource.WriteWarning(
                type,
                "DeploymentValidator: warning: " + format,
                args);
            return;
        }

        public ClusterManifestSettingsValidator()
        {
            this.logSourceId = defaultTraceType;
            return;
        }

        public virtual bool Validate(
                                ITraceWriter traceEventSource,
                                string sectionName,
                                Dictionary<string, string> parameters, 
                                Dictionary<string, SecureString> encryptedParameters)
        {
            this.traceSource = traceEventSource;
            this.logSourceId = sectionName;
            this.sectionName = sectionName;
            this.parameters = parameters;
            this.encryptedParameters = encryptedParameters;

            // First check if the consumer is enabled
            bool success = this.ValidateEnabledSetting(this.isEnabledParamName);
            if (success && (false == this.Enabled))
            {
                // The consumer is not enabled, no further checking is necessary
                return true;
            }

            // Look for the known settings and validate them
            foreach (string paramName in this.settingHandlers.Keys)
            {
                if (this.parameters.ContainsKey(paramName))
                {
                    // Setting is present in the cluster manifest. Validate its value.
                    if (null != this.settingHandlers[paramName].Validator)
                    {
                        success = this.settingHandlers[paramName].Validator() && success;
                    }
                }
                else
                {
                    // Setting is not present in the cluster manifest
                    if (null != this.settingHandlers[paramName].AbsenceHandler)
                    {
                        success = this.settingHandlers[paramName].AbsenceHandler() && success;
                    }
                }
            }

            foreach (string paramName in this.encryptedSettingHandlers.Keys)
            {
                if (this.encryptedParameters.ContainsKey(paramName))
                {
                    // Setting is present in the cluster manifest. Validate its value.
                    if (null != this.encryptedSettingHandlers[paramName].Validator)
                    {
                        success = this.encryptedSettingHandlers[paramName].Validator() && success;
                    }
                }
                else
                {
                    // Setting is not present in the cluster manifest
                    if (null != this.encryptedSettingHandlers[paramName].AbsenceHandler)
                    {
                        success = this.encryptedSettingHandlers[paramName].AbsenceHandler() && success;
                    }
                }
            }

            // Make sure the cluster manifest does not contain any unknown settings
            foreach (string paramName in this.parameters.Keys)
            {
                if ((false == this.settingHandlers.ContainsKey(paramName)) &&
                    (false == this.isEnabledParamName.Equals(paramName)))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Setting {0} in section {1} is not recognized.",
                        paramName,
                        this.sectionName);
                    success = false;
                    continue;
                }
            }

            foreach (string paramName in this.encryptedParameters.Keys)
            {
                if (false == this.encryptedSettingHandlers.ContainsKey(paramName))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Setting {0} in section {1} is not recognized.",
                        paramName,
                        this.sectionName);
                    success = false;
                    continue;
                }
            }

            return success;
        }

        [SuppressMessage("Microsoft.Design", "CA1045:DoNotPassTypesByReference",
            Justification = "The benefit from this change is not worth the code churn caused. According to MSDN it is safe to suppress this.")]
        protected bool ConvertFromString<T>(string paramName, string value, ref T convertedValue)
        {
            try
            {
                convertedValue = (T) Convert.ChangeType(
                                         value, 
                                         typeof(T), 
                                         CultureInfo.InvariantCulture);
            }
            catch (Exception e)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Value {0} for setting {1} in section {2} cannot be converted to {3}. Exception information: {4}",
                    value,
                    paramName,
                    this.sectionName,
                    typeof(T),
                    e);
                return false;
            }

            return true;
        }

        protected bool ValidatePostiveIntegerValue(string paramName, string value, out int convertedValue)
        {
            convertedValue = 0;
            if (false == this.ConvertFromString(paramName, value, ref convertedValue))
            {
                return false;
            }

            if (convertedValue <= 0)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "{0} is an invalid value for setting {1} in section {2}. The value should be greater than 0.",
                    convertedValue,
                    paramName,
                    this.sectionName);
                return false;
            }
            
            return true;
        }

        protected bool ValidateNonNegativeIntegerValue(string paramName, string value, out int convertedValue)
        {
            convertedValue = 0;
            if (false == this.ConvertFromString(paramName, value, ref convertedValue))
            {
                return false;
            }

            if (convertedValue < 0)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "{0} is an invalid value for setting {1} in section {2}. The value should be greater than or equal to 0.",
                    convertedValue,
                    paramName,
                    this.sectionName);
                return false;
            }
            
            return true;
        }

        private bool ValidateEnabledSetting(string paramName)
        {
            if (false == this.parameters.ContainsKey(paramName))
            {
                this.Enabled = false;
                return true;
            }
            
            string enabledAsString = this.parameters[paramName];
            bool enabled = false;
            if (false == this.ConvertFromString(
                             paramName, 
                             enabledAsString, 
                             ref enabled))
            {
                return false;
            }

            this.Enabled = enabled;
            return true;
        }

        protected bool ValidateUploadIntervalSetting(string paramName, int minRecommendedUploadInterval, out int uploadInterval)
        {
            string uploadIntervalAsString = this.parameters[paramName];
            if (false == this.ValidatePostiveIntegerValue(
                             paramName,
                             uploadIntervalAsString,
                             out uploadInterval))
            {
                return false;
            }

            if (minRecommendedUploadInterval > uploadInterval)
            {
                this.WriteWarningInVSOutputFormat(
                    this.logSourceId,
                    "The value for setting {0} in section {1} is currently specified as {2} minutes. The minimum recommended value {3} minutes.",
                    paramName,
                    this.sectionName,
                    uploadInterval,
                    minRecommendedUploadInterval);
            }

            return true;
        }

        protected bool ValidateUploadIntervalSetting(string paramName, int minRecommendedUploadInterval)
        {
            int uploadIntervalNotUsed;
            return ValidateUploadIntervalSetting(
                       paramName, 
                       minRecommendedUploadInterval, 
                       out uploadIntervalNotUsed);
        }

        protected bool ValidateFileSyncIntervalSetting(string paramName)
        {
            string fileSyncIntervalAsString = this.parameters[paramName];
            int fileSyncInterval;
            if (false == ValidatePostiveIntegerValue(
                             paramName,
                             fileSyncIntervalAsString,
                             out fileSyncInterval))
            {
                return false;
            }
            const int minRecommendedFileSyncInterval = 30;
            if (minRecommendedFileSyncInterval > fileSyncInterval)
            {
                WriteWarningInVSOutputFormat(
                    this.logSourceId,
                    "The value for setting {0} in section {1} is currently specified as {2} minutes. The minimum recommended value {3} minutes.",
                    paramName,
                    this.sectionName,
                    fileSyncInterval,
                    minRecommendedFileSyncInterval);
            }
            return true;
        }

        protected bool ValidateLogDeletionAgeSetting(string paramName, int maxIntegerValue)
        {
            string deletionAgeAsString = this.parameters[paramName];
            int deletionAge;
            if (false == this.ValidatePostiveIntegerValue(
                             paramName,
                             deletionAgeAsString, 
                             out deletionAge))
            {
                return false;
            }

            if (deletionAge > maxIntegerValue)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}.",
                    deletionAge,
                    this.sectionName,
                    paramName,
                    maxIntegerValue);
                return false;
            }

            return true;
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

        protected bool ValidateLogFilterSetting(string paramName)
        {
            bool valid = true;
            string filterAsString = this.parameters[paramName];
            List<string> taskEventPairs = new List<string>();
            bool defaultFilterFound = false;
            bool summaryFilterFound = false;
            bool allInfoFilterFound = false;
            string[] filters = filterAsString.Split(',');
            const string defaultFilterTag = "_default_";
            const string summaryFilterTag = "_summary_";

            foreach (string filter in filters)
            {
                string trimmedFilter = filter.Trim();
                if (trimmedFilter.Equals(defaultFilterTag, StringComparison.OrdinalIgnoreCase))
                {
                    defaultFilterFound = true;
                    continue;
                }
                else if (trimmedFilter.Equals(summaryFilterTag, StringComparison.OrdinalIgnoreCase))
                {
                    summaryFilterFound = true;
                    continue;
                }

                char[] filterSeparators = { '.', ':' };
                string[] filterParts = filter.Split(filterSeparators);
                if (filterParts.Length != ((int)FilterComponents.ComponentCount))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "The log filter '{0}' in the '{1}' section is not in the correct format. The correct format is <TaskName>.<EventType>:<Level>. Multiple filters should be comma separated.",
                        filter,
                        this.sectionName);
                    valid = false;
                    continue;
                }

                string taskName = filterParts[(int)FilterComponents.TaskName].Trim();
                string eventType = filterParts[(int)FilterComponents.EventType].Trim();
                if (taskName.Equals("*") && (false == eventType.Equals("*")))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "The log filter '{0}' in the '{1}' section is not in the correct format. The format *.<EventType>:<Level> is not supported.",
                        filter,
                        this.sectionName);
                    valid = false;
                    continue;
                }

                string taskEventPair = taskName + "." + eventType;
                string existingPair = taskEventPairs.FirstOrDefault(s => (s.Equals(taskEventPair, StringComparison.Ordinal)));
                if (false == string.IsNullOrEmpty(existingPair))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "The task/event pair '{0}' appears more than once in the list of filters in section {1}. A given task/event pair can appear no more than once in the filter list.",
                        taskEventPair,
                        this.sectionName);
                    valid = false;
                    continue;
                }
                else
                {
                    taskEventPairs.Add(taskEventPair);
                }

                int level;
                try
                {
                    level = int.Parse(filterParts[(int)FilterComponents.Level], CultureInfo.InvariantCulture);
                }
                catch (FormatException)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "The level in the log filter '{0}' in the '{1}' section must be an integer.",
                        filter,
                        this.sectionName);
                    valid = false;
                    continue;
                }
                catch (OverflowException)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Overflow: The level in the log filter '{0}' in the '{1}' section cannot be represented by a 32-bit integer.",
                        filter,
                        this.sectionName);
                    valid = false;
                    continue;
                }

                if (taskName.Equals("*") && eventType.Equals("*") && (level == 4))
                {
                    allInfoFilterFound = true;
                }
            }

            if (!(defaultFilterFound || summaryFilterFound || allInfoFilterFound))
            {
                this.WriteWarningInVSOutputFormat(
                    this.logSourceId,
                    "The '{0}' parameter in the '{1}' section does not match the commonly-used filters. Therefore, please ensure that traces captured by the specified filter are sufficient for your debugging.",
                    paramName,
                    this.sectionName,
                    defaultFilterTag);
            }

            return valid;
        }

        protected bool ValidateFlushOnDisposeSetting(string paramName)
        {
            bool success = true;
            bool flushOnDispose = false;
            string flushOnDisposeAsString = this.parameters[paramName];
            if (false == ConvertFromString(
                             paramName,
                             flushOnDisposeAsString,
                             ref flushOnDispose))
            {
                success = false;
            }
            return success;
        }
    }
}