// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca 
{
    using System.IO;
    using System.Reflection;

    // This class implements file compression
    internal static class FileCompressor
    {
#if DotNetCoreClrLinux
        private const string SystemIoCompressionAssemblyName = "System.IO.Compression";
        private const string SystemIoCompressionFileSystemAssemblyName = "System.IO.Compression.ZipFile";
#else
        private const string SystemAssemblyName = "System, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";
        private const string SystemIoCompressionAssemblyName = "System.IO.Compression, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";
        private const string SystemIoCompressionFileSystemAssemblyName = "System.IO.Compression.FileSystem, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089";
#endif

        private static readonly ConstructorInfo ZipArchiveConstructor;
        private static readonly MethodInfo ZipArchiveDispose;
        private static readonly MethodInfo ZipFileExtCreateEntryFromFile;
        private static readonly object ZipArchiveModeCreate;
        private static readonly object CompressionLevelOptimal;

        static FileCompressor()
        {
            try
            {
                // Load the necessary assemblies
#if !DotNetCoreClrLinux
                Assembly system = Assembly.Load(new AssemblyName(SystemAssemblyName));
#endif
                Assembly systemIoCompression = Assembly.Load(new AssemblyName(SystemIoCompressionAssemblyName));
                Assembly systemIoCompressionFileSystem = Assembly.Load(new AssemblyName(SystemIoCompressionFileSystemAssemblyName));

                // Get an object representing ZipArchiveMode.Create
                Type zipArchiveMode = systemIoCompression.GetType("System.IO.Compression.ZipArchiveMode");
                ZipArchiveModeCreate = Enum.Parse(zipArchiveMode, "Create");

                // Get the constructor for the ZipArchive class
                Type[] constructorArgTypes = new Type[] { typeof(FileStream), zipArchiveMode };
                Type zipArchive = systemIoCompression.GetType("System.IO.Compression.ZipArchive");
                ZipArchiveConstructor = zipArchive.GetConstructor(constructorArgTypes);
                ZipArchiveDispose = zipArchive.GetMethod("Dispose", Type.EmptyTypes);

                // Get an object representing CompressionLevel.Optimal
#if DotNetCoreClrLinux
                Type compressionLevel = systemIoCompression.GetType("System.IO.Compression.CompressionLevel");
#else
                Type compressionLevel = system.GetType("System.IO.Compression.CompressionLevel");
#endif
                CompressionLevelOptimal = Enum.Parse(compressionLevel, "Optimal");

                // Get the ZipFileExtensions.CreateEntryFromFile method
                Type[] methodArgTypes = new Type[] { zipArchive, typeof(string), typeof(string), compressionLevel };
                Type zipFileExtensions = systemIoCompressionFileSystem.GetType("System.IO.Compression.ZipFileExtensions");
                ZipFileExtCreateEntryFromFile = zipFileExtensions.GetMethod("CreateEntryFromFile", methodArgTypes);
            }
            catch (Exception)
            {
            }

            CompressionEnabled = (ZipArchiveConstructor != null) &&
                                 (ZipArchiveDispose != null) &&
                                 (ZipFileExtCreateEntryFromFile != null) &&
                                 (ZipArchiveModeCreate != null) &&
                                 (CompressionLevelOptimal != null);
        }

        internal static bool CompressionEnabled { get; private set; }

        internal static void Compress(string source, string archiveEntryName, out string tempArchivePath)
        {
            string archiveFullPath = Utility.GetTempFileName();
            Utility.PerformIOWithRetries(
                () =>
                {
                    using (FileStream fs = new FileStream(archiveFullPath, FileMode.Create))
                    {
                        // ZipArchive archive;
                        object archive = null;
                        try
                        {
                            try
                            {
                                // archive = new ZipArchive(fs, ZipArchiveMode.Create);
                                archive = ZipArchiveConstructor.Invoke(new[] { fs, ZipArchiveModeCreate });

                                // archive.CreateEntryFromFile(source, archiveEntryName, CompressionLevel.Optimal);
                                ZipFileExtCreateEntryFromFile.Invoke(
                                    null,
                                    new[] { archive, source, archiveEntryName, CompressionLevelOptimal });
                            }
                            catch (TargetInvocationException e)
                            {
                                throw e.InnerException;
                            }
                        }
                        finally
                        {
                            if (null != archive)
                            {
                                // archive.Dispose();
                                ZipArchiveDispose.Invoke(archive, null);
                            }
                        }
                    }
                });

            tempArchivePath = archiveFullPath;
        }
    }
}