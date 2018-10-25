// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Security;
    using System.Collections.Generic;
    using System.Fabric.Dca.Validator;

    // This class implements validation for folder producer settings in the cluster
    // manifest
    public class FolderProducerValidator : ClusterManifestSettingsValidator, IDcaSettingsValidator
    {
        public const string EnabledParamName = "IsEnabled";
        public const string FolderTypeParamName = "FolderType";
        public const string FolderPathParamName = "FolderPath";
        public const string DataDeletionAgeParamName = "DataDeletionAgeInDays";
        public const string TestDataDeletionAgeParamName = "TestOnlyDataDeletionAgeInMinutes";
        public const string AppRelativeFolderPathParamName = "RelativeFolderPath";
        public const string KernelCrashUploadEnabled = "KernelCrashUploadEnabled";

        public const string WindowsFabricCrashDumps = "WindowsFabricCrashDumps";
        public const string WindowsFabricPerformanceCounters = "WindowsFabricPerformanceCounters";
        public const string ServiceFabricCrashDumps = "ServiceFabricCrashDumps";
        public const string ServiceFabricPerformanceCounters = "ServiceFabricPerformanceCounters";
        public const string CustomFolder = "CustomFolder";

        // Folder type specified by the user
        private FolderProducerType folderType;
        
        // Whether or not a custom folder path has been provided
        private bool folderPathProvided;

        // Whether or not a kernel dump upload for linux is configured
        private bool kernelCrashConfigured;

        protected override string isEnabledParamName
        {
            get
            {
                return EnabledParamName;
            }
        }

        public FolderProducerValidator()
        {
            this.folderType = FolderProducerType.Invalid;
            this.folderPathProvided = false;
            this.kernelCrashConfigured = false;
            this.settingHandlers = new Dictionary<string, Handler>()
                                   {
                                       {
                                           FolderTypeParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateFolderTypeSetting,
                                               AbsenceHandler = this.FolderTypeAbsenceHandler
                                           }
                                       },
                                       {
                                           FolderPathParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateFolderPathSetting
                                           }
                                       },
                                       {
                                           DataDeletionAgeParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateLogDeletionAgeSetting
                                           }
                                       },
                                       {
                                           TestDataDeletionAgeParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateLogDeletionAgeTestSetting
                                           }
                                       },
                                       {
                                           KernelCrashUploadEnabled,
                                           new Handler()
                                           {
                                               Validator = this.ValidateKernelCrashUploadEnabled
                                           }
                                       },
                                   };
            this.encryptedSettingHandlers = new Dictionary<string, Handler>();
            return;
        }
        
        private bool ValidateFolderTypeSetting()
        {
            string folderTypeAsString = this.parameters[FolderTypeParamName];
            if (folderTypeAsString.Equals(
                    WindowsFabricCrashDumps,
                    StringComparison.Ordinal) || folderTypeAsString.Equals(
                    ServiceFabricCrashDumps,
                    StringComparison.Ordinal))
            {
                folderType = FolderProducerType.WindowsFabricCrashDumps;
                return true;
            }
            else if (folderTypeAsString.Equals(
                    WindowsFabricPerformanceCounters,
                    StringComparison.Ordinal) || folderTypeAsString.Equals(
                    ServiceFabricPerformanceCounters,
                    StringComparison.Ordinal))
            {
                folderType = FolderProducerType.WindowsFabricPerformanceCounters;
                return true;
            }
            else if (folderTypeAsString.Equals(
                         CustomFolder,
                         StringComparison.Ordinal))
            {
                folderType = FolderProducerType.CustomFolder;
                return true;
            }
            else
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "The folder type '{0}' specified in section {1}, parameter {2} is not supported.",
                    folderTypeAsString,
                    this.sectionName,
                    FolderTypeParamName);
                return false;
            }
        }

        private bool FolderTypeAbsenceHandler()
        {
            this.traceSource.WriteError(
                this.logSourceId,
                "Required parameter {0} in section {1} has not been specified.",
                FolderTypeParamName,
                this.sectionName);
            return false;
        }

        private bool ValidateFolderPathSetting()
        {
            this.folderPathProvided = true;
            return true;
        }

        private bool ValidateLogDeletionAgeSetting()
        {
            return this.ValidateLogDeletionAgeSetting(
                       DataDeletionAgeParamName,
                       (int)FolderProducerConstants.MaxDataDeletionAge.TotalDays);
        }

        private bool ValidateLogDeletionAgeTestSetting()
        {
            return this.ValidateLogDeletionAgeSetting(
                       TestDataDeletionAgeParamName,
                       (int)FolderProducerConstants.MaxDataDeletionAge.TotalMinutes);
        }

        private bool ValidateKernelCrashUploadEnabled()
        {
            bool kernelCrashUploadEnabled = false;
            if (false == ConvertFromString(KernelCrashUploadEnabled, this.parameters[KernelCrashUploadEnabled], ref kernelCrashUploadEnabled))
            {
                return false;
            }

            if (kernelCrashUploadEnabled == true)
            {
                kernelCrashConfigured = true;
            }

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
                if ((FolderProducerType.CustomFolder == this.folderType) &&
                    (false == this.folderPathProvided))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Setting {0} in section {1} is specified as {2}, but setting {3} has not been specified.",
                        FolderTypeParamName,
                        this.sectionName,
                        CustomFolder,
                        FolderPathParamName);
                    success = false;
                }
                if ((this.folderPathProvided) && 
                    (FolderProducerType.CustomFolder != this.folderType))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Setting {0} in section {1} is not specified as {2}. Therefore setting {3} should also not be specified.",
                        FolderTypeParamName,
                        this.sectionName,
                        CustomFolder,
                        FolderPathParamName);
                    success = false;
                }
                if ((this.kernelCrashConfigured) &&
                    (FolderProducerType.WindowsFabricCrashDumps != this.folderType))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Setting {0} in section {1} is not specified as {2}. Therefore setting {3} should also not be specified.",
                        FolderTypeParamName,
                        this.sectionName,
                        WindowsFabricCrashDumps,
                        FolderPathParamName);
                    success = false;
                }
            }
            return success;
        }
    }
}