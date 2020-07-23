// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;

    internal class AzureBlobEtwConfigReader
    {
        private readonly IConfigReader configReader;
        private readonly FabricEvents.ExtensionsEvents traceSource;
        private readonly string logSourceId;
        private readonly AzureUtility azureUtility;
        private readonly string sectionName;
        
        public AzureBlobEtwConfigReader(
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
                AzureConstants.ContainerParamName,
                AzureConstants.DefaultContainerName);
        }

        internal TimeSpan GetFileSyncInterval()
        {
            return TimeSpan.FromMinutes(this.configReader.GetUnencryptedConfigValue(
                this.sectionName,
                AzureConstants.FileSyncIntervalParamName,
                AzureConstants.DefaultFileSyncIntervalMinutes.TotalMinutes));
        }

        internal TimeSpan GetDataDeletionAge()
        {
            var blobDeletionAge = TimeSpan.FromDays(this.configReader.GetUnencryptedConfigValue(
                this.sectionName,
                AzureConstants.DataDeletionAgeParamName,
                (int)AzureConstants.DefaultDataDeletionAge.TotalDays));
            if (blobDeletionAge > AzureConstants.MaxDataDeletionAge)
            {
                this.traceSource.WriteWarning(
                    this.logSourceId,
                    "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                    blobDeletionAge,
                    this.sectionName,
                    AzureConstants.DataDeletionAgeParamName,
                    AzureConstants.MaxDataDeletionAge);
                blobDeletionAge = AzureConstants.MaxDataDeletionAge;
            }

            // Check for test settings
            var logDeletionAgeTestValue = TimeSpan.FromMinutes(this.configReader.GetUnencryptedConfigValue(
                                                 this.sectionName,
                                                 AzureConstants.TestDataDeletionAgeParamName,
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
                        AzureConstants.TestDataDeletionAgeParamName,
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
                AzureConstants.ContainerParamName,
                AzureConstants.DefaultEtwTraceContainerName);
        }

        internal string GetFilters()
        {
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