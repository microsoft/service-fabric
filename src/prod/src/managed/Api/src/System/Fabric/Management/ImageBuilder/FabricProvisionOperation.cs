// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.Interop;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Text;
    using System.Xml;
    using System.Linq;
    using System.Fabric.FabricDeployer;
    using System.Fabric.Management.FabricDeployer;

    internal class FabricProvisionOperation
    {
        private static readonly string TraceType = "FabricProvisionOperation";

        private static readonly string[] ChildFoldersInXcopyPackage = { "bin", "Eula", "FabricInstallerService.Code" };

        private ImageStoreWrapper imageStoreWrapper;
        private XmlReaderSettings validatingXmlReaderSettings;

        public FabricProvisionOperation(XmlReaderSettings validatingXmlReaderSettings, ImageStoreWrapper storeWrapper)
        {
            this.validatingXmlReaderSettings = validatingXmlReaderSettings;
            this.imageStoreWrapper = storeWrapper;
        }

        public void ProvisionFabric(
            string localCodePath,
            string localConfigPath,
#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
            string configurationCsvFilePath,
#endif
            string infrastructureManifestFilePath,
            bool validateOnly,
            TimeSpan timeout)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);

#if DotNetCoreClrLinux || DotNetCoreClrIOT
            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Starting ProvisionFabric. CodePath:{0}, ConfigPath:{1}, InfrastructureManifestFilePath:{2}, Timeout:{3}",
                localCodePath,
                localConfigPath,
                infrastructureManifestFilePath,
                timeoutHelper.GetRemainingTime());
#else            
            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Starting ProvisionFabric. CodePath:{0}, ConfigPath:{1}, ConfigurationCsvFilePath:{2}, InfrastructureManifestFilePath:{3}, Timeout:{4}",
                localCodePath,
                localConfigPath,
                configurationCsvFilePath,
                infrastructureManifestFilePath,
                timeoutHelper.GetRemainingTime());
#endif

            // TODO: Once FabricTest has been modified to generate InfrastructureManifest.xml file, we should not
            // allow InfrastructureManifest file to be optional
            if (string.IsNullOrEmpty(infrastructureManifestFilePath) || !FabricFile.Exists(infrastructureManifestFilePath))
            {
                infrastructureManifestFilePath = null;
                ImageBuilder.TraceSource.WriteWarning(
                    TraceType,
                    "The InfrastrucutreManifestFile:{0} is not found",
                    infrastructureManifestFilePath);
            }

            string codeVersion = string.Empty, clusterManifestVersion = string.Empty;

            if (!string.IsNullOrEmpty(localCodePath))
            {
                codeVersion = GetCodeVersion(localCodePath);
            }

            if (!string.IsNullOrEmpty(localConfigPath))
            {
                try
                {
                    var parameters = new Dictionary<string, dynamic>
                    {
                        {DeploymentParameters.ClusterManifestString, localConfigPath},
                        {DeploymentParameters.InfrastructureManifestString, infrastructureManifestFilePath}
                    };

                    var deploymentParameters = new DeploymentParameters();
                    deploymentParameters.SetParameters(parameters, DeploymentOperations.ValidateClusterManifest);
                    DeploymentOperation.ExecuteOperation(deploymentParameters);
                }
                catch (Exception e)
                {
                    ImageBuilderUtility.TraceAndThrowValidationError(
                        FabricProvisionOperation.TraceType,
                        StringResources.ImageBuilderError_ClusterManifestValidationFailed,
                        e.ToString(),
                        localConfigPath);
                }
            }

            if (!validateOnly)
            {
                timeoutHelper.ThrowIfExpired();

                WinFabStoreLayoutSpecification winFabLayout = WinFabStoreLayoutSpecification.Create();
                if (!string.IsNullOrEmpty(localCodePath))
                {
                    // Upload MSP file
                    string destinationCodePath;
                    if (ImageBuilderUtility.IsInstaller(localCodePath))
                    {
                        destinationCodePath = winFabLayout.GetPatchFile(codeVersion);
                    }
                    else if (ImageBuilderUtility.IsCabFile(localCodePath))
                    {
                        // Upload CAB file
                        destinationCodePath = winFabLayout.GetCabPatchFile(codeVersion);
                    }
                    else
                    {
                        destinationCodePath = winFabLayout.GetCodePackageFolder(codeVersion);
                    }

                    this.imageStoreWrapper.UploadContent(destinationCodePath, localCodePath, timeoutHelper.GetRemainingTime());
                }

                ClusterManifestType clusterManifest = null;
                if (!string.IsNullOrEmpty(localConfigPath))
                {
                    clusterManifest = ImageBuilderUtility.ReadXml<ClusterManifestType>(localConfigPath, this.validatingXmlReaderSettings);
                    clusterManifestVersion = clusterManifest.Version;
                }

                if (clusterManifest != null)
                {
                    // Upload ClusterManifest file

                    ReplaceDefaultImageStoreConnectionString(clusterManifest);
                    
                    string destinationConfigPath = winFabLayout.GetClusterManifestFile(clusterManifestVersion);
                    imageStoreWrapper.SetToStore<ClusterManifestType>(destinationConfigPath, clusterManifest, timeoutHelper.GetRemainingTime());
                }
            }

