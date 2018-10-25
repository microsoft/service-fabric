// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Security;
    using System.Globalization;
    using System.IO;
    using System.Collections.Generic;
    using System.Fabric.Dca.Validator;

    // This class implements validation for ETL producer settings in the cluster
    // manifest
    public class EtlProducerValidator : ClusterManifestSettingsValidator, IDcaSettingsValidator
    {
        public const string EnabledParamName = "IsEnabled";
        public const string EtlReadIntervalParamName = "EtlReadIntervalInMinutes";
        public const string UploadIntervalParamName = "UploadIntervalInMinutes";
        public const string DataDeletionAgeParamName = "DataDeletionAgeInDays";
        public const string WindowsFabricEtlTypeParamName = "WindowsFabricEtlType";
        public const string ServiceFabricEtlTypeParamName = "ServiceFabricEtlType";
        public const string TestDataDeletionAgeParamName = "TestOnlyDataDeletionAgeInMinutes";
        public const string TestEtlPathParamName = "TestOnlyEtlPath";
        public const string TestEtlFilePatternsParamName = "TestOnlyEtlFilePatterns";
        public const string TestManifestPathParamName = "TestOnlyCustomManifestPath";
        public const string TestProcessingWinFabEtlFilesParamName = "TestOnlyProcessingWinFabEtlFiles";

        public const string DefaultEtl = "DefaultEtl";
        public const string QueryEtl = "QueryEtl";
        public const string OperationalEtl = "OperationalEtl";

        // Constants
        private const string starDotEtl = "*.etl";

        // Windows Fabric ETL type specified by the user
        WinFabricEtlType etlType;

        // Whether or not a custom ETL path has been provided
        private bool etlPathProvided;

        // Custom ETL path
        private string etlPath;

        // Whether or not ETL file patterns have been provided
        private bool etlPatternsProvided;

        // Whether or not a custom manifest path has been provided
        private bool customManifestPathProvided;

        // Whether or not the setting that specifies whether the ETL files are

        // Windows Fabric ETL files is present
        private bool winFabEtlFilesSettingPresent;
        
        protected override string isEnabledParamName
        {
            get
            {
                return EnabledParamName;
            }
        }

        public EtlProducerValidator()
        {
            this.etlType = WinFabricEtlType.Unspecified;
            this.settingHandlers = new Dictionary<string, Handler>()
                                   {
                                       {
                                           EtlReadIntervalParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateEtlReadIntervalSetting
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
                                           WindowsFabricEtlTypeParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateEtlTypeSetting
                                           }
                                       },
                                       {
                                           ServiceFabricEtlTypeParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateEtlTypeSetting
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
                                           TestEtlPathParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateEtlPath
                                           }
                                       },
                                       {
                                           TestEtlFilePatternsParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateEtlFilePatterns
                                           }
                                       },
                                       {
                                           TestManifestPathParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateManifestPath
                                           }
                                       },
                                       {
                                           TestProcessingWinFabEtlFilesParamName,
                                           new Handler()
                                           {
                                               Validator = this.ValidateProcessingWinFabEtlFilesSetting
                                           }
                                       }
                                   };
            this.encryptedSettingHandlers = new Dictionary<string, Handler>();
            return;
        }
        
        private bool ValidateEtlReadIntervalSetting()
        {
            const int minRecommendedEtlReadInterval = 5;
            string etlReadIntervalAsString = this.parameters[EtlReadIntervalParamName];
            
            int etlReadInterval;
            if (false == this.ValidateNonNegativeIntegerValue(
                             EtlReadIntervalParamName,
                             etlReadIntervalAsString, 
                             out etlReadInterval))
            {
                return false;
            }

            if ((0 != etlReadInterval) && 
                (minRecommendedEtlReadInterval > etlReadInterval))
            {
                this.WriteWarningInVSOutputFormat(
                    this.logSourceId,
                    "The value for setting {0} in section {1} is currently specified as {2} minutes. The minimum recommended value {3} minutes.",
                    EtlReadIntervalParamName,
                    this.sectionName,
                    etlReadInterval,
                    minRecommendedEtlReadInterval);
            }

            return true;
        }

        private bool ValidateLogDeletionAgeSetting()
        {
            return this.ValidateLogDeletionAgeSetting(
                       DataDeletionAgeParamName,
                       (int)EtlProducerConstants.MaxDataDeletionAge.TotalDays);
        }

        private bool ValidateEtlTypeSetting()
        {
            string etlTypeAsString;
            string etlParameterName;
            if(this.parameters.ContainsKey(WindowsFabricEtlTypeParamName)){
                etlTypeAsString = this.parameters[WindowsFabricEtlTypeParamName];
                etlParameterName = WindowsFabricEtlTypeParamName;
            }
            else
            {
                etlTypeAsString = this.parameters[ServiceFabricEtlTypeParamName];
                etlParameterName = ServiceFabricEtlTypeParamName;
            }
            
            if (etlTypeAsString.Equals(
                DefaultEtl,
                StringComparison.Ordinal))
            {
                this.etlType = WinFabricEtlType.DefaultEtl;
                return true;
            }
            else if (etlTypeAsString.Equals(
                QueryEtl,
                StringComparison.Ordinal))
            {
                this.etlType = WinFabricEtlType.QueryEtl;
                return true;
            }
            else if (etlTypeAsString.Equals(
                OperationalEtl,
                StringComparison.Ordinal))
            {
                this.etlType = WinFabricEtlType.OperationalEtl;
                return true;
            }
            else
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "The ETL type '{0}' specified in section {1}, parameter {2} is not supported.",
                    etlTypeAsString,
                    this.sectionName,
                    etlParameterName);
                return false;
            }
        }

        private bool ValidateLogDeletionAgeTestSetting()
        {
            return this.ValidateLogDeletionAgeSetting(
                       TestDataDeletionAgeParamName,
                       (int)EtlProducerConstants.MaxDataDeletionAge.TotalDays);
        }

        private bool ValidateEtlPath()
        {
            this.etlPathProvided = true;
            this.etlPath = this.parameters[TestEtlPathParamName];
            return true;
        }

        private bool ValidateEtlFilePatterns()
        {
            this.etlPatternsProvided = true;
            
            bool success = true;
            string etlFilePatterns = this.parameters[TestEtlFilePatternsParamName];
            string[] patternArray = etlFilePatterns.Split(',');
            foreach (string patternAsIs in patternArray)
            {
                string pattern = patternAsIs.Trim();
        
                // Make sure the pattern contains no path information
                string tempPattern = Path.GetFileName(pattern);
                if (false == tempPattern.Equals(
                                 pattern, 
                                 StringComparison.OrdinalIgnoreCase))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "The ETL file pattern '{0}' in section {1} parameter {2} is invalid. Patterns should not contain any path information.",
                        pattern,
                        this.sectionName,
                        TestEtlFilePatternsParamName);
                    success = false;
                }
        
                // Make sure the pattern ends with *.etl"
                if (false == pattern.EndsWith(
                                 starDotEtl,
                                 StringComparison.OrdinalIgnoreCase))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "The ETL file pattern '{0}' in section {1} parameter {2} is invalid. Patterns should always end with {3}.",
                        pattern,
                        this.sectionName,
                        TestEtlFilePatternsParamName,
                        starDotEtl);
                    success = false;
                }
            }

            return success;
        }

        private bool ValidateManifestPath()
        {
            this.customManifestPathProvided = true;
            return true;
        }

        private bool ValidateProcessingWinFabEtlFilesSetting()
        {
            this.winFabEtlFilesSettingPresent = true;
            
            bool success = true;
            bool processingWinFabEtlFiles = false;
            string processingWinFabEtlFilesAsString = this.parameters[TestProcessingWinFabEtlFilesParamName];
            if (false == ConvertFromString(
                             TestProcessingWinFabEtlFilesParamName,
                             processingWinFabEtlFilesAsString,
                             ref processingWinFabEtlFiles))
            {
                success = false;
            }
            return success;
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
                if (this.etlPathProvided)
                {
                    if (false == this.etlPatternsProvided)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Setting {0} in section {1} should not be specified without specifying setting {2} as well.",
                            TestEtlPathParamName,
                            this.sectionName,
                            TestEtlFilePatternsParamName);
                        success = false;
                    }

                    if (WinFabricEtlType.Unspecified != this.etlType)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Setting {0}/{1} in section {2} should not be specified if setting {3} is specified.",
                            WindowsFabricEtlTypeParamName,
                            ServiceFabricEtlTypeParamName,
                            this.sectionName,
                            TestEtlPathParamName);
                        success = false;
                    }
                }
                else
                {
                    if (this.etlPatternsProvided)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Setting {0} in section {1} should not be specified without specifying setting {2} as well.",
                            TestEtlFilePatternsParamName,
                            this.sectionName,
                            TestEtlPathParamName);
                        success = false;
                    }
                    if (this.customManifestPathProvided)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Setting {0} in section {1} should not be specified without specifying setting {2} as well.",
                            TestManifestPathParamName,
                            this.sectionName,
                            TestEtlPathParamName);
                        success = false;
                    }
                    if (this.winFabEtlFilesSettingPresent)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Setting {0} in section {1} should not be specified without specifying setting {2} as well.",
                            TestProcessingWinFabEtlFilesParamName,
                            this.sectionName,
                            TestEtlPathParamName);
                        success = false;
                    }
                }
            }
        
            return success;
        }
    }
}