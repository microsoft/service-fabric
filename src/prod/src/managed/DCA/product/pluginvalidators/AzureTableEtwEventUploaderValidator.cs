// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Dca.Validator;
    using System.Security;
    
    // This class implements validation for Azure table upload settings in the cluster
    // manifest
    public class AzureTableEtwEventUploaderValidator : ClusterManifestAzureSettingsValidator, IDcaAzureSettingsValidator
    {
        protected override string isEnabledParamName
        {
            get
            {
                return AzureConstants.EnabledParamName;
            }
        }

        public AzureTableEtwEventUploaderValidator()
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
                                           AzureConstants.EtwTraceTableParamName,
                                           new Handler()
                                           {
                                               AbsenceHandler = this.EtwTraceTableAbsenceHandler
                                           }
                                       },
                                       {
                                           AzureConstants.UploadIntervalParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateUploadIntervalSetting
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
                                           AzureConstants.UploadConcurrencyParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateUploadConcurrencySetting
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
        
        private bool ValidateUploadIntervalSetting()
        {
            const int minRecommendedUploadInterval = 5;
            return ValidateUploadIntervalSetting(
                       AzureConstants.UploadIntervalParamName,
                       minRecommendedUploadInterval);
        }

        private bool ValidateLogDeletionAgeSetting()
        {
            return ValidateLogDeletionAgeSetting(
                       AzureConstants.DataDeletionAgeParamName,
                       (int)AzureConstants.MaxDataDeletionAge.TotalDays);
        }

        private bool ValidateLogDeletionAgeTestSetting()
        {
            return ValidateLogDeletionAgeSetting(
                       AzureConstants.TestDataDeletionAgeParamName,
                       (int)AzureConstants.MaxDataDeletionAge.TotalMinutes);
        }

        private bool ValidateLogFilterSetting()
        {
            return ValidateLogFilterSetting(AzureConstants.LogFilterParamName);
        }

        private bool ValidateUploadConcurrencySetting()
        {
            string uploadConcurrencyAsString = this.parameters[AzureConstants.UploadConcurrencyParamName];
            int uploadConcurrency;
            if (false == ValidatePostiveIntegerValue(
                             AzureConstants.UploadConcurrencyParamName,
                             uploadConcurrencyAsString, 
                             out uploadConcurrency))
            {
                return false;
            }
            if (AzureConstants.MaxBatchConcurrencyCount < uploadConcurrency)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "The value for setting {0} in section {1} is currently specified as {2}. The maximum supported value is {3}.",
                    AzureConstants.UploadConcurrencyParamName,
                    this.sectionName,
                    uploadConcurrency,
                    AzureConstants.MaxBatchConcurrencyCount);
                return false;
            }
            return true;
        }

        private bool EtwTraceTableAbsenceHandler()
        {
            WriteWarningInVSOutputFormat(
                this.logSourceId,
                "The value for setting {0} in section {1} is not specified. The default value {2} will be used.",
                AzureConstants.EtwTraceTableParamName,
                this.sectionName,
                AzureConstants.DefaultTableName);
            return true;
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
                SecureString connectionString;
                success = GetConnectionString(
                              AzureConstants.ConnectionStringParamName, 
                              out connectionString);
                if (success)
                {
                    AzureTableInfo tableInfo;
                    tableInfo.ConnectionString = connectionString;
                    tableInfo.TableName = this.parameters.ContainsKey(AzureConstants.EtwTraceTableParamName) ? 
                                              this.parameters[AzureConstants.EtwTraceTableParamName] :
                                              AzureConstants.DefaultTableName;
                    this.tableInfoList.Add(tableInfo);
                }
            }
        
            return success;
        }
    }
}