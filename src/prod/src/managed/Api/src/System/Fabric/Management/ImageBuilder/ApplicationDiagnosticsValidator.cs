// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Linq;

    internal class ApplicationDiagnosticsValidator : IApplicationValidator
    {
        private static readonly string TraceType = "ApplicationDiagnosticsValidator";
        private static readonly string CrashDumpSource = "CrashDumpSource";
        private static readonly string ETWSource = "ETWSource";
        private static readonly string FolderSource = "FolderSource";
        private static readonly string AzureBlob = "AzureBlob";
        private static readonly string FileStore = "FileStore";
        private static readonly string LocalStore = "LocalStore";
        private static readonly string IsEnabled = "IsEnabled";
        private static readonly string RelativeFolderPath = "RelativeFolderPath";
        private static readonly string DataDeletionAgeInDays = "DataDeletionAgeInDays";
        private static readonly string UploadIntervalInMinutes = "UploadIntervalInMinutes";
        private static readonly string LevelFilter = "LevelFilter";
        private static readonly string AccountType = "AccountType";
        private static readonly string AccountName = "AccountName";
        private static readonly string Password = "Password";
        private static readonly string PasswordEncrypted = "PasswordEncrypted";
        private static readonly string FileStorePath = "Path";
        
        private bool parametersAlreadyApplied;

        public void Validate(ApplicationTypeContext applicationTypeContext, bool isComposeDeployment, bool isSFVolumeDiskServiceEnabled)
        {
            this.parametersAlreadyApplied = false;
            ValidateApplicationManifestDiagnostics(applicationTypeContext.ApplicationManifest.Diagnostics);
            ValidateServiceManifestDiagnostics(applicationTypeContext.GetServiceManifestTypes());
        }

        public void Validate(ApplicationInstanceContext applicationInstanceContext)
        {
            this.parametersAlreadyApplied = true;
            ValidateApplicationManifestDiagnostics(applicationInstanceContext.ApplicationPackage.DigestedEnvironment.Diagnostics);
        }

        private bool ShouldValidate(string value)
        {
            return (!ImageBuilderUtility.IsParameter(value)) || (this.parametersAlreadyApplied);
        }

        private T ValidateType<T>(string value, string source, string destination, string elementName)
        {
            if (value == null)
            {
                return default(T);
            }

            if (!this.ShouldValidate(value))
            {
                return default(T);
            }

            T convertedValue = default(T);
            try
            {
                convertedValue = ImageBuilderUtility.ConvertString<T>(value);
            }
            catch (Exception)
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.ImageBuilderError_DiagnosticValidator_ValueCannotBeConverted,
                    value,
                    typeof(T),
                    source,
                    (destination != null) ?
                      destination :
                      String.Empty,
                    elementName);
            }

            return convertedValue;
        }

        private void ValidatePositiveInteger(string value, string source, string destination, string elementName)
        {
            if (value == null)
            {
                return;
            }

            if (!this.ShouldValidate(value))
            {
                return;
            }

            int convertedValue = ValidateType<int>(value, source, destination, elementName);
            if (convertedValue <= 0)
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.ImageBuilderError_DiagnosticValidator_ValueMustBeGreaterThanZero,                    
                    value,
                    source,
                    (destination != null) ?
                      destination :
                      String.Empty,
                    elementName);
            }
        }

        enum ETWTraceLevel
        {
            Error,
            Warning,
            Informational,
            Verbose
        }

        private void ValidateLevelFilter(string value, string source, string destination, string elementName)
        {
            if (value == null)
            {
                return;
            }

            if (!this.ShouldValidate(value))
            {
                return;
            }

            ETWTraceLevel etwTraceLevel;
            if (!Enum.TryParse(value, true, out etwTraceLevel))
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.ImageBuilderError_DiagnosticValidator_InvalidETWLevel,                    
                    value,
                    ETWTraceLevel.Error,
                    ETWTraceLevel.Warning,
                    ETWTraceLevel.Informational,
                    ETWTraceLevel.Verbose,
                    source,
                    (destination != null) ?
                      destination :
                      String.Empty,
                    elementName);
            }
        }

        private void ValidateRelativePath(string value, string source, string destination, string elementName)
        {
            if (value == null)
            {
                return;
            }

            if (!this.ShouldValidate(value))
            {
                return;
            }

            if (Path.IsPathRooted(value))
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.ImageBuilderError_DiagnosticValidator_AbsolutePathNotAllowed,
                    value,
                    source,
                    (destination != null) ?
                      destination :
                      String.Empty,
                    elementName);
            }

            string remainingPath = value;
            while (!String.IsNullOrEmpty(remainingPath))
            {
                string pathPart = Path.GetFileName(remainingPath);
                if (pathPart.Equals(StringConstants.DoubleDot, StringComparison.Ordinal))
                {
                    ImageBuilderUtility.TraceAndThrowValidationError(
                        TraceType,
                        StringResources.ImageBuilderError_DiagnosticValidator_InvalidRelativePath,
                        value,
                        source,
                        (destination != null) ?
                          destination :
                          String.Empty,
                        elementName);
                }
                remainingPath = FabricPath.GetDirectoryName(remainingPath);
            }
        }

        private void ValidateApplicationManifestDiagnostics(DiagnosticsType diagnostics)
        {
            if (diagnostics == null)
            {
                return;
            }

            if (diagnostics.CrashDumpSource != null)
            {
                ValidateType<bool>(
                    diagnostics.CrashDumpSource.IsEnabled, 
                    CrashDumpSource, 
                    null, 
                    IsEnabled);
                if (diagnostics.CrashDumpSource.Destinations != null)
                {
                    ValidateAzureBlob(diagnostics.CrashDumpSource.Destinations.AzureBlob, CrashDumpSource);
                    ValidateFileStore(diagnostics.CrashDumpSource.Destinations.FileStore, CrashDumpSource);
                    ValidateLocalStore(diagnostics.CrashDumpSource.Destinations.LocalStore, CrashDumpSource);
                }
            }
            if (diagnostics.ETWSource != null)
            {
                ValidateType<bool>(
                    diagnostics.ETWSource.IsEnabled, 
                    ETWSource, 
                    null, 
                    IsEnabled);
                if (diagnostics.ETWSource.Destinations != null)
                {
                    ValidateAzureBlob(diagnostics.ETWSource.Destinations.AzureBlob, ETWSource);
                    ValidateFileStore(diagnostics.ETWSource.Destinations.FileStore, ETWSource);
                    ValidateLocalStore(diagnostics.ETWSource.Destinations.LocalStore, ETWSource);
                }
            }
            if (diagnostics.FolderSource != null)
            {
                foreach (DiagnosticsTypeFolderSource folderSource in diagnostics.FolderSource)
                {
                    ValidateType<bool>(
                        folderSource.IsEnabled,
                        FolderSource, 
                        null,
                        IsEnabled);
                    ValidateRelativePath(
                        folderSource.RelativeFolderPath,
                        FolderSource,
                        null,
                        RelativeFolderPath);
                    ValidatePositiveInteger(
                        folderSource.DataDeletionAgeInDays,
                        FolderSource, 
                        null,
                        DataDeletionAgeInDays);
                    if (folderSource.Destinations != null)
                    {
                        ValidateAzureBlob(folderSource.Destinations.AzureBlob, FolderSource);
                        ValidateFileStore(folderSource.Destinations.FileStore, FolderSource);
                        ValidateLocalStore(folderSource.Destinations.LocalStore, FolderSource);
                    }
                }
            }
       }

        private void ValidateAzureStore(AzureStoreBaseType azureStore, string source, string destination)
        {
            ValidateType<bool>(
                azureStore.IsEnabled,
                source,
                destination,
                IsEnabled);
            ValidateType<bool>(
                azureStore.ConnectionStringIsEncrypted,
                source,
                destination,
                IsEnabled);
            ValidatePositiveInteger(
                azureStore.UploadIntervalInMinutes,
                source,
                destination,
                UploadIntervalInMinutes);
            ValidatePositiveInteger(
                azureStore.DataDeletionAgeInDays,
                source,
                destination,
                DataDeletionAgeInDays);
        }

        private void ValidateAzureBlob(AzureBlobType[] azureBlobs, string source)
        {
            if (azureBlobs == null)
            {
                return;
            }
            foreach (AzureBlobType azureBlob in azureBlobs)
            {
                ValidateAzureStore((AzureStoreBaseType)azureBlob, source, AzureBlob);
            }
        }

        private void ValidateAzureBlob(AzureBlobETWType[] azureBlobs, string source)
        {
            if (azureBlobs == null)
            {
                return;
            }
            foreach (AzureBlobETWType azureBlob in azureBlobs)
            {
                ValidateAzureStore((AzureBlobType)azureBlob, source, AzureBlob);
                ValidateLevelFilter(azureBlob.LevelFilter, source, AzureBlob, LevelFilter);
            }
        }

        private void ValidateAccountName(string value, string source, string destination, string elementName)
        {
            if (value == null)
            {
                return;
            }

            if (!this.ShouldValidate(value))
            {
                return;
            }

            if (!ImageBuilderUtility.IsValidAccountName(value))
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.ImageBuilderError_DiagnosticValidator_InvalidAccountNameFormat,
                    value,
                    source,
                    (destination != null) ?
                      destination :
                      String.Empty,
                    elementName);
            }
        }

        private void ValidateDomainUser(FileStoreType fileStore, string source)
        {
            ValidateAccountName(fileStore.AccountName, source, FileStore, AccountName);
            ValidateType<bool>(
                fileStore.PasswordEncrypted,
                source,
                FileStore,
                PasswordEncrypted);
        }

        private void ValidateManagedServiceAccount(FileStoreType fileStore, string source)
        {
            if ((this.ShouldValidate(fileStore.Password)) && 
                (!String.IsNullOrEmpty(fileStore.Password)))
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.ImageBuilderError_DiagnosticValidator_PasswordAttributeInvalid,
                    Password,
                    AccountType,
                    FileStoreAccessAccountType.ManagedServiceAccount,
                    source,
                    FileStore);
            }
            if ((this.ShouldValidate(fileStore.PasswordEncrypted)) &&
                (!String.IsNullOrEmpty(fileStore.PasswordEncrypted)))
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.ImageBuilderError_DiagnosticValidator_PasswordAttributeInvalid,
                    PasswordEncrypted,
                    AccountType,
                    FileStoreAccessAccountType.ManagedServiceAccount,
                    source,
                    FileStore);
            }
            ValidateAccountName(fileStore.AccountName, source, FileStore, AccountName);
        }

        private void ValidateFileStorePath(FileStoreType fileStore, string source)
        {
            if (!this.ShouldValidate(fileStore.Path))
            {
                return;
            }
            if (String.IsNullOrEmpty(fileStore.Path))
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.ImageBuilderError_DiagnosticValidator_EmptyFilePathNotAllowed,
                    source,
                    FileStore,
                    FileStorePath);
            }
            try
            {
                Uri uri = new Uri(fileStore.Path);
            }
            catch (Exception e)
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.ImageBuilderError_DiagnosticValidator_PathNotValidUri,
                    fileStore.Path,
                    source,
                    FileStore,
                    FileStorePath,
                    e);
            }
        }

        enum FileStoreAccessAccountType
        {
            DomainUser,
            ManagedServiceAccount
        }

        private void ValidateFileStoreAccessInformation(FileStoreType fileStore, string source)
        {
            if (String.IsNullOrEmpty(fileStore.AccountType))
            {
                if ((this.ShouldValidate(fileStore.AccountName)) && 
                    (!String.IsNullOrEmpty(fileStore.AccountName)))
                {
                    ImageBuilderUtility.TraceAndThrowValidationError(
                        TraceType,
                        StringResources.ImageBuilderError_DiagnosticValidator_AttributeInvalid,
                        AccountName,
                        AccountType,
                        source,
                        FileStore);
                }
                if ((this.ShouldValidate(fileStore.Password)) && 
                    (!String.IsNullOrEmpty(fileStore.Password)))
                {
                    ImageBuilderUtility.TraceAndThrowValidationError(
                        TraceType,
                        StringResources.ImageBuilderError_DiagnosticValidator_AttributeInvalid,
                        Password,
                        AccountType,
                        source,
                        FileStore);
                }
                if ((this.ShouldValidate(fileStore.PasswordEncrypted)) && 
                    (!String.IsNullOrEmpty(fileStore.PasswordEncrypted)))
                {
                    ImageBuilderUtility.TraceAndThrowValidationError(
                        TraceType,
                        StringResources.ImageBuilderError_DiagnosticValidator_AttributeInvalid,
                        PasswordEncrypted,
                        AccountType,
                        source,
                        FileStore);
                }
                return;
            }

            if (!this.ShouldValidate(fileStore.AccountType))
            {
                return;
            }

            FileStoreAccessAccountType accountType;
            if (!Enum.TryParse(fileStore.AccountType, out accountType))
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.ImageBuilderError_DiagnosticValidator_InvalidAccountType,
                    fileStore.AccountType,
                    FileStoreAccessAccountType.DomainUser,
                    FileStoreAccessAccountType.ManagedServiceAccount,
                    source,
                    FileStore,
                    AccountType);
            }
            switch (accountType)
            {
                case FileStoreAccessAccountType.DomainUser:
                    {
                        ValidateDomainUser(fileStore, source);
                    }
                    break;

                case FileStoreAccessAccountType.ManagedServiceAccount:
                    {
                        ValidateManagedServiceAccount(fileStore, source);
                    }
                    break;
            }
        }

        private void ValidateFileStore(FileStoreType fileStore, string source)
        {
            ValidateType<bool>(
                fileStore.IsEnabled,
                source,
                FileStore,
                IsEnabled);
            ValidateFileStorePath(fileStore, source);
            ValidateFileStoreAccessInformation(fileStore, source);
            ValidatePositiveInteger(
                fileStore.UploadIntervalInMinutes,
                source,
                FileStore,
                UploadIntervalInMinutes);
            ValidatePositiveInteger(
                fileStore.DataDeletionAgeInDays,
                source,
                FileStore,
                DataDeletionAgeInDays);
        }

        private void ValidateFileStore(FileStoreType[] fileStores, string source)
        {
            if (fileStores == null)
            {
                return;
            }
            foreach (FileStoreType fileStore in fileStores)
            {
                ValidateFileStore(fileStore, source);
            }
        }

        private void ValidateFileStore(FileStoreETWType[] fileStores, string source)
        {
            if (fileStores == null)
            {
                return;
            }
            foreach (FileStoreETWType fileStore in fileStores)
            {
                ValidateFileStore((FileStoreType)fileStore, source);
                ValidateLevelFilter(fileStore.LevelFilter, source, FileStore, LevelFilter);
            }
        }

        private void ValidateLocalStore(LocalStoreType localStore, string source)
        {
            ValidateType<bool>(
                localStore.IsEnabled,
                source,
                LocalStore,
                IsEnabled);
            ValidateRelativePath(
                localStore.RelativeFolderPath,
                source,
                LocalStore,
                RelativeFolderPath);
            ValidatePositiveInteger(
                localStore.DataDeletionAgeInDays,
                source,
                LocalStore,
                DataDeletionAgeInDays);
        }

        private void ValidateLocalStore(LocalStoreType[] localStores, string source)
        {
            if (localStores == null)
            {
                return;
            }
            foreach (LocalStoreType localStore in localStores)
            {
                ValidateLocalStore(localStore, source);
            }
            return;
        }

        private void ValidateLocalStore(LocalStoreETWType[] localStores, string source)
        {
            if (localStores == null)
            {
                return;
            }
            foreach (LocalStoreETWType localStore in localStores)
            {
                ValidateLocalStore((LocalStoreType)localStore, source);
                ValidateLevelFilter(localStore.LevelFilter, source, LocalStore, LevelFilter);
            }
            return;
        }

        private void ValidateServiceManifestDiagnostics(IEnumerable<ServiceManifestType> serviceManifestTypes)
        {
            foreach (ServiceManifestType serviceManifestType in serviceManifestTypes)
            {
                ValidateServiceManifestDiagnostics(serviceManifestType);
            }
        }

        private void ValidateServiceManifestDiagnostics(ServiceManifestType serviceManifestType)
        {
            if ((null == serviceManifestType.Diagnostics) ||
                (null == serviceManifestType.Diagnostics.ETW) ||
                (null == serviceManifestType.Diagnostics.ETW.ManifestDataPackages))
            {
                return;
            }

            DataPackageType[] dataPackageTypes = serviceManifestType.Diagnostics.ETW.ManifestDataPackages;
            foreach (DataPackageType dataPackageType in dataPackageTypes)
            {
                Version version;
                if (!Version.TryParse(dataPackageType.Version, out version))
                {
                    ImageBuilderUtility.TraceAndThrowValidationError(
                        TraceType,
                        StringResources.ImageBuilderError_DiagnosticValidator_InvalidETWManifestDataPackageVersion,
                        dataPackageType.Version,
                        dataPackageType.Name,
                        serviceManifestType.Name);
                }
            }
        }
    }
}