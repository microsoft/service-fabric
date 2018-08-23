// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageStore
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Fabric.Common;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Fabric.Security;
    using System.Globalization;
    using System.IO;
    using System.Security.Cryptography.X509Certificates;
    using System.Text;
    
    /// <summary>
    /// Provides supplementary utility methods for application package deployment
    /// </summary>
    public class ImageStoreUtility
    {
        /// <summary>
        /// <para>
        /// Creates a 'sfpkg' file from the application package found in the specified application package root directory.
        /// </para>
        /// </summary>
        /// <param name="appPackageRootDirectory"><para>The application package root directory. It must exist.</para></param>
        /// <param name="destinationDirectory"><para>The folder where the sfpkg file is placed. If it doesn't exist, it is created.</para></param>
        /// <param name="sfpkgName"><para>The name of the sfpkg file.</para></param>
        /// <param name="applyCompression"><para>If true, the package is
        /// <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-package-apps#compress-a-package">compressed</see>
        /// before generating the sfpkg, inside <paramref name="appPackageRootDirectory"/>.
        /// If the package is already compressed, no changes are made. To compress, the appPackageRootDirectory must allow write access.
        /// If false, the sfpkg is generated using the application package as it is, either compressed or uncompressed. Default: true.</para></param>
        /// <returns><para>The path to the sfpkg file. Can be used to expand the sfpkg using <see cref="System.Fabric.ImageStore.ImageStoreUtility.ExpandSfpkg(string, string)"/>.</para></returns>
        /// <remarks>
        /// <para>
        /// If sfpkgName is not specified or null, the generated package takes the name of the application package root folder, adding the '.sfpkg' extension.
        /// If the specified name has no extension or a different extension, the extension is replaced with '.sfpkg'.
        /// </para>
        /// <para>The destination directory can't be same or a child of the application package root directory, as extraction will fail.</para>
        /// <para>No validation is performed to ensure the package is valid. Please test the package before generating the sfpkg.</para>
        /// <para>The sfpkg can be uploaded to an external store and used for provisioning.</para>
        /// <para>To expand the sfpkg to see the original content, use <see cref="System.Fabric.ImageStore.ImageStoreUtility.ExpandSfpkg(string, string)"/></para>
        /// </remarks>
        public static string GenerateSfpkg(
            string appPackageRootDirectory,
            string destinationDirectory,
            bool applyCompression = true,
            string sfpkgName = null)
        {
            return Utility.WrapNativeSyncInvokeInMTA<string>(() => { return GenerateSfpkgHelper(appPackageRootDirectory, destinationDirectory, applyCompression, sfpkgName); }, "GenerateSfpkg");
        }

        /// <summary>
        /// Expands the specified sfpkg file in the application package folder.
        /// </summary>
        /// <param name="sfpkgFilePath"><para>The path to the sfpkg file, which must exist and have '.sfpkg' extension.</para></param>
        /// <param name="appPackageRootDirectory"><para>The directory where the package is expanded. If it exists, it will be first deleted to ensure the resulting application package is valid.</para></param>
        /// <remarks>
        /// <para>If the destination directory doesn't exist, it is created.</para>
        /// <para>If the destination folder contains the sfpkg to be extracted, the package is extracted in place and, when extraction is complete, the sfpkg is deleted, to ensure the folder is a valid application package. Otherwise, the sfpkg is not deleted.</para>
        /// <para>If the destination folder exists, it must not contain other files or folders.</para>
        /// <para>
        /// The command does not perform any validation on the sfpkg or the expanded package.
        /// </para>
        /// <para>If the sfpkg was created using <see cref="System.Fabric.ImageStore.ImageStoreUtility.GenerateSfpkg(string, string, bool, string)"/>, the <paramref name="sfpkgFilePath"/> is the returned value.</para>
        /// </remarks>
        public static void ExpandSfpkg(
            string sfpkgFilePath,
            string appPackageRootDirectory)
        {
            Utility.WrapNativeSyncInvokeInMTA(() => { ExpandSfpkgHelper(sfpkgFilePath, appPackageRootDirectory); }, "ExpandSfpkg");
        }

        /// <summary>
        /// Compresses all service package sub-directories (i.e. code/config/data packages) found under the application package root directory.
        /// </summary>
        /// <param name="appPackageRootDirectory">The application package root directory.</param>
        /// <param name="progressHandler">An optional handler for tracking progress of the compression operation.</param>
        /// <remarks>
        /// <para>
        /// Package checksums are computed on the resulting compressed package files rather than the original uncompressed contents. This means that when uploading compressed packages that were previously uncompressed, their versions must be changed in the Service Manifest.
        /// Alternatively, simply omit the sub-package whose contents have not changed from the overall application package.
        /// Also note that changing the last modified timestamp of a compressed package's contents will also result in changing the package's checksum.
        /// </para>
        /// </remarks>
        public static void ArchiveApplicationPackage(
            string appPackageRootDirectory,
            IImageStoreProgressHandler progressHandler)
        {
            Utility.WrapNativeSyncInvokeInMTA(()=> { ArchiveApplicationPackageHelper(appPackageRootDirectory, progressHandler); }, "ArchiveApplicationPackage");
        }

        /// <summary>
        /// Uncompresses all service package sub-directories (i.e. code/config/data packages) found under the application package root directory.
        /// </summary>
        /// <param name="appPackageRootDirectory">The application package root directory.</param>
        /// <param name="progressHandler">An optional handler for tracking progress of the uncompression operation.</param>
        /// <returns>
        /// <para>True if any compressed packages were found, false otherwise.</para>
        /// </returns>
        public static bool TryExtractApplicationPackage(
            string appPackageRootDirectory,
            IImageStoreProgressHandler progressHandler)
        {
            return Utility.WrapNativeSyncInvokeInMTA<bool>(()=> { return ExtractApplicationPackageHelper(appPackageRootDirectory, progressHandler); }, "ExtractApplicationPackage");
        }

        /// <summary>
        /// Generates checksum files for all service package sub-directories (i.e. code/config/data packages) and service manifests found under the application package root directory.
        /// </summary>
        /// <param name="appPackageRootDirectory">The application package root directory.</param>
        /// <param name="progressHandler">An optional handler for tracking progress of the checksum generation.</param>
        /// <param name="isImageStoreServiceEnabled">Should be set to true if the cluster's ImageStoreConnectionString is set to "fabric:ImageStore", false otherwise.</param>
        /// <remarks>
        /// <para>
        /// The cluster will automatically generate checksums during application type registration if they're not already part of the application package. This method can be used to incur the cost of checksum file generation while preparing the application package, which will reduce the cost of registering the application type package for large packages.
        /// </para>
        /// </remarks>
        public static void GenerateApplicationPackageChecksumFiles(
            string appPackageRootDirectory, 
            IImageStoreProgressHandler progressHandler,
            bool isImageStoreServiceEnabled = true)
        {
            var layoutSpecification = BuildLayoutSpecification.Create();

            var files = new List<string>();
            var directories = new List<string>();

            foreach (var servicePackageDirectory in FabricDirectory.GetDirectories(appPackageRootDirectory))
            {
                files.AddRange(FabricDirectory.GetFiles(servicePackageDirectory));
                directories.AddRange(FabricDirectory.GetDirectories(servicePackageDirectory));
            }

            int completedItems = 0;
            int totalItems = files.Count + directories.Count;

            foreach (var file in files)
            {
                GenerateChecksumFile(file, layoutSpecification, isImageStoreServiceEnabled);

                try
                {
                    if (progressHandler != null)
                    {
                        progressHandler.UpdateProgress(++completedItems, totalItems, ProgressUnitType.Files);
                    }
                }
                catch (Exception)
                {
                    // Do not fail checksum processing if progress update fails
                }
            }

            foreach (var directory in directories)
            {
                GenerateChecksumFile(directory, layoutSpecification, isImageStoreServiceEnabled);

                try
                {
                    if (progressHandler != null)
                    {
                        progressHandler.UpdateProgress(++completedItems, totalItems, ProgressUnitType.Files);
                    }
                }
                catch (Exception)
                {
                    // Do not fail checksum processing if progress update fails
                }
            }
        }

        /// <summary>
        /// Helper method to determine the Image Store provider type based on an unencrypted Image Store connection string.
        /// </summary>
        /// <param name="imageStoreConnectionString">The unencrypted Image Store connection string found in the cluster manifest.</param>
        /// <returns>
        /// <para>The Image Store provider type.</para>
        /// </returns>
        internal static ImageStoreProviderType GetImageStoreProviderType(string imageStoreConnectionString)
        {
            if (FileImageStore.IsFileStoreUri(imageStoreConnectionString))
            {
                return ImageStoreProviderType.FileShare;
            }
            else if (XStoreProxy.IsXStoreUri(imageStoreConnectionString))
            {
                return ImageStoreProviderType.AzureStorage;
            }
            else if (NativeImageStoreClient.IsNativeImageStoreUri(imageStoreConnectionString))
            {
                return ImageStoreProviderType.ImageStoreService;
            }
            else
            {
                return ImageStoreProviderType.Invalid;
            }
        }

        internal static void WriteStringToFile(string fileName, string value, bool writeLine = true, Encoding encoding = null)
        {
            var directory = FabricPath.GetDirectoryName(fileName); 

            if (!FabricDirectory.Exists(directory))
            {
                FabricDirectory.CreateDirectory(directory);
            }

            if (encoding == null)
            {
#if DotNetCoreClrLinux
                encoding = new UTF8Encoding(false);
#else
                encoding = Encoding.GetEncoding(0);
#endif
            }

            using (StreamWriter writer = new StreamWriter(FabricFile.Open(fileName, FileMode.OpenOrCreate), encoding))
            {
                if (writeLine)
                { 
                    writer.WriteLine(value);
                }
                else
                {
                    writer.Write(value);
                }
#if DotNetCoreClrLinux
                Helpers.UpdateFilePermission(fileName);
#endif
            }
        }

        private static void GenerateChecksumFile(
            string fileOrDirectoryPath, 
            BuildLayoutSpecification layoutSpecification, 
            bool isImageStoreServiceEnabled)
        {
            var checksumFileName = layoutSpecification.GetChecksumFile(fileOrDirectoryPath);

            if (FabricFile.Exists(checksumFileName))
            {
                FabricFile.Delete(checksumFileName);
            }

            var checksumValue = ChecksumUtility.ComputeHash(fileOrDirectoryPath, isImageStoreServiceEnabled);

            WriteStringToFile(checksumFileName, checksumValue);
        }

        #region Interop Helpers

        private static string GenerateSfpkgHelper(
            string appPackageRootDirectory,
            string destinationDirectory,
            bool applyCompression,
            string sfpkgName)
        {
            using (var pin = new PinCollection())
            {
                var nativeResult = NativeImageStore.GenerateSfpkg(
                    pin.AddObject(appPackageRootDirectory),
                    pin.AddObject(destinationDirectory),
                    NativeTypes.ToBOOLEAN(applyCompression),
                    pin.AddObject(sfpkgName));
                
                return StringResult.FromNative(nativeResult);
            }
        }

        private static void ExpandSfpkgHelper(
            string sfpkgFilePath,
            string appPackageRootDirectory)
        {
            using (var pin = new PinCollection())
            {
                NativeImageStore.ExpandSfpkg(
                    pin.AddObject(sfpkgFilePath),
                    pin.AddObject(appPackageRootDirectory));
            }
        }

        private static void ArchiveApplicationPackageHelper(
            string appPackageRootDirectory,
            IImageStoreProgressHandler progressHandler)
        {
            using (var pin = new PinCollection())
            {
                NativeImageStore.ArchiveApplicationPackage(
                    pin.AddObject(appPackageRootDirectory),
                    new NativeImageStoreProgressEventHandlerBroker(progressHandler));
            }
        }

        private static bool ExtractApplicationPackageHelper(
            string appPackageRootDirectory,
            IImageStoreProgressHandler progressHandler)
        {
            using (var pin = new PinCollection())
            {
                return NativeImageStore.TryExtractApplicationPackage(
                    pin.AddObject(appPackageRootDirectory),
                    new NativeImageStoreProgressEventHandlerBroker(progressHandler));
            }
        }

        #endregion
    };
}