#if DotNetCoreClrLinux || DotNetCoreClrIOT
            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Completed ProvisionFabric. CodePath:{0}, ConfigPath:{1}",
                localCodePath,
                localConfigPath);
#else
            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Completed ProvisionFabric. CodePath:{0}, ConfigPath:{1}, ConfigurationCsvFilePath:{2}",
                localCodePath,
                localConfigPath,
                configurationCsvFilePath);
#endif
        }

        internal static string GetCodeVersion(string fabricCodePackagePath)
        {
            string codeVersion;

            // Check if the code package is a MSI
            if (ImageBuilderUtility.IsInstaller(fabricCodePackagePath))
            {
#if DotNetCoreClrLinux
                codeVersion = GetProductVersionLinux(fabricCodePackagePath);
#else
                if (!TryQueryProductVersionFromMsi(fabricCodePackagePath, out codeVersion))
                {
                    // If query of MSI to get ProductVersion fails, fallback to trying
                    // to get version information from file name
                    codeVersion = ParseFileNameForProductVersion(fabricCodePackagePath);
                }
#endif
            }
            // WU CAB & XCopy CAB
            else if (ImageBuilderUtility.IsCabFile(fabricCodePackagePath))
            {
                if (!TryQueryProductVersionFromCab(fabricCodePackagePath, out codeVersion))
                {
                    // If query of CAB to get Version fails, fallback to trying
                    // to get version information using other means
#if DotNetCoreClrLinux
                    codeVersion = GetProductVersionLinux(fabricCodePackagePath);
#else
                    codeVersion = ParseFileNameForProductVersion(fabricCodePackagePath);
#endif
                }
            }
            else // XCOPY deprecated package
            {
                FabricProvisionOperation.ValidateXCopyPackageAndGetVersion(fabricCodePackagePath, out codeVersion);
            }

            return codeVersion;
        }

        private void ReplaceDefaultImageStoreConnectionString(ClusterManifestType manifest)
        {
            SettingsOverridesTypeSection managementSection = manifest.FabricSettings.FirstOrDefault(
                section => section.Name.Equals(System.Fabric.FabricDeployer.Constants.SectionNames.Management, StringComparison.OrdinalIgnoreCase));

            SettingsOverridesTypeSectionParameter imageStoreParameter = null;
            if (managementSection != null)
            {
                imageStoreParameter = managementSection.Parameter.FirstOrDefault(
                    parameter => parameter.Name.Equals(System.Fabric.FabricDeployer.Constants.ParameterNames.ImageStoreConnectionString, StringComparison.OrdinalIgnoreCase));
            }

            if (imageStoreParameter == null)
            {
                return;
            }

            if (string.IsNullOrEmpty(imageStoreParameter.Value) || 
                imageStoreParameter.Value.Equals(System.Fabric.FabricDeployer.Constants.DefaultTag, StringComparison.OrdinalIgnoreCase))
            {
                string defaultImageStoreRoot = Path.Combine(FabricEnvironment.GetDataRoot(), System.Fabric.FabricDeployer.Constants.DefaultImageStoreRootFolderName);
                imageStoreParameter.Value = string.Format(CultureInfo.InvariantCulture, "{0}{1}", System.Fabric.FabricValidatorConstants.FileImageStoreConnectionStringPrefix, defaultImageStoreRoot);
            }
        }

        private static bool TryQueryProductVersionFromCab(string fileName, out string productVersion)
        {
            productVersion = CabFileOperations.GetCabVersion(fileName);
            return !string.IsNullOrWhiteSpace(productVersion);
        }

        private static bool TryQueryProductVersionFromMsi(string fileName, out string productVersion)
        {
            productVersion = string.Empty;

            string sqlStatement = "SELECT * FROM Property WHERE Property = 'ProductVersion'";
            IntPtr databaseHandle = IntPtr.Zero;
            IntPtr viewHandle = IntPtr.Zero;
            IntPtr recordHandle = IntPtr.Zero;

            StringBuilder productVersionBuffer = new StringBuilder(255);
            int productVersionBufferSize = 255;

            try
            {                
                // Open the MSI database in the input file 
                uint val = NativeHelper.MsiOpenDatabase(fileName, IntPtr.Zero, out databaseHandle);
                if (val != 0)
                {
                    int lastError = Marshal.GetLastWin32Error();
                    ImageBuilder.TraceSource.WriteWarning(
                        TraceType,
                        "MsiOpenDatabase failed with {0}",
                        lastError);
                    return false;
                }

                recordHandle = NativeHelper.MsiCreateRecord(1);

                // Open a view on the Property table for the version property 
                int viewVal = NativeHelper.MsiDatabaseOpenViewW(databaseHandle, sqlStatement, out viewHandle);
                if (viewVal != 0)
                {
                    int lastError = Marshal.GetLastWin32Error();
                    ImageBuilder.TraceSource.WriteWarning(
                        TraceType,
                        "MsiDatabaseOpenViewW failed with {0}",
                        lastError);
                    return false;
                }

                // Execute the view query 
                int exeVal = NativeHelper.MsiViewExecute(viewHandle, recordHandle);
                if (exeVal != 0)
                {
                    int lastError = Marshal.GetLastWin32Error();
                    ImageBuilder.TraceSource.WriteWarning(
                        TraceType,
                        "MsiViewExecute failed with {0}",
                        lastError);
                    return false;
                }

                // Get the record from the view 
                uint fetchVal = NativeHelper.MsiViewFetch(viewHandle, out recordHandle);
                if (fetchVal != 0)
                {
                    int lastError = Marshal.GetLastWin32Error();
                    ImageBuilder.TraceSource.WriteWarning(
                        TraceType,
                        "MsiViewFetch failed with {0}",
                        lastError);
                    return false;
                }

                // Get the version from the data 
                int retVal = NativeHelper.MsiRecordGetString(recordHandle, 2, productVersionBuffer, ref productVersionBufferSize);
                if (retVal != 0)
                {
                    int lastError = Marshal.GetLastWin32Error();
                    ImageBuilder.TraceSource.WriteWarning(
                        TraceType,
                        "MsiRecordGetString failed with {0}",
                        lastError);
                    return false;
                }
            }
            finally
            {
                if (databaseHandle != IntPtr.Zero) { NativeHelper.MsiCloseHandle(databaseHandle); }
                if (viewHandle != IntPtr.Zero) { NativeHelper.MsiCloseHandle(viewHandle); }
                if (recordHandle != IntPtr.Zero) { NativeHelper.MsiCloseHandle(recordHandle); }
            }

            productVersion = productVersionBuffer.ToString();
            return true;
        }

