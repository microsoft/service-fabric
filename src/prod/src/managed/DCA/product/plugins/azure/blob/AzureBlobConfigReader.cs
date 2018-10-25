// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;

    internal class AzureBlobConfigReader
    {
        // Default values
        private const string AppInstanceEtwFilterPrefix = "*.*:";
        
        private readonly IConfigReader configReader;
        private readonly FabricEvents.ExtensionsEvents traceSource;
        private readonly string logSourceId;

        private readonly AzureUtility azureUtility;

        private readonly string sectionName;
        private readonly string containerParamName;
        private readonly string etwContainerParamName;
        private readonly string dataDeletionAgeParamName;
        private readonly string dataDeletionAgeTestParamName;

        public AzureBlobConfigReader(
            IConfigReader configReader, 
            string sectionName,
            FabricEvents.ExtensionsEvents traceSource,
            string logSourceId)
        {
            this.configReader = configReader;
            this.sectionName = sectionName;
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;
            this.azureUtility = new AzureUtility(traceSource, logSourceId);

            this.containerParamName = AzureConstants.ContainerParamName;
            this.etwContainerParamName = AzureConstants.ContainerParamName;
            this.dataDeletionAgeParamName = AzureConstants.DataDeletionAgeParamName;
            this.dataDeletionAgeTestParamName = AzureConstants.TestDataDeletionAgeParamName;
        }

        public bool IsReadingFromApplicationManifest
        {
            get { return this.configReader.IsReadingFromApplicationManifest; }
        }

        internal string GetApplicationLogFolder()
        {
            return this.configReader.GetApplicationLogFolder();
        }

        internal string GetApplicationType()
        {
            return this.configReader.GetApplicationType();
        }

        internal bool GetEnabled()
        {
            return this.configReader.GetUnencryptedConfigValue(
                this.sectionName,
                AzureConstants.EnabledParamName,
                AzureConstants.BlobUploadEnabledByDefault);
        }

        internal string GetContainerName()
        {
            return this.configReader.GetUnencryptedConfigValue(
                this.sectionName,
                this.containerParamName,
                AzureConstants.DefaultContainerName);
        }

        internal TimeSpan GetUploadInterval()
        {
            return TimeSpan.FromMinutes(this.configReader.GetUnencryptedConfigValue(
                this.sectionName,
                AzureConstants.UploadIntervalParamName,
                (int)AzureConstants.DefaultBlobUploadIntervalMinutes.TotalMinutes));
        }

        internal TimeSpan GetFileSyncInterval()
        {
            return TimeSpan.FromMinutes(this.configReader.GetUnencryptedConfigValue(
                this.sectionName,
                AzureConstants.FileSyncIntervalParamName,
                (int)AzureConstants.DefaultFileSyncIntervalMinutes.TotalMinutes));
        }

        internal TimeSpan GetDataDeletionAge()
        {
            var blobDeletionAge = TimeSpan.FromDays(this.configReader.GetUnencryptedConfigValue(
                this.sectionName,
                this.dataDeletionAgeParamName,
                (int)AzureConstants.DefaultDataDeletionAge.TotalDays));
            if (blobDeletionAge > AzureConstants.MaxDataDeletionAge)
            {
                this.traceSource.WriteWarning(
                    this.logSourceId,
                    "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                    blobDeletionAge,
                    this.sectionName,
                    this.dataDeletionAgeParamName,
                    AzureConstants.MaxDataDeletionAge);
                blobDeletionAge = AzureConstants.MaxDataDeletionAge;
            }

            // Check for test settings
            var logDeletionAgeTestValue = TimeSpan.FromMinutes(this.configReader.GetUnencryptedConfigValue(
                                                 this.sectionName,
                                                 this.dataDeletionAgeTestParamName,
                                                 0));
            if (logDeletionAgeTestValue != TimeSpan.Zero)
            {
                if (logDeletionAgeTestValue > AzureConstants.MaxDataDeletionAge)
                {
                    this.traceSource.WriteWarning(
                        this.logSourceId,
                        "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                        logDeletionAgeTestValue,
                        this.sectionName,
                        this.dataDeletionAgeTestParamName,
                        AzureConstants.MaxDataDeletionAge);
                    logDeletionAgeTestValue = AzureConstants.MaxDataDeletionAge;
                }

                blobDeletionAge = logDeletionAgeTestValue;
            }

            return blobDeletionAge;
        }

        internal string GetDeploymentId()
        {
            return this.configReader.GetUnencryptedConfigValue(
                this.sectionName,
                AzureConstants.DeploymentId,
                AzureConstants.DefaultDeploymentId);
        }

        internal string GetEtwTraceContainerName()
        {
            return this.configReader.GetUnencryptedConfigValue(
                this.sectionName,
                this.etwContainerParamName,
                AzureConstants.DefaultEtwTraceContainerName);
        }

        internal string GetFilters()
        {
            if (this.configReader.IsReadingFromApplicationManifest)
            {
                // Get filters from application manifest
                string levelName = this.configReader.GetUnencryptedConfigValue(
                                        this.sectionName,
                                        AzureConstants.AppEtwLevelFilterParamName,
                                        AzureConstants.DefaultAppEtwLevelFilter);
                if (false == Utility.EtwLevelNameToNumber.ContainsKey(levelName))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "The value {0} specified for section {1}, parameter {2} was not recognized. The default value {3} will be used instead.",
                        levelName,
                        this.sectionName,
                        AzureConstants.AppEtwLevelFilterParamName,
                        AzureConstants.DefaultAppEtwLevelFilter);
                    return string.Concat(
                                AppInstanceEtwFilterPrefix,
                                Utility.EtwLevelNameToNumber[AzureConstants.DefaultAppEtwLevelFilter]);
                }

                return string.Concat(
                    AppInstanceEtwFilterPrefix,
                    Utility.EtwLevelNameToNumber[levelName]);
            }

            // Get filters from cluster manifest
            return this.configReader.GetUnencryptedConfigValue(
                this.sectionName,
                AzureConstants.LogFilterParamName,
                WinFabDefaultFilter.StringRepresentation);
        }

        internal StorageAccountFactory GetStorageAccountFactory()
        {
            return this.azureUtility.GetStorageAccountFactory(
                this.configReader, 
                this.sectionName, 
                AzureConstants.ConnectionStringParamName);
        }
    }
}