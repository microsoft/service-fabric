// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.Globalization;

    using FileStoreService = FabricValidatorConstants.ParameterNames.FileStoreService;

    /// <summary>
    /// This is the class handles the validation of configuration for the ImageStoreService section.
    /// </summary>
    class FileStoreServiceConfigurationValidator : BaseFabricConfigurationValidator
    {
        public override string SectionName
        {
            get { return FabricValidatorConstants.SectionNames.FileStoreService; }
        }

        public override void ValidateConfiguration(WindowsFabricSettings windowsFabricSettings)
        {
            this.ValidateThumbprint(windowsFabricSettings);

            this.ValidateCommonName(windowsFabricSettings);
        }

        public override void ValidateConfigurationUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {
            this.ValidateCommonName(targetWindowsFabricSettings);

            this.ValidateCommonNameUpgrade(currentWindowsFabricSettings, targetWindowsFabricSettings);
        }

        internal void ValidateThumbprint(WindowsFabricSettings windowsFabricSettings)
        {
            string primaryAccountType = windowsFabricSettings.GetParameter(this.SectionName, FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountType).Value;
            bool isPrimaryDeclared = ValidateThumbprintInformation(
                primaryAccountType,
                windowsFabricSettings.GetParameter(this.SectionName, FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountUserName).Value,
                windowsFabricSettings.GetParameter(this.SectionName, FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountUserPassword).Value,
                windowsFabricSettings.GetParameter(this.SectionName, FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountNTLMPasswordSecret).Value,
                true /*isPrimary*/);

            bool isSecondaryDeclared = ValidateThumbprintInformation(
                windowsFabricSettings.GetParameter(this.SectionName, FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountType).Value,
                windowsFabricSettings.GetParameter(this.SectionName, FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountUserName).Value,
                windowsFabricSettings.GetParameter(this.SectionName, FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountUserPassword).Value,
                windowsFabricSettings.GetParameter(this.SectionName, FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountNTLMPasswordSecret).Value,
                false /*isPrimary*/);

            if(isPrimaryDeclared)
            {
                if (!isSecondaryDeclared &&
                    (SecurityPrincipalsTypeUserAccountType)Enum.Parse(typeof(SecurityPrincipalsTypeUserAccountType), primaryAccountType) != SecurityPrincipalsTypeUserAccountType.ManagedServiceAccount)
                {
                    throw new ArgumentException(StringResources.Error_FileStoreService_SecondaryAccountTypeNotFound);
                }
            }
            else
            {
                if(isSecondaryDeclared)
                {
                    throw new ArgumentException(StringResources.Error_FileStoreService_SecondaryAccountTypeFoundWithoutPrimary);
                }
            }
        }

        internal void ValidateCommonName(WindowsFabricSettings windowsFabricSettings)
        {
            bool isPasswordSecretDeclared = !string.IsNullOrWhiteSpace(windowsFabricSettings.GetParameter(this.SectionName, FileStoreService.CommonNameNtlmPasswordSecret).Value);
            bool isCn1Declared = !string.IsNullOrWhiteSpace(windowsFabricSettings.GetParameter(this.SectionName, FileStoreService.CommonName1Ntlmx509CommonName).Value);
            bool isCn2Declared = !string.IsNullOrWhiteSpace(windowsFabricSettings.GetParameter(this.SectionName, FileStoreService.CommonName2Ntlmx509CommonName).Value);

            if (isCn1Declared != isCn2Declared)
            {
                throw new ArgumentException(StringResources.Error_FileStoreService_InvalidCn);
            }

            if (isPasswordSecretDeclared != isCn1Declared)
            {
                throw new ArgumentException(StringResources.Error_FileStoreService_InvalidCnAndSecret);
            }
        }

        internal void ValidateCommonNameUpgrade(WindowsFabricSettings currentWindowsFabricSettings, WindowsFabricSettings targetWindowsFabricSettings)
        {
            bool isCn1ConfiguredAndChanged = this.GetCommonNameUpgradeInformation(FileStoreService.CommonName1Ntlmx509CommonName, currentWindowsFabricSettings, targetWindowsFabricSettings);
            bool isCn2ConfiguredAndChanged = this.GetCommonNameUpgradeInformation(FileStoreService.CommonName2Ntlmx509CommonName, currentWindowsFabricSettings, targetWindowsFabricSettings);
            if (isCn1ConfiguredAndChanged && isCn2ConfiguredAndChanged)
            {
                throw new ArgumentException(StringResources.Error_FileStoreService_InvalidCnUpgrade);
            }

            string currentSecret = currentWindowsFabricSettings.GetParameter(this.SectionName, FileStoreService.CommonNameNtlmPasswordSecret).Value;
            string targetSecret = targetWindowsFabricSettings.GetParameter(this.SectionName, FileStoreService.CommonNameNtlmPasswordSecret).Value;
            if (!string.IsNullOrWhiteSpace(currentSecret) && !string.IsNullOrWhiteSpace(targetSecret)
                && currentSecret != targetSecret)
            {
                throw new ArgumentException(StringResources.Error_FileStoreService_InvalidCnSecretUpgrade);
            }
        }

        internal bool ValidateThumbprintInformation(string accountType, string accountUserName, string accountUserPassword, string accountNTLMPasswordSecret, bool isPrimary)
        {
            if(string.IsNullOrEmpty(accountType))
            {
                return false;
            }

            SecurityPrincipalsTypeUserAccountType accountTypeEnum;
            if(!Enum.TryParse<SecurityPrincipalsTypeUserAccountType>(accountType, out accountTypeEnum))
            {
                throw new ArgumentException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_FileStoreService_InvalidAccountType,
                        accountType,
                        isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountType : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountType,
                        SecurityPrincipalsTypeUserAccountType.LocalUser,
                        SecurityPrincipalsTypeUserAccountType.DomainUser,
                        SecurityPrincipalsTypeUserAccountType.ManagedServiceAccount));
            }

            if(accountTypeEnum == SecurityPrincipalsTypeUserAccountType.LocalUser)
            {
                if(string.IsNullOrEmpty(accountNTLMPasswordSecret))
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.Error_FileStoreService_RequiredParameterNotSpecified,
                            isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountNTLMPasswordSecret : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountNTLMPasswordSecret,
                            isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountType : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountType,
                            accountType));
                }

                if(!string.IsNullOrEmpty(accountUserName))
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.Error_FileStoreService_ParameterUnexpected,
                            isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountUserName : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountUserName,
                            isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountType : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountType,
                            accountType));
                }

                if (!string.IsNullOrEmpty(accountUserPassword))
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.Error_FileStoreService_ParameterUnexpected,
                            isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountUserPassword : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountUserPassword,
                            isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountType : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountType,
                            accountType));
                }
            }
            else if(accountTypeEnum == SecurityPrincipalsTypeUserAccountType.DomainUser)
            {
                if (string.IsNullOrEmpty(accountUserName))
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.Error_FileStoreService_RequiredParameterNotSpecified,
                            isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountUserName : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountUserName,
                            isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountType : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountType,
                            accountType));
                }

                if (string.IsNullOrEmpty(accountUserPassword))
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.Error_FileStoreService_RequiredParameterNotSpecified,
                            isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountUserPassword : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountUserPassword,
                            isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountType : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountType,
                            accountType));
                }

                if (!string.IsNullOrEmpty(accountNTLMPasswordSecret))
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.Error_FileStoreService_ParameterUnexpected,
                            isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountNTLMPasswordSecret : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountNTLMPasswordSecret,
                            isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountType : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountType,
                            accountType));
                }
            }
            else if(accountTypeEnum == SecurityPrincipalsTypeUserAccountType.ManagedServiceAccount)
            {
                if (string.IsNullOrEmpty(accountUserName))
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.Error_FileStoreService_RequiredParameterNotSpecified,
                            isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountUserName : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountUserName,
                            isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountType : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountType,
                            accountType));
                }

                if (!string.IsNullOrEmpty(accountUserPassword))
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.Error_FileStoreService_ParameterUnexpected,
                            isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountUserPassword : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountUserPassword,
                            isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountType : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountType,
                            accountType));
                }

                if (!string.IsNullOrEmpty(accountNTLMPasswordSecret))
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.Error_FileStoreService_ParameterUnexpected,
                            isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountNTLMPasswordSecret : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountNTLMPasswordSecret,
                            isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountType : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountType,
                            accountType));
                }
            }
            else
            {
                throw new ArgumentException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_FileStoreService_InvalidAccountType,
                        accountType,
                        isPrimary ? FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountType : FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountType,
                        SecurityPrincipalsTypeUserAccountType.LocalUser,
                        SecurityPrincipalsTypeUserAccountType.DomainUser,
                        SecurityPrincipalsTypeUserAccountType.ManagedServiceAccount));
            }

            return true;
        }

        internal bool GetCommonNameUpgradeInformation(
            string parameterName,
            WindowsFabricSettings currentWindowsFabricSettings,
            WindowsFabricSettings targetWindowsFabricSettings)
        {
            string currentCn = currentWindowsFabricSettings.GetParameter(this.SectionName, parameterName).Value;
            string targetCn = targetWindowsFabricSettings.GetParameter(this.SectionName, parameterName).Value;
            return !string.IsNullOrWhiteSpace(currentCn) && !string.IsNullOrWhiteSpace(targetCn) && currentCn != targetCn;
        }       
    }
}