#if DotNetCoreClrLinux
        private static string GetProductVersionLinux(string codePath)
        {
            var packageManagerType = FabricEnvironment.GetLinuxPackageManagerType();
            switch (packageManagerType)
            {
                case LinuxPackageManagerType.Deb:
                    return GetProductVersionFromFilename(codePath, StringConstants.Deb);
                case LinuxPackageManagerType.Rpm:
                    return GetProductVersionFromFilename(codePath, StringConstants.Rpm);
                default:
                    ImageBuilderUtility.TraceAndThrowValidationError(
                        TraceType,
                        StringResources.ImageBuilderError_LinuxPackageManagerUnknown,
                        (int)packageManagerType);
                    return null; // To please compiler
            }
        }

        private static string GetProductVersionFromFilename(string codePath, string expectedFileSuffix)
        {
            string fileName = Path.GetFileName(codePath);
            string[] firstTokens = fileName.Split('_');
            if (firstTokens.Length != 2 || !ImageBuilderUtility.Equals(firstTokens[0], StringConstants.ServiceFabric))
            {
                ThrowInvalidCodeUpgradeFileName(fileName);
            }

            string[] secondTokens = firstTokens[1].Split('.');
            if (secondTokens.Length != 5 || !ImageBuilderUtility.Equals(secondTokens[4], expectedFileSuffix))
            {
                ThrowInvalidCodeUpgradeFileName(fileName);
            }

            for (int i = 0; i < secondTokens.Length - 1; i++)
            {
                int value;
                if (!int.TryParse(secondTokens[i], out value))
                {
                    ThrowInvalidCodeUpgradeFileName(fileName);
                }
            }

            return string.Format(CultureInfo.InvariantCulture, "{0}.{1}.{2}.{3}", secondTokens[0], secondTokens[1], secondTokens[2], secondTokens[3]);
        }
