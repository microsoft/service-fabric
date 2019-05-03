// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.IO;
    using System.Threading;
    
    // Class that implements the processing of binary files containing performance 
    // counters specified in the cluster manifest
    internal class PerfCounterFolderProcessor : IDisposable
    {
        // Constants
        private const string IsEnabledParamName = "IsEnabled";
        private const string TestOnlyCounterFilePathParamName = "TestOnlyCounterFilePath";
        private const string PerfCounterFolderProcessingTimerIdSuffix = "_PerfCounterFolderProcessingTimer";
        private const string PerfCounterBinaryFilePattern = "*.blg";
        private static readonly TimeSpan DefaultPerfCounterFolderProcessingInterval = TimeSpan.FromMinutes(5);

        // PerfCounterFolderProcessor is a singleton. This is a reference to the only active
        // instance of this class.
        private static PerfCounterFolderProcessor folderProcessorSingleton;

        // Tag used to represent the source of the log message
        private string logSourceId;

        // Object used for tracing
        private FabricEvents.ExtensionsEvents traceSource;

        // Configuration reader
        private ConfigReader configReader;

        // Path to the log folder
        private string logDirectory;

        // Whether or not performance counter collection is enabled
        private bool perfCounterCollectionEnabled;

        // Timer to process performance counter binary files
        private DcaTimer perfCounterFolderProcessingTimer;

        // Path to the folder containing performance counter binary files
        private string perfCounterBinaryFolder;

        // Path to the folder containing archived performance counter binary files
        private string perfCounterBinaryArchiveFolder;

        // Whether or not the archive folder is a sub-folder of the folder containing
        // the performance counter binary files
        private bool archiveFolderIsUnderBinaryFolder;

        // Path to the folder where we need to place the performance counter 
        // files that are ready for the consumers
        private string outputFolderPath;

        // Whether or not the we are in the process of stopping
        private bool stopping;

        // Whether or not the object has been disposed
        private bool disposed;

        public static PerfCounterFolderProcessor Create(
                          FabricEvents.ExtensionsEvents traceSource,
                          string logSourceId,
                          ConfigReader configReader,
                          string logDirectory,
                          string outputFolderPath,
                          out bool isEnabled,
                          out List<string> additionalFoldersToTrim)
        {
            additionalFoldersToTrim = null;
            isEnabled = false;

            // Create a new instance of the folder processor 
            PerfCounterFolderProcessor folderProcessor = new PerfCounterFolderProcessor();

            // PerfCounterFolderProcessor is a singleton, so make sure there are no other instances.
            object original = Interlocked.CompareExchange(ref folderProcessorSingleton, folderProcessor, null);
            if (null != original)
            {
                traceSource.WriteError(
                    logSourceId,
                    "Cannot have more than one producer of type {0}, whose {1} value is {2}.",
                    StandardPluginTypes.FolderProducer,
                    FolderProducerValidator.FolderTypeParamName,
                    FolderProducerValidator.ServiceFabricPerformanceCounters);
                return null;
            }

            // Initialize the folder processor
            if (false == folderProcessor.Initialize(traceSource, logSourceId, configReader, logDirectory, outputFolderPath))
            {
                return null;
            }

            isEnabled = folderProcessor.perfCounterCollectionEnabled;
            if (isEnabled)
            {
                // In addition to the output folder, which is already trimmed by default, we also need
                // the perf counter binary folder and the perf counter binary archive folder to be trimmed.
                additionalFoldersToTrim = new List<string> { folderProcessor.perfCounterBinaryFolder };
                if (false == folderProcessor.archiveFolderIsUnderBinaryFolder)
                {
                    additionalFoldersToTrim.Add(folderProcessor.perfCounterBinaryArchiveFolder);
                }
            }

            return folderProcessor;
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            // Keep track of the fact that the producer is stopping
            this.stopping = true;

            if (null != this.perfCounterFolderProcessingTimer)
            {
                // Stop and dispose timer
                this.perfCounterFolderProcessingTimer.StopAndDispose();

                // Wait for timer callback to finish executing
                this.perfCounterFolderProcessingTimer.DisposedEvent.WaitOne();
            }

            // Since the current instance is being disposed, it is okay to create other
            // instances after this.
            object original = Interlocked.Exchange(ref folderProcessorSingleton, null);
            Debug.Assert(original == this, "Object being disposed should be set as current folder processor.");

            GC.SuppressFinalize(this);
        }

        private static int CompareFileCreationTimes(FileInfo file1, FileInfo file2)
        {
            // We want the file with the more recent creation time to come first
            return DateTime.Compare(file2.CreationTime, file1.CreationTime);
        }

        private bool Initialize(
            FabricEvents.ExtensionsEvents traceSource, 
            string logSourceId, 
            ConfigReader configReader, 
            string logDirectory, 
            string outputFolderPath)
        {
            // Initialization
            this.stopping = false;
            this.logSourceId = logSourceId;
            this.traceSource = traceSource;
            this.configReader = configReader;
            this.logDirectory = logDirectory;
            this.outputFolderPath = outputFolderPath;
            this.archiveFolderIsUnderBinaryFolder = false;

            // Read performance counter collection settings from settings.xml
            this.GetPerformanceCounterConfiguration();
            if (false == this.perfCounterCollectionEnabled)
            {
                // Performance counter collection is not enabled, so return immediately
                return true;
            }

            // Create a timer to process performance counter binary files
            string timerId = string.Concat(
                                 this.logSourceId,
                                 PerfCounterFolderProcessingTimerIdSuffix);
            this.perfCounterFolderProcessingTimer = new DcaTimer(
                                                           timerId,
                                                           this.ProcessPerfCounterFolder,
                                                           DefaultPerfCounterFolderProcessingInterval);
            this.perfCounterFolderProcessingTimer.Start();
            return true;
        }

        private void GetPerformanceCounterConfiguration()
        {
            // Figure out if performance counter collection is enabled. By default
            // it is enabled and the 'IsEnabled' setting must be explicitly 
            // specified to disable it.
            this.perfCounterCollectionEnabled = this.configReader.GetUnencryptedConfigValue(
                                                    PerformanceCounterCommon.PerformanceCounterSectionName,
                                                    IsEnabledParamName,
                                                    true);
            if (this.perfCounterCollectionEnabled)
            {
                // Get the path to the performance counter binary files
                string binaryFilePath = this.configReader.GetUnencryptedConfigValue(
                                            PerformanceCounterCommon.PerformanceCounterSectionName,
                                            TestOnlyCounterFilePathParamName,
                                            string.Empty);
                if (false == string.IsNullOrEmpty(binaryFilePath))
                {
                    // Non-default path was specified
                    this.perfCounterBinaryFolder = binaryFilePath;
                    this.perfCounterBinaryArchiveFolder = Path.Combine(
                                                              binaryFilePath,
                                                              FolderProducerConstants.PerformanceCountersBinaryArchiveDirectoryName);
                    this.archiveFolderIsUnderBinaryFolder = true;
                }
                else
                {
                    // Use default path
                    this.perfCounterBinaryFolder = Path.Combine(
                                                       this.logDirectory,
                                                       FolderProducerConstants.PerformanceCountersBinaryDirectoryName);
                    this.perfCounterBinaryArchiveFolder = Path.Combine(
                                                              this.logDirectory,
                                                              FolderProducerConstants.PerformanceCountersBinaryArchiveDirectoryName);
                }

                // Create the directory where the performance counter binary files will be archived.
                FabricDirectory.CreateDirectory(this.perfCounterBinaryArchiveFolder);

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Performance counter data collection is enabled. Performance counter binary folder: {0}, performance counter binary archive folder: {1}, performance counter output folder: {2}.",
                    this.perfCounterBinaryFolder,
                    this.perfCounterBinaryArchiveFolder,
                    this.outputFolderPath);
            }
            else
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Performance counter data collection has been disabled via the {0} parameter in the {1} section of the cluster manifest.",
                    IsEnabledParamName,
                    PerformanceCounterCommon.PerformanceCounterSectionName);
            }
        }

        private void ProcessPerfCounterFolder(object state)
        {
            // Get the files in the performance counter binary files directory
            this.traceSource.WriteInfo(
                this.logSourceId,
                "Searching for performance counter binary files in directory {0}.",
                this.perfCounterBinaryFolder);

            DirectoryInfo dirInfo = new DirectoryInfo(this.perfCounterBinaryFolder);
            FileInfo[] files = dirInfo.GetFiles(PerfCounterBinaryFilePattern);

            // Sort the files by creation time, such that the most recent 
            // file is first.
            Array.Sort(files, CompareFileCreationTimes);
            if (files.Length > 1)
            {
                // Exclude the file with the most recent creation time, because 
                // that is the file to which the OS is currently writing performance
                // counter data. Therefore it is not ready for use by the consumer.
                FileInfo[] outputFiles = new FileInfo[files.Length - 1];
                Array.Copy(files, 1, outputFiles, 0, files.Length - 1);

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Found {0} performance counter binary files in directory {1} that are ready to be processed by consumers.",
                    outputFiles.Length,
                    this.perfCounterBinaryFolder);

                // Process the remaining files
                foreach (FileInfo outputFile in outputFiles)
                {
                    if (this.stopping)
                    {
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "The producer is being stopped. Therefore no more performance counter binary files from directory {0} will be made available to consumers.",
                            this.perfCounterBinaryFolder);
                        break;
                    }

                    this.ProcessPerfCounterFile(outputFile);
                }
            }

            // Schedule the next pass
            this.perfCounterFolderProcessingTimer.Start();
        }

        private void ProcessPerfCounterFile(FileInfo perfCounterFile)
        {
            // Copy the file to the output folder.
            // NOTE: In the future we may (optionally?) convert this to a human-
            // readable CSV file before copying to the output folder.
            string destinationFilePath = Path.Combine(
                                             this.outputFolderPath,
                                             perfCounterFile.Name);
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    {
                        FabricFile.Copy(perfCounterFile.FullName, destinationFilePath, true);
                    });
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Failed to copy performance counter binary file. Source: {0}, Destination: {1}.",
                    perfCounterFile.FullName,
                    destinationFilePath);
                return;
            }

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Performance counter binary copied successfully. Source: {0}, Destination: {1}.",
                perfCounterFile.FullName,
                destinationFilePath);

            // Move the file to the archives folder
            destinationFilePath = Path.Combine(
                                      this.perfCounterBinaryArchiveFolder,
                                      perfCounterFile.Name);
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    {
                        FabricFile.Move(perfCounterFile.FullName, destinationFilePath);
                    });
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Failed to move performance counter binary file to the archives folder. Source: {0}, Destination: {1}.",
                    perfCounterFile.FullName,
                    destinationFilePath);
                return;
            }

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Performance counter binary successfully moved to the archives folder. Source: {0}, Destination: {1}.",
                perfCounterFile.FullName,
                destinationFilePath);
        }
    }
}