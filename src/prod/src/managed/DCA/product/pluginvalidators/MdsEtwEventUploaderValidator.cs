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
    public class MdsEtwEventUploaderValidator : ClusterManifestSettingsValidator, IDcaSettingsValidator
    {
        protected override string isEnabledParamName
        {
            get
            {
                return MdsConstants.EnabledParamName;
            }
        }

        public MdsEtwEventUploaderValidator()
        {
            this.settingHandlers = new Dictionary<string, Handler>()
                                   {
                                       {
                                           MdsConstants.DirectoryParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateDirectoryNameSetting,
                                               AbsenceHandler = this.DirectoryNameAbsenceHandler
                                           }
                                       },
                                       {
                                           MdsConstants.TableParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateTableNameSetting,
                                               AbsenceHandler = this.TableNameAbsenceHandler
                                           }
                                       },
                                       {
                                           MdsConstants.UploadIntervalParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateUploadIntervalSetting
                                           }
                                       },
                                       {
                                           MdsConstants.TablePriorityParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateTablePrioritySetting
                                           }
                                       },
                                       {
                                           MdsConstants.DiskQuotaParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateDiskQuotaSetting
                                           }
                                       },
                                       {
                                           MdsConstants.DataDeletionAgeParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateLogDeletionAgeSetting
                                           }
                                       },
                                       {
                                           MdsConstants.TestDataDeletionAgeParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateLogDeletionAgeTestSetting
                                           }
                                       },
                                       {
                                           MdsConstants.LogFilterParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateLogFilterSetting
                                           }
                                       },
                                   };
            this.encryptedSettingHandlers = new Dictionary<string, Handler>();
            return;
        }

        private bool DirectoryNameAbsenceHandler()
        {
            this.traceSource.WriteError(
                this.logSourceId,
                "Setting {0} must be present in section {1}.",
                MdsConstants.DirectoryParamName,
                this.sectionName);
            return false;
        }

        private bool ValidateDirectoryNameSetting()
        {
            string directoryName = this.parameters[MdsConstants.DirectoryParamName];
            if (String.IsNullOrEmpty(directoryName))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Section {0}, parameter {1} cannot be an empty string.",
                    this.sectionName,
                    MdsConstants.DirectoryParamName);
                return false;
            }
            return true;
        }

        private bool ValidateTableNameSetting()
        {
            string tableName = this.parameters[MdsConstants.TableParamName];
            Regex regEx = new Regex(MdsConstants.TableNameRegularExpressionManifest);
            if (false == regEx.IsMatch(tableName))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Invalid table name {0} specified in section {1}, parameter {2}. Table names must match the regular expression {3}.",
                    tableName,
                    this.sectionName,
                    MdsConstants.TableParamName,
                    MdsConstants.TableNameRegularExpressionManifest);
                return false;
            }
            return true;
        }

        private bool TableNameAbsenceHandler()
        {
            WriteWarningInVSOutputFormat(
                this.logSourceId,
                "The value for setting {0} in section {1} is not specified. The default value {2} will be used.",
                MdsConstants.TableParamName,
                this.sectionName,
                MdsConstants.DefaultTableName);
            return true;
        }

        private bool ValidateUploadIntervalSetting()
        {
            const int minRecommendedUploadInterval = 5;
            return ValidateUploadIntervalSetting(
                       MdsConstants.UploadIntervalParamName,
                       minRecommendedUploadInterval);
        }

        private bool ValidateTablePrioritySetting()
        {
            string[] tablePriority = new string[] { "PriLow", "PriNormal", "PriHigh" };
            if (null == Array.Find(
                            tablePriority, 
                            p => 
                            { 
                                return p.Equals(this.parameters[MdsConstants.TablePriorityParamName], StringComparison.Ordinal); 
                            }))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Invalid table priority {0} specified in section {1}, parameter {2}",
                    this.parameters[MdsConstants.TablePriorityParamName],
                    this.sectionName,
                    MdsConstants.TablePriorityParamName);
                return false;
            }
            return true;
        }

        private bool ValidateDiskQuotaSetting()
        {
            int dontCare;
            return ValidateNonNegativeIntegerValue(
                       MdsConstants.DiskQuotaParamName,
                       this.parameters[MdsConstants.DiskQuotaParamName],
                       out dontCare);
        }

        private bool ValidateLogDeletionAgeSetting()
        {
            return ValidateLogDeletionAgeSetting(
                       MdsConstants.DataDeletionAgeParamName,
                       (int)MdsConstants.MaxDataDeletionAge.TotalDays);
        }

        private bool ValidateLogDeletionAgeTestSetting()
        {
            return ValidateLogDeletionAgeSetting(
                       MdsConstants.TestDataDeletionAgeParamName,
                       (int)MdsConstants.MaxDataDeletionAge.TotalMinutes);
        }

        private bool ValidateLogFilterSetting()
        {
            return ValidateLogFilterSetting(MdsConstants.LogFilterParamName);
        }
    }
}