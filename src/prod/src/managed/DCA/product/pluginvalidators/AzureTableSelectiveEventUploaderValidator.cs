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
    using System.Text.RegularExpressions;

    // This class implements validation for Azure table upload settings in the cluster
    // manifest
    public class AzureTableSelectiveEventUploaderValidator : ClusterManifestAzureSettingsValidator, IDcaAzureSettingsValidator
    {
        private const string TableNamePrefixRegularExpression = "^[A-Za-z][A-Za-z0-9]{0,50}$";
        private string etwTraceTablePrefix;

        protected override string isEnabledParamName
        {
            get
            {
                return AzureConstants.EnabledParamName;
            }
        }

        public AzureTableSelectiveEventUploaderValidator()
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
                                           AzureConstants.EtwTraceTablePrefixParamName,
                                           new Handler()
                                           {
                                               Validator = this.EtwTraceTablePrefixHandler,
                                               AbsenceHandler = this.EtwTraceTablePrefixAbsenceHandler
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

        private bool EtwTraceTablePrefixHandler()
        {
            this.etwTraceTablePrefix = this.parameters[AzureConstants.EtwTraceTablePrefixParamName];

            Regex regEx = new Regex(TableNamePrefixRegularExpression);
            if (false == regEx.IsMatch(this.etwTraceTablePrefix))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "The value for parameter {0} in section {1} does not match the regular expression {2}.",
                    AzureConstants.EtwTraceTablePrefixParamName,
                    this.sectionName,
                    TableNamePrefixRegularExpression);
                return false;
            }
            return true;
        }

        private bool EtwTraceTablePrefixAbsenceHandler()
        {
            WriteWarningInVSOutputFormat(
                this.logSourceId,
                "The value for setting {0} in section {1} is not specified. The default value {2} will be used.",
                AzureConstants.EtwTraceTablePrefixParamName,
                this.sectionName,
                AzureConstants.DefaultTableNamePrefix);
            this.etwTraceTablePrefix = AzureConstants.EtwTraceTablePrefixParamName;
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
                if (false == String.IsNullOrEmpty(this.deploymentId))
                {
                    if (this.etwTraceTablePrefix.Length > AzureConstants.MaxTableNamePrefixLengthForDeploymentIdInclusion)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "The value for parameter {0} in section {1} cannot exceed {2} characters in length if parameter {3} is also specified in that section.",
                            AzureConstants.EtwTraceTablePrefixParamName,
                            this.sectionName,
                            AzureConstants.MaxTableNamePrefixLengthForDeploymentIdInclusion,
                            AzureConstants.DeploymentId);
                        success = false;
                    }
                }
            }

            return success;
        }
    }
}