#endif

        private static string ParseFileNameForProductVersion(string codePath)
        {
            string fileName = Path.GetFileName(codePath);
            string[] tokens = fileName.Split('.');
            if (tokens.Length != 6 ||
                !ImageBuilderUtility.Equals(tokens[0], StringConstants.WindowsFabric) ||
                !ImageBuilderUtility.Equals(tokens[5], StringConstants.Msi))
            {
                ThrowInvalidCodeUpgradeFileName(fileName);
            }

            for (int i = 1; i < tokens.Length - 1; i++)
            {
                int value;
                if (!int.TryParse(tokens[i], out value))
                {
                    ThrowInvalidCodeUpgradeFileName(fileName);
                }
            }

            return string.Format(CultureInfo.InvariantCulture, "{0}.{1}.{2}.{3}", tokens[1], tokens[2], tokens[3], tokens[4]);
        }

        private static void ThrowInvalidCodeUpgradeFileName(string fileName)
        {
            ImageBuilderUtility.TraceAndThrowValidationError(
                    FabricErrorCode.ImageBuilderInvalidMsiFile,
                    TraceType,
                    StringResources.ImageBuilderError_InvalidMSIFileName,
                    fileName);
        }

        private static void ValidateXCopyPackageAndGetVersion(string packagePath, out string codeVersion)
        {
            foreach (string childFolder in ChildFoldersInXcopyPackage)
            {
                string newPath = Path.Combine(packagePath, childFolder);
                if(!FabricDirectory.Exists(newPath))
                {
                    ImageBuilderUtility.TraceAndThrowValidationError(
                        FabricProvisionOperation.TraceType,
                        StringResources.ImageStoreError_ChildFolderDoesNotExist_Formatted,
                        childFolder,
                        packagePath);
                }
            }

            codeVersion = "0.0.0.0";
            var fabricExeCollection = FabricDirectory.GetFiles(packagePath, "fabric.exe", true, SearchOption.AllDirectories);
            if (fabricExeCollection.Count() == 0)
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    FabricProvisionOperation.TraceType,
                    StringResources.ImageStoreError_FabricExeNotFound);
            }

            // Build number in version might have leading 0's i.e. 3.0.00123.0
            // This causes issues during download, hence trimming the leading 0's
#if DotNetCoreClrLinux
            var versionString = FileVersionInfo.GetVersionInfo(fabricExeCollection.First()).ProductVersion;
#else
            var versionString = FabricFile.GetVersionInfo(fabricExeCollection.First());
#endif
            Version productVersion = null;
            if(!Version.TryParse(versionString, out productVersion))
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    FabricProvisionOperation.TraceType,
                    StringResources.ImageStoreError_InvalidProductVersion,
                    versionString);
            }

            codeVersion = productVersion.ToString();
        }
    }
}