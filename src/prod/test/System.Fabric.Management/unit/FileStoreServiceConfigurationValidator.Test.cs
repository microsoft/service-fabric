// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Collections.Generic;
    using WEX.TestExecution;

    using FileStoreService = System.Fabric.FabricValidatorConstants.ParameterNames.FileStoreService;

    [TestClass]
    public class FileStoreServiceConfigurationValidatorTest
    {
        FileStoreServiceConfigurationValidator validator;

        [TestInitialize]
        public void TestSetup()
        {
            this.validator = new FileStoreServiceConfigurationValidator();
        }
        
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateConfigurationTest()
        {
            // positive
            List<SettingsOverridesTypeSectionParameter> parameters = new List<SettingsOverridesTypeSectionParameter>()
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName1Ntlmx509CommonName,
                    Value = "cn1"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName2Ntlmx509CommonName,
                    Value = "cn1"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonNameNtlmPasswordSecret,
                    Value = "password"
                },
            };
            this.TestValidateConfiguration(parameters, expectError: false);

            // negative: only 1 cn is declared
            parameters = new List<SettingsOverridesTypeSectionParameter>()
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName1Ntlmx509CommonName,
                    Value = "cn1"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonNameNtlmPasswordSecret,
                    Value = "password"
                },
            };
            this.TestValidateConfiguration(parameters, expectError: true);

            // negative: cn and secret are not declared together
            parameters = new List<SettingsOverridesTypeSectionParameter>()
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName1Ntlmx509CommonName,
                    Value = "cn1"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName2Ntlmx509CommonName,
                    Value = "cn1"
                },
            };
            this.TestValidateConfiguration(parameters, expectError: true);
        }


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateConfigurationUpgradeTest()
        {
            // positive: add cn
            List<SettingsOverridesTypeSectionParameter> current = new List<SettingsOverridesTypeSectionParameter>()
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName1Ntlmx509CommonName,
                    Value = "cn1"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName2Ntlmx509CommonName,
                    Value = "cn1"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonNameNtlmPasswordSecret,
                    Value = "password"
                },
            };
            List<SettingsOverridesTypeSectionParameter> target = new List<SettingsOverridesTypeSectionParameter>()
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName1Ntlmx509CommonName,
                    Value = "cn2"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName2Ntlmx509CommonName,
                    Value = "cn1"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonNameNtlmPasswordSecret,
                    Value = "password"
                },
            };
            this.TestValidateConfigurationUpgrade(current, target, expectError: false);

            // positive: remove cn
            current = new List<SettingsOverridesTypeSectionParameter>()
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName1Ntlmx509CommonName,
                    Value = "cn1"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName2Ntlmx509CommonName,
                    Value = "cn2"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonNameNtlmPasswordSecret,
                    Value = "password"
                },
            };
            target = new List<SettingsOverridesTypeSectionParameter>()
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName1Ntlmx509CommonName,
                    Value = "cn1"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName2Ntlmx509CommonName,
                    Value = "cn1"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonNameNtlmPasswordSecret,
                    Value = "password"
                },
            };
            this.TestValidateConfigurationUpgrade(current, target, expectError: false);

            // positive: thumbprint -> cn
            current = new List<SettingsOverridesTypeSectionParameter>();
            target = new List<SettingsOverridesTypeSectionParameter>()
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName1Ntlmx509CommonName,
                    Value = "cn1"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName2Ntlmx509CommonName,
                    Value = "cn2"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonNameNtlmPasswordSecret,
                    Value = "password"
                },
            };
            this.TestValidateConfigurationUpgrade(current, target, expectError: false);

            // positive: cn -> thumbprint
            current = new List<SettingsOverridesTypeSectionParameter>()
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName1Ntlmx509CommonName,
                    Value = "cn1"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName2Ntlmx509CommonName,
                    Value = "cn2"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonNameNtlmPasswordSecret,
                    Value = "password"
                },
            };
            target = new List<SettingsOverridesTypeSectionParameter>();
            this.TestValidateConfigurationUpgrade(current, target, expectError: false);

            // negative: target is invalid
            current = new List<SettingsOverridesTypeSectionParameter>()
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName1Ntlmx509CommonName,
                    Value = "cn1"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName2Ntlmx509CommonName,
                    Value = "cn2"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonNameNtlmPasswordSecret,
                    Value = "password"
                },
            };
            target = new List<SettingsOverridesTypeSectionParameter>()
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName1Ntlmx509CommonName,
                    Value = "cn1"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonNameNtlmPasswordSecret,
                    Value = "password"
                },
            };
            this.TestValidateConfigurationUpgrade(current, target, expectError: true);

            // negative: cn replace
            current = new List<SettingsOverridesTypeSectionParameter>()
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName1Ntlmx509CommonName,
                    Value = "cn1"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName2Ntlmx509CommonName,
                    Value = "cn1"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonNameNtlmPasswordSecret,
                    Value = "password"
                },
            };
            target = new List<SettingsOverridesTypeSectionParameter>()
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName1Ntlmx509CommonName,
                    Value = "cn2"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName2Ntlmx509CommonName,
                    Value = "cn2"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonNameNtlmPasswordSecret,
                    Value = "password"
                },
            };
            this.TestValidateConfigurationUpgrade(current, target, expectError: true);

            // negative: secret changes
            current = new List<SettingsOverridesTypeSectionParameter>()
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName1Ntlmx509CommonName,
                    Value = "cn1"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName2Ntlmx509CommonName,
                    Value = "cn2"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonNameNtlmPasswordSecret,
                    Value = "password"
                },
            };
            target = new List<SettingsOverridesTypeSectionParameter>()
            {
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName1Ntlmx509CommonName,
                    Value = "cn1"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonName2Ntlmx509CommonName,
                    Value = "cn2"
                },
                new SettingsOverridesTypeSectionParameter()
                {
                    Name = FileStoreService.CommonNameNtlmPasswordSecret,
                    Value = "password222"
                },
            };
            this.TestValidateConfigurationUpgrade(current, target, expectError: true);
        }

        private WindowsFabricSettings GetWindowsFabricSettings(List<SettingsOverridesTypeSectionParameter> parameters)
        {
            ClusterManifestType clusterManifest = (new ClusterManifestHelper(true, false)).ClusterManifest;
            clusterManifest.FabricSettings = new SettingsOverridesTypeSection[1];
            clusterManifest.FabricSettings[0] = new SettingsOverridesTypeSection() { Name = FabricValidatorConstants.SectionNames.FileStoreService, Parameter = parameters.ToArray() };
            return new WindowsFabricSettings(clusterManifest);
        }

        private void TestValidateConfiguration(
            List<SettingsOverridesTypeSectionParameter> parameters,
            bool expectError)
        {
            if (expectError)
            {
                try
                {
                    this.validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
                    Verify.Fail("should fail");
                }
                catch (ArgumentException)
                {
                }
            }
            else
            {
                this.validator.ValidateConfiguration(GetWindowsFabricSettings(parameters));
            }
        }
        
        private void TestValidateConfigurationUpgrade(
            List<SettingsOverridesTypeSectionParameter> currentParameters,
            List<SettingsOverridesTypeSectionParameter> targetParameters,
            bool expectError)
        {
            WindowsFabricSettings currentWindowsFabricSettings = GetWindowsFabricSettings(currentParameters);
            WindowsFabricSettings targetWindowsFabricSettings = GetWindowsFabricSettings(targetParameters);

            if (expectError)
            {
                try
                {
                    this.validator.ValidateConfigurationUpgrade(currentWindowsFabricSettings, targetWindowsFabricSettings);
                    Verify.Fail("should fail");
                }
                catch (ArgumentException)
                {
                }
            }
            else
            {
                this.validator.ValidateConfigurationUpgrade(currentWindowsFabricSettings, targetWindowsFabricSettings);
            }
        }
    }
}