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
    
    // This class implements validation for blob upload settings for upload of 
    // CSV files in the cluster manifest
    public class AzureBlobCsvUploaderValidator : ClusterManifestBlobSettingsValidator, IDcaAzureSettingsValidator
    {
        protected override string isEnabledParamName
        {
            get
            {
                return AzureConstants.EnabledParamName;
            }
        }

        public AzureBlobCsvUploaderValidator()
        {
            this.settingHandlers = new Dictionary<string, Handler>()
                                   {
                                       {
                                           AzureConstants.ConnectionStringParamName,
                                           new Handler()
                                           {
                                               Validator = this.ConnectionStringHandler
                                           }
                                       },
                                       {
                                           AzureConstants.ContainerParamName,
                                           new Handler()
                                           {
                                               AbsenceHandler = this.TraceContainerAbsenceHandler
                                           }
                                       },
                                       {
                                           AzureConstants.UploadIntervalParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateUploadIntervalSetting,
                                               AbsenceHandler = this.UploadIntervalAbsenceHandler
                                           }
                                       },
                                       {
                                           AzureConstants.FileSyncIntervalParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateFileSyncIntervalSetting
                                           }
                                       },
                                       {
                                           AzureConstants.DataDeletionAgeParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateLogDeletionAgeSetting
                                           }
                                       },
                                       {
                                           AzureConstants.TestDataDeletionAgeParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateLogDeletionAgeTestSetting
                                           }
                                       },
                                       {
                                           AzureConstants.LogFilterParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateLogFilterSetting
                                           }
                                       },
                                       {
                                           AzureConstants.DeploymentId,
                                           new Handler()
                                           {
                                               Validator = this.DeploymentIdHandler
                                           }
                                       },
                                   };
            this.encryptedSettingHandlers = new Dictionary<string, Handler>()
                                            {
                                                {
                                                    AzureConstants.ConnectionStringParamName,
                                                    new Handler()
                                                    {
                                                        Validator = this.ConnectionStringHandler
                                                    }
                                                }
                                            };
            return;
        }
        
        private bool ValidateLogFilterSetting()
        {
            return ValidateLogFilterSetting(AzureConstants.LogFilterParamName);
        }

        private bool TraceContainerAbsenceHandler()
        {
            return ContainerAbsenceHandler(AzureConstants.DefaultLttTraceContainerName);
        }

        public override bool Validate(
                                 ITraceWriter traceEventSource,
                                 string sectionName,
                                 Dictionary<string, string> parameters, 
                                 Dictionary<string, SecureString> encryptedParameters)
        {
            bool success = base.Validate(
                               traceEventSource, 
                               sectionName, 
                               parameters, 
                               encryptedParameters);

            if (success && this.Enabled)
            {
                success = SetUpBlobContainerValidation(
                              AzureConstants.DefaultLttTraceContainerName);
            }

            return success;
        }
    }
}