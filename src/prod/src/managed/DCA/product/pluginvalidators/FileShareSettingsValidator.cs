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

    // This clas implements validation for file share upload settings in the
    // cluster manifest
    public abstract class ClusterManifestFileShareSettingsValidator : ClusterManifestSettingsValidator
    {
        private const string Default = @"_default_";

        enum AccountType
        {
            DomainUser,
            ManagedServiceAccount
        }

        private bool destinationPathSpecified;
        private bool accountNameSpecified;
        private bool accountPasswordSpecified;
        private bool accountTypeSpecified;

        private AccountType accountType;

        protected bool ValidateLogDeletionAgeSetting()
        {
            return ValidateLogDeletionAgeSetting(
                       FileShareUploaderConstants.DataDeletionAgeParamName,
                       FileShareUploaderConstants.MaxDataDeletionAge.Days);
        }

        protected bool ValidateLogDeletionAgeTestSetting()
        {
            return ValidateLogDeletionAgeSetting(
                       FileShareUploaderConstants.TestDataDeletionAgeParamName,
                       (int)FileShareUploaderConstants.MaxDataDeletionAge.TotalMinutes);
        }

        protected bool ValidateLogFilterSetting()
        {
            return ValidateLogFilterSetting(FileShareUploaderConstants.LogFilterParamName);
        }

        protected bool DestinationPathHandler()
        {
            this.destinationPathSpecified = true;
            string destinationPath = this.parameters[FileShareUploaderConstants.StoreConnectionStringParamName];
            if (destinationPath.Equals(Default))
            {
                return true;
            }

            try
            {
                Uri destinationUri = new Uri(destinationPath);
            }
            catch (UriFormatException e)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "The value '{0}' for setting {1} in section {2} could not be parsed as a URI. Exception information: {3}",
                    destinationPath,
                    FileShareUploaderConstants.StoreConnectionStringParamName,
                    this.sectionName,
                    e);
                return false;
            }

            return true;
        }

        protected bool ValidateUploadIntervalSetting()
        {
            const int minRecommendedUploadInterval = 5;
            return ValidateUploadIntervalSetting(
                       FileShareUploaderConstants.UploadIntervalParamName,
                       minRecommendedUploadInterval);
        }

        protected bool ValidateFileSyncIntervalSetting()
        {
            return ValidateFileSyncIntervalSetting(FileShareUploaderConstants.FileSyncIntervalParamName);
        }

        protected bool ValidateAccountName()
        {
            string accountName = this.parameters[FileShareUploaderConstants.StoreAccessAccountName];
            if (String.IsNullOrEmpty(accountName))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Parameter {0} in section {1} should not be an empty string.",
                    FileShareUploaderConstants.StoreAccessAccountName,
                    this.sectionName);
                return false;
            }

            string[] names = null;
            if (accountName.Contains("@"))
            {
                names = accountName.Split('@');
            }
            else if (accountName.Contains("\\"))
            {
                names = accountName.Split('\\');
            }
            if ((null != names) && (names.Length != 2))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "The value '{0}' for parameter {1} in section {2} is invalid. Account name in UPN or downlevel name format is required.",
                    accountName,
                    FileShareUploaderConstants.StoreAccessAccountName,
                    this.sectionName);
                return false;
            }

            this.accountNameSpecified = true;
            return true;
        }

        protected bool AccountPasswordHandler()
        {
            this.accountPasswordSpecified = true;
            return true;
        }

        protected bool ValidateAccountType()
        {
            string accountTypeAsString = this.parameters[FileShareUploaderConstants.StoreAccessAccountType];
            if (false == Enum.TryParse(
                            accountTypeAsString, 
                            out this.accountType))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "{0} is not a supported value for setting {1} in section {2}",
                    accountTypeAsString,
                    FileShareUploaderConstants.StoreAccessAccountType,
                    this.sectionName);
                return false;
            }
            this.accountTypeSpecified = true;
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
                if (false == this.destinationPathSpecified)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Consumer {0} is enabled, but the required parameter {1} for this consumer has not been provided.",
                        this.sectionName,
                        FileShareUploaderConstants.StoreConnectionStringParamName);
                    success = false;
                }

                if (this.accountTypeSpecified)
                {
                    if (false == this.accountNameSpecified)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "If parameter {0} is specified in section {1}, parameter {2} should also be specified.",
                            FileShareUploaderConstants.StoreAccessAccountType,
                            this.sectionName,
                            FileShareUploaderConstants.StoreAccessAccountName);
                        success = false;
                    }

                    switch (this.accountType)
                    {
                        case AccountType.DomainUser:
                            {
                                if (false == this.accountPasswordSpecified)
                                {
                                    this.traceSource.WriteError(
                                        this.logSourceId,
                                        "If parameter {0} is specified as {1} in section {2}, parameter {3} should also be specified.",
                                        FileShareUploaderConstants.StoreAccessAccountType,
                                        this.accountType,
                                        this.sectionName,
                                        FileShareUploaderConstants.StoreAccessAccountPassword);
                                    success = false;
                                }
                            }
                            break;

                        case AccountType.ManagedServiceAccount:
                            {
                                if (this.accountPasswordSpecified)
                                {
                                    this.traceSource.WriteError(
                                        this.logSourceId,
                                        "If parameter {0} is specified as {1} in section {2}, parameter {3} should not be specified.",
                                        FileShareUploaderConstants.StoreAccessAccountType,
                                        this.accountType,
                                        this.sectionName,
                                        FileShareUploaderConstants.StoreAccessAccountPassword);
                                    success = false;
                                }
                            }
                            break;
                    }
                }
                else
                {
                    if (this.accountNameSpecified)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Parameter {0} should not be specified in section {1} unless parameter {2} has also been specified.",
                            FileShareUploaderConstants.StoreAccessAccountName,
                            this.sectionName,
                            FileShareUploaderConstants.StoreAccessAccountType);
                        success = false;
                    }
                    if (this.accountPasswordSpecified)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Parameter {0} should not be specified in section {1} unless parameter {2} has also been specified.",
                            FileShareUploaderConstants.StoreAccessAccountPassword,
                            this.sectionName,
                            FileShareUploaderConstants.StoreAccessAccountType);
                        success = false;
                    }
                }
            }

            return success;
        }
    }
}