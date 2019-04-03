// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Fabric.Common;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.IO;
    using System.IO.Compression;
    using System.Linq;
    using System.Text;
    using System.Xml;
    using System.Xml.Serialization;
    using System.Threading.Tasks;

    internal static partial class ImageBuilderUtility
    {
        internal static readonly TimeSpan ImageStoreDefaultTimeout = TimeSpan.FromMinutes(10);
        private static HashSet<char> InvalidFileNameChars = new HashSet<char>(Path.GetInvalidFileNameChars().ToList<char>());
        private static HashSet<char> InvalidPathChars = new HashSet<char>(Path.GetInvalidPathChars().ToList<char>());

        private const string TraceType = "ImageBuilderUtility";

        public static T ReadXml<T>(string fileName, XmlReaderSettings validatingXmlReaderSettings)
        {
#if !DotNetCoreClr
            // Ensure the XmlResolve is set to null
            validatingXmlReaderSettings.XmlResolver = null;
#endif

            using (var stream = FabricFile.Open(fileName, FileMode.Open, FileAccess.Read, FileShare.Read))
            using (var xmlReader = XmlReader.Create(stream, validatingXmlReaderSettings))
            {
                T v = ReadXml<T>(fileName, xmlReader);
#if DotNetCoreClr
                try
                {
                    var applicationManifestType = v as IXmlValidator ;
                    if (applicationManifestType != null)
                    {
                        applicationManifestType.Validate();
                    }
                }
                catch (InvalidOperationException xmlParseException)
                {
                    if (FabricFile.Exists(fileName))
                    {
                        ImageBuilder.TraceSource.WriteError(
                            TraceType,
                            "Invalid XML Content. File Size: {0}",
                            FabricFile.GetSize(fileName));
                    }
                    else
                    {
                        ImageBuilder.TraceSource.WriteError(
                            TraceType,
                            "File '{0}' does not exist.",
                            fileName);
                    }

                    TraceAndThrowValidationErrorWithFileName(
                        xmlParseException,
                        TraceType,
                        fileName,
                        StringResources.ImageBuilderError_ErrorParsingXmlFile,
                        fileName);
                }
                catch(XmlException xmlException)
                {
                    TraceAndThrowValidationErrorWithFileName(
                        xmlException,
                        TraceType,
                        fileName,
                        StringResources.ImageBuilderError_ErrorParsingXmlFile,
                        fileName);
                }
#endif
                return v;
            }
        }

        public static T ReadXml<T>(string fileName)
        {
            if (FabricDirectory.Exists(fileName))
            {
                TraceAndThrowValidationError(
                    TraceType,
                    StringResources.ImageBuilderError_InvalidXmlFile,
                    fileName);
            }

            using (var stream = FabricFile.Open(fileName, FileMode.Open, FileAccess.Read, FileShare.Read))
#if DotNetCoreClr
            using (var xmlReader = XmlReader.Create(stream))
#else
            using (var xmlReader = XmlReader.Create(stream, new XmlReaderSettings() { XmlResolver = null }))
#endif
            {
                return ReadXml<T>(fileName, xmlReader);
            }
        }

        public static string ReadStringFromFile(string fileName)
        {
            // Will automatically detect encoding using BOM
            using (var stream = FabricFile.Open(fileName, FileMode.Open, FileAccess.Read, FileShare.Read))
            using (var reader = new StreamReader(stream))
            {
                return reader.ReadLine();
            }
        }

        public static void WriteXml<T>(Stream outputStream, T value)
        {
            using (var writer = XmlWriter.Create(outputStream))
            {
                var serializer = new XmlSerializer(typeof(T));
                serializer.Serialize(writer, value);
            }
        }

        public static void WriteXml<T>(string fileName, T value)
        {
            var directoryName = FabricPath.GetDirectoryName(fileName);
            if (!FabricDirectory.Exists(directoryName))
            {
                FabricDirectory.CreateDirectory(directoryName);
            }

            using (var stream = FabricFile.Open(fileName, FileMode.Create, FileAccess.Write, FileShare.Read))
            using (var writer = XmlWriter.Create(stream))
            {
                var serializer = new XmlSerializer(typeof(T));
                serializer.Serialize(writer, value);
            }

#if DotNetCoreClrLinux
            Helpers.UpdateFilePermission(fileName);
#endif
        }

        public static void WriteStringToFile(string fileName, string value, bool writeLine = true, Encoding encoding = null)
        {
            ImageStoreUtility.WriteStringToFile(fileName, value, writeLine, encoding);
        }

        public static bool IsEqual<T>(T param1, T param2)
            where T : class
        {
            if (param1 == null && param2 == null)
            {
                return true;
            }

            if (param1 == null || param2 == null)
            {
                return false;
            }

            var serializer = new XmlSerializer(typeof(T));
            using (MemoryStream stream1 = new MemoryStream(), stream2 = new MemoryStream())
            {
                serializer.Serialize(stream1, param1);
                serializer.Serialize(stream2, param2);

                stream1.Seek(0, SeekOrigin.Begin);
                stream2.Seek(0, SeekOrigin.Begin);

                return ImageBuilderUtility.Equals(new StreamReader(stream1).ReadToEnd(), new StreamReader(stream2).ReadToEnd());
            }
        }

        public static bool IsNotEqual<T>(T param1, T param2)
            where T : class
        {
            return !ImageBuilderUtility.IsEqual<T>(param1, param2);
        }

        public static bool IsEqual<T>(T[] param1, T[] param2)
            where T : class
        {
            bool isEqual = ImageBuilderUtility.IsArrayItemCountEqual<T>(param1, param2);

            for (int i = 0; isEqual && (param2 != null) && i < param2.Length; i++)
            {
                isEqual = ImageBuilderUtility.IsEqual<T>(param1[i], param2[i]);
            }

            return isEqual;
        }

        public static bool IsNotEqual<T>(T[] param1, T[] param2)
            where T : class
        {
            return !ImageBuilderUtility.IsEqual<T>(param1, param2);
        }

        public static bool IsArrayItemCountEqual<T>(T[] param1, T[] param2)
           where T : class
        {
            int param1Count = (param1 == null) ? 0 : param1.Length;
            int param2Count = (param2 == null) ? 0 : param2.Length;
            return param1Count == param2Count;
        }

        public static bool IsArrayItemCountNotEqual<T>(T[] param1, T[] param2)
            where T : class
        {
            return !ImageBuilderUtility.IsArrayItemCountEqual<T>(param1, param2);
        }

        public static bool Equals(string string1, string string2)
        {
            return string.Equals(string1, string2, System.StringComparison.OrdinalIgnoreCase);
        }

        public static bool NotEquals(string string1, string string2)
        {
            return !ImageBuilderUtility.Equals(string1, string2);
        }

        public static bool IsValidFileName(string value)
        {
            foreach (char c in value)
            {
                if (InvalidFileNameChars.Contains(c))
                {
                    return false;
                }
            }

            return true;
        }

        public static void TraceAndThrowReservedDirectoryError(string type, string format, params object[] args)
        {
            ImageBuilder.TraceSource.WriteError(
                type,
                format,
                args);

            throw new FabricImageBuilderReservedDirectoryException(
                string.Format(CultureInfo.InvariantCulture, format, args),
                FabricErrorCode.ImageBuilderReservedDirectoryError);
        }

        public static void TraceAndThrowValidationError(string type, string format, params object[] args)
        {
            TraceAndThrowValidationError(
                FabricErrorCode.ImageBuilderValidationError,
                type,
                format,
                args);
        }

        public static void TraceAndThrowValidationError(FabricErrorCode fabricErrorCode, string type, string format, params object[] args)
        {
            ImageBuilder.TraceSource.WriteError(
                type,
                format,
                args);

            throw new FabricImageBuilderValidationException(
                string.Format(CultureInfo.InvariantCulture, format, args),
                fabricErrorCode);
        }

        public static void TraceAndThrowValidationErrorWithFileName(string type, string fileName, string format, params object[] args)
        {
            TraceAndThrowValidationErrorWithFileName(
                FabricErrorCode.ImageBuilderValidationError,
                type,
                fileName,
                format,
                args);
        }

        public static void TraceAndThrowValidationErrorWithFileName(FabricErrorCode fabricErrorCode, string type, string fileName, string format, params object[] args)
        {
            string traceErrorMessage = string.Format(CultureInfo.InvariantCulture, format, args);
            if (!string.IsNullOrEmpty(fileName))
            {
                string fileNameMessage = string.Format(CultureInfo.InvariantCulture, StringResources.ImageBuilderError_FileName, fileName);
                traceErrorMessage = string.Format(CultureInfo.InvariantCulture, "{0}\n{1}", traceErrorMessage, fileNameMessage);
            }

            ImageBuilder.TraceSource.WriteError(
                type,
                traceErrorMessage);

            throw new FabricImageBuilderValidationException(
                string.Format(CultureInfo.InvariantCulture, format, args),
                fileName,
                fabricErrorCode);
        }

        public static void TraceAndThrowValidationErrorWithFileName(Exception innerException, string type, string fileName, string format, params object[] args)
        {
            string traceErrorMessage = string.Format(CultureInfo.InvariantCulture, format, args);
            if (!string.IsNullOrEmpty(fileName))
            {
                string fileNameMessage = string.Format(CultureInfo.InvariantCulture, StringResources.ImageBuilderError_FileName, fileName);
                traceErrorMessage = string.Format(CultureInfo.InvariantCulture, "{0}\n{1}", traceErrorMessage, fileNameMessage);
            }

            ImageBuilder.TraceSource.WriteError(
                type,
                traceErrorMessage);

            throw new FabricImageBuilderValidationException(
                string.Format(CultureInfo.InvariantCulture, format, args),
                fileName,
                innerException,
                FabricErrorCode.ImageBuilderValidationError);
        }

        public static void CreateFolderIfNotExists(string directoryPath)
        {
            if (!FabricDirectory.Exists(directoryPath))
            {
                FabricDirectory.CreateDirectory(directoryPath);
            }
        }

        public static string GetTempFileName(string localRoot)
        {
            return Path.Combine(localRoot, Path.GetRandomFileName());
        }

        public static bool IsParameter(string value)
        {
            if (string.IsNullOrEmpty(value))
            {
                return false;
            }

            return value[0] == '[' && value[value.Length - 1] == ']';
        }

        public static bool IsValidAccountName(string value)
        {
            string[] names = null;

            if (value.Contains("@"))
            {
                names = value.Split(new char[] { '@' });
            }
            else if (value.Contains("\\"))
            {
                names = value.Split(new char[] { '\\' });
            }

            if ((names == null) ||
                (names.Length != 2))
            {
                return false;
            }

            return true;
        }

        public static void DeleteTempLocationWithRetry(string location)
        {
            int maxRetryCount = 5;
            int retryCount = 0;
            while (retryCount < maxRetryCount)
            {
                try
                {
                    DeleteTempLocation(location);
                    return;
                }
                catch (FileLoadException ex)
                {
                    ++retryCount;
                    if (retryCount == maxRetryCount)
                    {
                        ImageBuilder.TraceSource.WriteWarning(
                               TraceType,
                               "DeleteTempLocation {0} failed with {1} and max retry count {2} was reached.",
                               location,
                               ex,
                               retryCount);
                        throw;
                    }
                    else
                    {
                        ImageBuilder.TraceSource.WriteWarning(
                               TraceType,
                               "DeleteTempLocation {0} failed with {1}. Retry count: {2}, retrying.",
                               location,
                               ex,
                               retryCount);
                    }
                }
            }
        }

        public static void DeleteTempLocation(string location)
        {
            if (!string.IsNullOrEmpty(location) && FabricDirectory.Exists(location))
            {
                FabricDirectory.Delete(location, recursive: true, deleteReadOnlyFiles: true);
            }
            else if (!string.IsNullOrEmpty(location) && FabricFile.Exists(location))
            {
                FabricFile.Delete(location, deleteReadonly: true);
            }
        }

        public static void RemoveReadOnlyFlag(string path)
        {
            if (FabricDirectory.Exists(path))
            {
                var fileFullPaths = FabricDirectory.GetFiles(path, "*", true, SearchOption.AllDirectories);

                foreach (var file in fileFullPaths)
                {
                    FabricFile.RemoveReadOnlyAttribute(file);
                }
            }
            else if (FabricFile.Exists(path))
            {
                FabricFile.RemoveReadOnlyAttribute(path);
            }
            else
            {
                return;
            }
        }

        public static bool IsInstaller(string codePackagePath)
        {
#if DotNetCoreClrLinux
            var packageManagerType = FabricEnvironment.GetLinuxPackageManagerType();
            switch (packageManagerType)
            {
                case LinuxPackageManagerType.Deb:
                    return Path.GetExtension(codePackagePath) == ".deb";
                case LinuxPackageManagerType.Rpm:
                    return Path.GetExtension(codePackagePath) == ".rpm";
                default:
                    ImageBuilderUtility.TraceAndThrowValidationError(
                        TraceType,
                        StringResources.ImageBuilderError_LinuxPackageManagerUnknown,
                        (int)packageManagerType);
                    return false; // To please compiler
            }
#else
            return Path.GetExtension(codePackagePath) == ".msi";
#endif
        }

        public static bool IsMSI(string codePackagePath)
        {
            return Path.GetExtension(codePackagePath) == ".msi";
        }

        public static bool IsCabFile(string codePackagePath)
        {
            return Path.GetExtension(codePackagePath) == ".cab";
        }

        public static bool IsInArchive(string archiveFilePath, string filePath)
        {
            return IsAnyInArchive(archiveFilePath, new string[] { filePath }, true);
        }

        public static bool IsAnyInArchive(string archiveFilePath, string[] fileNames)
        {
            return IsAnyInArchive(archiveFilePath, fileNames, false);
        }

        // .zip standard requires forward slash as path separator, but we support back slash for backward compatibility and user friendly.
        private static bool IsMatchZipEntry(string entryInZip, string filename)
        {
#if DotNetCoreClrLinux
            return string.Equals(entryInZip, filename, StringComparison.OrdinalIgnoreCase);
#else
            return string.Equals(entryInZip.Replace('\\', '/'), filename.Replace('\\', '/'), StringComparison.OrdinalIgnoreCase);
#endif
        }

        private static bool IsAnyInArchive(string archiveFilePath, string[] files, bool matchFullName)
        {
            using (var archive = ZipFile.OpenRead(archiveFilePath))
            {
                foreach (var entry in archive.Entries)
                {
                    foreach (var file in files)
                    {
                        bool matched = matchFullName
                            ? IsMatchZipEntry(entry.FullName, file)
                            : IsMatchZipEntry(entry.Name, file);

                        if (matched)
                        {
                            return true;
                        }
                    }
                }

                return false;
            }
        }

        public static void ExtractArchive(string archiveFilePath, string destFolderPath)
        {
            using (var archive = ZipFile.OpenRead(archiveFilePath))
            {
                archive.ExtractToDirectory(destFolderPath);
            }
        }

        public static void ExtractFromArchive(string archiveFilePath, string srcFilePath, string destFilePath)
        {
            using (var archive = ZipFile.OpenRead(archiveFilePath))
            {
                var result = archive.Entries.FirstOrDefault<ZipArchiveEntry>((entry) =>
                {
                    return IsMatchZipEntry(entry.FullName, srcFilePath);
                });

                if (result != null)
                {
                    result.ExtractToFile(destFilePath);
                }
            }
        }

        public static void AddToArchive(string archiveFilePath, string srcFilePath, string destFilePath)
        {
            using (var archive = ZipFile.Open(archiveFilePath, ZipArchiveMode.Update))
            {
                archive.CreateEntryFromFile(srcFilePath, destFilePath);
            }
        }

        public static string EnsureHashFile(string resourcePath, string hashFilePath, bool imageStoreServiceEnabled)
        {
            if (!FabricFile.Exists(hashFilePath))
            {
                string checksum = ChecksumUtility.ComputeHash(resourcePath, imageStoreServiceEnabled);
                ImageBuilderUtility.WriteStringToFile(hashFilePath, checksum);
                return checksum;
            }
            else
            {
                return ImageBuilderUtility.ReadStringFromFile(hashFilePath);
            }
        }

        internal static string ValidateSfpkgDownloadPathAndGetFileName(string sfpkgPath)
        {
            Uri uri;
            if (!Uri.TryCreate(sfpkgPath, UriKind.Absolute, out uri))
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.Error_InvalidApplicationPackageDownloadUri,
                    sfpkgPath);
            }

            if (string.IsNullOrEmpty(uri.Scheme))
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.Error_InvalidApplicationPackageDownloadUri,
                    sfpkgPath);
            }

            string protocol = uri.Scheme.ToLowerInvariant();
            if (protocol != "http" && protocol != "https")
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.Error_InvalidApplicationPackageDownloadUri_Protocol,
                    sfpkgPath,
                    protocol);
            }


            var segments = uri.Segments;
            if (segments.Length == 0)
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.Error_InvalidApplicationPackageDownloadUri,
                    sfpkgPath);
            }

            var fileName = segments[segments.Length - 1];
            if (!fileName.EndsWith(StringConstants.SfpkgExtension))
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.Error_InvalidApplicationPackageDownloadUri_FileName,
                    sfpkgPath,
                    fileName);
            }

            if (!ImageBuilderUtility.IsValidFileName(fileName))
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.Error_InvalidApplicationPackageDownloadUri_FileName,
                    sfpkgPath);
            }

            return fileName;
        }

        internal static bool IsSfpkgFile(string path)
        {
            return Path.GetExtension(path).Equals(StringConstants.SfpkgExtension, StringComparison.InvariantCultureIgnoreCase);
        }

        internal static void ValidateRelativePath(string path, string[] reservedDirectoryNames = null)
        {
            if (Path.IsPathRooted(path.TrimStart('\\')))
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.ImageBuilderError_AbsolutePathNotAllowed,
                    path);
            }

            string remainingPath = path;
            while (!string.IsNullOrEmpty(remainingPath))
            {
                string pathPart = Path.GetFileName(remainingPath);
                if (pathPart.Equals(StringConstants.DoubleDot, StringComparison.Ordinal))
                {
                    ImageBuilderUtility.TraceAndThrowValidationError(
                        TraceType,
                        StringResources.ImageBuilderError_InvalidRelativePath,
                        path);
                }

                remainingPath = FabricPath.GetDirectoryName(remainingPath);

                if (string.IsNullOrEmpty(remainingPath)
                    && reservedDirectoryNames != null
                    && reservedDirectoryNames.Count() > 0)
                {
                    foreach (string directoryName in reservedDirectoryNames)
                    {
                        if (string.Compare(directoryName, pathPart, true) == 0)
                        {
                            ImageBuilderUtility.TraceAndThrowReservedDirectoryError(
                                TraceType,
                                StringResources.ImageBuilderError_ReservedDirectoryName,
                                path,
                                pathPart);
                        }
                    }
                }
            }
        }

        internal static bool IsOpenNetworkReference(string networkRef)
        {
            return Equals(networkRef, StringConstants.OpenNetworkName);
        }

        internal static bool IsNatNetworkReference(string networkRef)
        {
            return Equals(networkRef, StringConstants.NatNetworkName);
        }

        private static T ReadXml<T>(string fileName, XmlReader reader)
        {
            try
            {
                var serializer = new XmlSerializer(typeof(T));
                T obj = (T)serializer.Deserialize(reader);
                return obj;
            }
            catch (InvalidOperationException xmlParseException)
            {
                if (FabricFile.Exists(fileName))
                {
                    ImageBuilder.TraceSource.WriteError(
                        TraceType,
                        "Invalid XML Content. File Size: {0}",
                        FabricFile.GetSize(fileName));
                }
                else
                {
                    ImageBuilder.TraceSource.WriteError(
                        TraceType,
                        "File '{0}' does not exist.",
                        fileName);
                }

                TraceAndThrowValidationErrorWithFileName(
                    xmlParseException,
                    TraceType,
                    fileName,
                    StringResources.ImageBuilderError_ErrorParsingXmlFile,
                    fileName);
            }

            return default(T);
        }

        public static void CopyFile(string source, string destination)
        {
            FabricFile.Copy(source, destination, true);
        }

        public static void CompareAndFixTargetManifestVersionsForUpgrade(
            System.Fabric.Common.ImageModel.StoreLayoutSpecification storeLayoutSpecification,
            ImageStoreWrapper destinationImageStoreWrapper,
            TimeoutHelper timeoutHelper,
            Dictionary<string, Dictionary<string, SettingsType>> settingsType,
            string typeName,
            ApplicationManifestType currentApplicationManifest,
            ApplicationManifestType targetApplicationManifest,
            IList<ServiceManifestType> currentServiceManifests,
            ref IList<ServiceManifestType> targetServiceManifests)
        {
            // Service manifest imports that exist in both current and target.
            var targetServiceManifestImports = targetApplicationManifest.ServiceManifestImport;
            var commonServiceManifestImports = currentApplicationManifest.ServiceManifestImport.Where(
                ci => targetServiceManifestImports.Any(ti => ti.ServiceManifestRef.ServiceManifestName == ci.ServiceManifestRef.ServiceManifestName));

            List<ServiceManifestType> unchangedServiceManifests = new List<ServiceManifestType>();
            foreach (ApplicationManifestTypeServiceManifestImport currentManifestImport in commonServiceManifestImports)
            {
                var currentManifest = currentServiceManifests.FirstOrDefault(item => item.Name == currentManifestImport.ServiceManifestRef.ServiceManifestName);
                var targetManifest = targetServiceManifests.FirstOrDefault(item => item.Name == currentManifestImport.ServiceManifestRef.ServiceManifestName);
                // TODO: would codepackage versions be different than manifest version?
                var currentManifestVersion = currentManifest.Version;
                var targetManifestVersion = targetManifest.Version;

                bool equal = ImageBuilderUtility.IsEqual<ServiceManifestType>(
                    ReplaceVersion(currentManifest, "0"), ReplaceVersion(targetManifest, "0"));

                ReplaceVersion(currentManifest, currentManifestVersion);
                ReplaceVersion(targetManifest, targetManifestVersion);

                #region Determine if settings have been changed since last version
                if (equal && settingsType != null)
                {
                    string serviceManifestName = currentManifest.Name;

                    // 1. Check if setting count remains the same
                    int newSettingsTypeCount = 0;
                    if (settingsType.ContainsKey(serviceManifestName))
                    {
                        newSettingsTypeCount = settingsType[serviceManifestName].Count(cp => cp.Key != null);
                    }
                    int currentSettingsTypeCount = 0;
                    if (currentManifest.ConfigPackage != null)
                    {
                        currentSettingsTypeCount = currentManifest.ConfigPackage.Count();
                    }

                    if (newSettingsTypeCount != currentSettingsTypeCount)
                    {
                        equal = false;
                    }

                    // 2. Read settings from each configPackage and compare the value
                    if (equal && currentManifest.ConfigPackage != null)
                    {
                        foreach (var configPackage in currentManifest.ConfigPackage)
                        {
                            SettingsType newSettingsType = null;
                            if (settingsType.ContainsKey(serviceManifestName) && settingsType[serviceManifestName].ContainsKey(configPackage.Name))
                            {
                                newSettingsType = settingsType[serviceManifestName][configPackage.Name];
                            }

                            string configPackageDirectory = storeLayoutSpecification.GetConfigPackageFolder(typeName, serviceManifestName, configPackage.Name, configPackage.Version);
                            string settingsFile = storeLayoutSpecification.GetSettingsFile(configPackageDirectory);
                            SettingsType currentSettingsType = null;
                            if (destinationImageStoreWrapper.DoesContentExists(settingsFile, timeoutHelper.GetRemainingTime()))
                            {
                                currentSettingsType = destinationImageStoreWrapper.GetFromStore<SettingsType>(settingsFile, timeoutHelper.GetRemainingTime());
                            }

                            equal &= CompareSettingsType(currentSettingsType, newSettingsType);
                            if (!equal) break;
                        }
                    }

                    // Code pacakge versions not change for settings upgrade
                    if (!equal)
                    {
                        if (targetManifest.CodePackage != null)
                        {
                            for (int i = 0; i < targetManifest.CodePackage.Count(); ++i)
                            {
                                ReplaceVersion(ref targetManifest.CodePackage[i], currentManifestVersion);
                            }
                        }
                    }
                }
                #endregion

                if (equal)
                {
                    unchangedServiceManifests.Add(currentManifest);
                }
            }

            // Use the unchanged manifests.
            if (unchangedServiceManifests.Count() != 0)
            {
                List<ServiceManifestType> updatedTargetServiceManifests = new List<ServiceManifestType>();
                foreach (var manifest in targetServiceManifests)
                {
                    var unchangedManifest = unchangedServiceManifests.FirstOrDefault(item => item.Name == manifest.Name);
                    if (unchangedManifest != default(ServiceManifestType))
                    {
                        updatedTargetServiceManifests.Add(unchangedManifest);
                        for (int i = 0; i < targetApplicationManifest.ServiceManifestImport.Count(); ++i)
                        {
                            if (targetApplicationManifest.ServiceManifestImport[i].ServiceManifestRef.ServiceManifestName == manifest.Name)
                            {
                                targetApplicationManifest.ServiceManifestImport[i].ServiceManifestRef.ServiceManifestVersion = unchangedManifest.Version;
                                break;
                            }
                        }

                    }
                    else
                    {
                        updatedTargetServiceManifests.Add(manifest);
                    }
                }

                targetServiceManifests = updatedTargetServiceManifests;
            }
        }

        private static bool CompareSettingsType(SettingsType currentSettingsType, SettingsType newSettingsType)
        {
            if (newSettingsType == null && currentSettingsType == null)
            {
                return true;
            }
            else if (newSettingsType == null || currentSettingsType == null)
            {
                return false;
            }
            else
            {
                foreach (SettingsTypeSection section in currentSettingsType.Section)
                {
                    SettingsTypeSection newSection = newSettingsType.Section.FirstOrDefault(sts => sts.Name.Equals(section.Name));
                    if (newSection == null || newSection.Parameter.Count() != section.Parameter.Count())
                    {
                        return false;
                    }
                    var newParameters = newSection.Parameter.OrderBy(p => p.Name);
                    var currentParameters = section.Parameter.OrderBy(p => p.Name);
                    for (int i = 0; i < newSection.Parameter.Count(); i++)
                    {
                        SettingsTypeSectionParameter curPara = section.Parameter[i];
                        SettingsTypeSectionParameter newPara = newSection.Parameter[i];
                        if (curPara.Name != newPara.Name
                            || curPara.Value != newPara.Value
                            || curPara.IsEncrypted != curPara.IsEncrypted)
                        {
                            return false;
                        }
                    }
                }
                return true;
            }
        }

        private static ServiceManifestType ReplaceVersion(ServiceManifestType manifest, string version)
        {
            var replacedManifest = manifest;
            manifest.Version = version;
            if (manifest.CodePackage != null)
            {
                for (int i = 0; i < manifest.CodePackage.Count(); ++i)
                {
                    ReplaceVersion(ref manifest.CodePackage[i], version);
                }
            }

            if (manifest.ConfigPackage != null)
            {
                for (int i = 0; i < manifest.ConfigPackage.Count(); ++i)
                {
                    ReplaceVersion(ref manifest.ConfigPackage[i], version);
                }
            }

            if (manifest.DataPackage != null)
            {
                for (int i = 0; i < manifest.DataPackage.Count(); ++i)
                {
                    ReplaceVersion(ref manifest.DataPackage[i], version);
                }
            }

            return manifest;
        }

        private static void ReplaceVersion(ref CodePackageType codePackage, string version)
        {
            codePackage.Version = version;
        }

        private static void ReplaceVersion(ref ConfigPackageType configPackage, string version)
        {
            configPackage.Version = version;
        }

        private static void ReplaceVersion(ref DataPackageType dataPackage, string version)
        {
            dataPackage.Version = version;
        }
    }
}