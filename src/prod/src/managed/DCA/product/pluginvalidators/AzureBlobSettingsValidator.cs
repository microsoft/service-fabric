// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Security;
    using System.Fabric.Dca.Validator;

    // This class implements validation for Azure blob upload settings in the cluster
    // manifest
    public abstract class ClusterManifestBlobSettingsValidator : ClusterManifestAzureSettingsValidator
    {
        protected bool ValidateUploadIntervalSetting()
        {
            const int minRecommendedUploadInterval = 5;
            int uploadInterval;
            bool result = ValidateUploadIntervalSetting(
                              AzureConstants.UploadIntervalParamName,
                              minRecommendedUploadInterval,
                              out uploadInterval);
            return result;
        }

        protected bool ValidateFileSyncIntervalSetting()
        {
            return ValidateFileSyncIntervalSetting(AzureConstants.FileSyncIntervalParamName);
        }

        protected bool UploadIntervalAbsenceHandler()
        {
            return true;
        }

        protected bool ValidateLogDeletionAgeSetting()
        {
            return ValidateLogDeletionAgeSetting(
                       AzureConstants.DataDeletionAgeParamName,
                       (int)AzureConstants.MaxDataDeletionAge.TotalDays);
        }

        protected bool ValidateLogDeletionAgeTestSetting()
        {
            return ValidateLogDeletionAgeSetting(
                       AzureConstants.TestDataDeletionAgeParamName,
                       (int)AzureConstants.MaxDataDeletionAge.TotalMinutes);
        }

        protected bool ContainerAbsenceHandler(string defaultContainerName)
        {
            WriteWarningInVSOutputFormat(
                this.logSourceId,
                "The value for setting {0} in section {1} is not specified. The default value {2} will be used.",
                AzureConstants.ContainerParamName,
                this.sectionName,
                defaultContainerName);
            return true;
        }
        
        protected bool SetUpBlobContainerValidation(string defaultContainerName)
        {
            SecureString connectionString;
            bool success = GetConnectionString(
                               AzureConstants.ConnectionStringParamName, 
                               out connectionString);
            if (success)
            {
                AzureBlobContainerInfo containerInfo;
                containerInfo.ConnectionString = connectionString;
                containerInfo.ContainerName = this.parameters.ContainsKey(AzureConstants.ContainerParamName) ? 
                                                  this.parameters[AzureConstants.ContainerParamName] :
                                                  defaultContainerName;
                this.blobContainerInfoList.Add(containerInfo);
            }
            return success;
        }
    }
}