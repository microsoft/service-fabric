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
    public class MdsFileProducerValidator : ClusterManifestSettingsValidator, IDcaSettingsValidator
    {
        protected override string isEnabledParamName
        {
            get
            {
                return MdsFileProducerConstants.EnabledParamName;
            }
        }

        public MdsFileProducerValidator()
        {
            this.settingHandlers = new Dictionary<string, Handler>()
                                   {
                                       {
                                           MdsFileProducerConstants.DirectoryParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateDirectoryNameSetting,
                                               AbsenceHandler = this.DirectoryNameAbsenceHandler
                                           }
                                       },
                                       {
                                           MdsFileProducerConstants.TableParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateTableNameSetting,
                                               AbsenceHandler = this.TableNameAbsenceHandler
                                           }
                                       },
                                       {
                                           MdsFileProducerConstants.TablePriorityParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateTablePrioritySetting
                                           }
                                       },
                                       {
                                           MdsFileProducerConstants.DiskQuotaParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateDiskQuotaSetting
                                           }
                                       },
                                       {
                                           MdsFileProducerConstants.DataDeletionAgeParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateLogDeletionAgeSetting
                                           }
                                       },
                                       {
                                           MdsFileProducerConstants.TestDataDeletionAgeParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateLogDeletionAgeTestSetting
                                           }
                                       },
                                       {
                                           MdsFileProducerConstants.LogFilterParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateLogFilterSetting
                                           }
                                       },
                                       {
                                           MdsFileProducerConstants.BookmarkBatchSizeParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateBookmarkBatchSizeSetting
                                           }
                                       }
                                   };
            this.encryptedSettingHandlers = new Dictionary<string, Handler>();
            return;
        }

        private bool DirectoryNameAbsenceHandler()
        {
            this.traceSource.WriteError(
                this.logSourceId,
                "Setting {0} must be present in section {1}.",
                MdsFileProducerConstants.DirectoryParamName,
                this.sectionName);
            return false;
        }

        private bool ValidateDirectoryNameSetting()
        {
            string directoryName = this.parameters[MdsFileProducerConstants.DirectoryParamName];
            if (String.IsNullOrEmpty(directoryName))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Section {0}, parameter {1} cannot be an empty string.",
                    this.sectionName,
                    MdsFileProducerConstants.DirectoryParamName);
                return false;
            }
            return true;
        }

        private bool ValidateTableNameSetting()
        {
            string tableName = this.parameters[MdsFileProducerConstants.TableParamName];
            Regex regEx = new Regex(MdsFileProducerConstants.TableNameRegularExpressionManifest);
            if (false == regEx.IsMatch(tableName))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Invalid table name {0} specified in section {1}, parameter {2}. Table names must match the regular expression {3}.",
                    tableName,
                    this.sectionName,
                    MdsFileProducerConstants.TableParamName,
                    MdsFileProducerConstants.TableNameRegularExpressionManifest);
                return false;
            }
            return true;
        }

        private bool TableNameAbsenceHandler()
        {
            WriteWarningInVSOutputFormat(
                this.logSourceId,
                "The value for setting {0} in section {1} is not specified. The default value {2} will be used.",
                MdsFileProducerConstants.TableParamName,
                this.sectionName,
                MdsFileProducerConstants.DefaultTableName);
            return true;
        }

        private bool ValidateTablePrioritySetting()
        {
            string[] tablePriority = new string[] { "PriLow", "PriNormal", "PriHigh" };
            if (null == Array.Find(
                            tablePriority, 
                            p => 
                            { 
                                return p.Equals(this.parameters[MdsFileProducerConstants.TablePriorityParamName], StringComparison.Ordinal); 
                            }))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Invalid table priority {0} specified in section {1}, parameter {2}",
                    this.parameters[MdsFileProducerConstants.TablePriorityParamName],
                    this.sectionName,
                    MdsFileProducerConstants.TablePriorityParamName);
                return false;
            }
            return true;
        }

        private bool ValidateDiskQuotaSetting()
        {
            int dontCare;
            return ValidateNonNegativeIntegerValue(
                       MdsFileProducerConstants.DiskQuotaParamName,
                       this.parameters[MdsFileProducerConstants.DiskQuotaParamName],
                       out dontCare);
        }

        private bool ValidateLogDeletionAgeSetting()
        {
            return ValidateLogDeletionAgeSetting(
                       MdsFileProducerConstants.DataDeletionAgeParamName,
                       (int)MdsFileProducerConstants.MaxDataDeletionAge.TotalDays);
        }

        private bool ValidateLogDeletionAgeTestSetting()
        {
            return ValidateLogDeletionAgeSetting(
                       MdsFileProducerConstants.TestDataDeletionAgeParamName,
                       (int)MdsFileProducerConstants.MaxDataDeletionAge.TotalMinutes);
        }

        private bool ValidateLogFilterSetting()
        {
            return ValidateLogFilterSetting(MdsFileProducerConstants.LogFilterParamName);
        }

        private bool ValidateBookmarkBatchSizeSetting()
        {
            int dontCare;
            return ValidateNonNegativeIntegerValue(
                       MdsFileProducerConstants.BookmarkBatchSizeParamName,
                       this.parameters[MdsFileProducerConstants.BookmarkBatchSizeParamName],
                       out dontCare);
        }
    }
}