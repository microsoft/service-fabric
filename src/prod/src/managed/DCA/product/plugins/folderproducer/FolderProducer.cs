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
    using System.Linq;
    using Microsoft.Win32;

    // Class that implements a producer that writes data of interest to a specific folder
    internal class FolderProducer : IDcaProducer
    {
        // Constants
        private const string WERKeyName = "SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\LocalDumps";
        private const string DumpFolderValueName = "DumpFolder";

        private static readonly TimeSpan MinimumFileRetentionTime = TimeSpan.FromMinutes(30);
        private readonly DiskSpaceManager diskSpaceManager;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Configuration reader
        private readonly ConfigReader configReader;

#if !DotNetCoreClr
        // Performance counter folder processor
        private readonly PerfCounterFolderProcessor perfCounterFolderProcessor;
#endif

        private readonly List<string> additionalAppConfigSections;
        private readonly List<string> serviceConfigSections;

        // Producer initialization parameters
        private ProducerInitializationParameters initParam;

        // Settings
        private FolderProducerSettings folderProducerSettings;

        // Whether or not the object has been disposed
        private bool disposed;

        public FolderProducer(DiskSpaceManager diskSpaceManager, ProducerInitializationParameters initializationParameters)
        {
            this.diskSpaceManager = diskSpaceManager;

            // Initialization
            this.initParam = initializationParameters;
            this.logSourceId = string.Concat(this.initParam.ApplicationInstanceId, "_", this.initParam.SectionName);
            this.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            this.configReader = new ConfigReader(this.initParam.ApplicationInstanceId);
            this.additionalAppConfigSections = new List<string>();
            this.serviceConfigSections = new List<string>();

            // Read instance-specific settings from settings.xml
            this.GetSettings();
            if (false == this.folderProducerSettings.Enabled)
            {
                // Producer is not enabled, so return immediately
                return;
            }

            if (this.configReader.IsReadingFromApplicationManifest &&
                FolderProducerType.WindowsFabricCrashDumps == this.folderProducerSettings.Type)
            {
                this.serviceConfigSections.Add(ServiceConfig.ExeHostElement);
            }

            var additionalFoldersToTrim = new List<string>();
#if !DotNetCoreClr
            if (FolderProducerType.WindowsFabricPerformanceCounters == this.folderProducerSettings.Type)
            {
                // We will need information from the <PerformanceCounterLocalStore> section of the
                // service manifest.
                this.additionalAppConfigSections.Add(PerformanceCounterCommon.PerformanceCounterSectionName);

                // The performance counter binary files cannot be read while the OS is still
                // writing to them. Therefore, we make the files available to the consumer only
                // when the OS has finished writing to them. Hence we need a special processor
                // for these files.
                List<string> additionalPerfCounterFoldersToTrim;
                bool perfCounterCollectionEnabled;

                // There should be only one path in the path list for performance counters
                string perfCounterPath = this.folderProducerSettings.Paths[0];
                this.perfCounterFolderProcessor = PerfCounterFolderProcessor.Create(
                                                      this.traceSource,
                                                      this.logSourceId,
                                                      this.configReader,
                                                      this.initParam.LogDirectory,
                                                      perfCounterPath,
                                                      out perfCounterCollectionEnabled,
                                                      out additionalPerfCounterFoldersToTrim);
                if (null == this.perfCounterFolderProcessor)
                {
                    return;
                }

                if (false == perfCounterCollectionEnabled)
                {
                    return;
                }

                if (null != additionalPerfCounterFoldersToTrim)
                {
                    additionalFoldersToTrim.AddRange(additionalPerfCounterFoldersToTrim);
                }
            }
#endif

            if (null != this.initParam.ConsumerSinks)
            {
                foreach (object sinkAsObject in this.initParam.ConsumerSinks)
                {
                    IFolderSink folderSink = null;

                    try
                    {
                        folderSink = (IFolderSink)sinkAsObject;
                    }
                    catch (InvalidCastException e)
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Exception occured while casting a sink object of type {0} to interface IFolderSink. Exception information: {1}.",
                            sinkAsObject.GetType(),
                            e);
                    }

                    if (null == folderSink)
                    {
                        continue;
                    }

                    folderSink.RegisterFolders(this.folderProducerSettings.Paths);
                }
            }

#if DotNetCoreClrLinux
            if (FolderProducerType.WindowsFabricCrashDumps == this.folderProducerSettings.Type)
            {
                const int filePermissions =
                    Helpers.LINUX_USER_READ | Helpers.LINUX_USER_WRITE | Helpers.LINUX_USER_EXECUTE
                    | Helpers.LINUX_GROUP_READ | Helpers.LINUX_GROUP_WRITE | Helpers.LINUX_GROUP_EXECUTE
                    | Helpers.LINUX_OTHER_READ | Helpers.LINUX_OTHER_WRITE | Helpers.LINUX_OTHER_EXECUTE;

                Helpers.UpdateFilePermission(this.folderProducerSettings.Paths[0], filePermissions);

                using (FileStream fs = new FileStream("/proc/sys/kernel/core_pattern", FileMode.Open))
                {
                    using (StreamWriter sw = new StreamWriter(fs))
                    {
                        sw.Write(Path.Combine(this.folderProducerSettings.Paths[0], "%e.%p.dmp"));
                    }
                }
            }
#endif

            var foldersToTrim = this.folderProducerSettings.Paths.Concat(additionalFoldersToTrim).ToArray();

            if (this.IsDiskSpaceManagementEnabled())
            {
                foreach (string folderPath in foldersToTrim)
                {
                    // Figure out the timestamp before which all files will be deleted
                    this.diskSpaceManager.RegisterFolder(
                        this.logSourceId,
                        () => new DirectoryInfo(folderPath).EnumerateFiles("*", SearchOption.AllDirectories),
                        f => f.LastWriteTimeUtc < DateTime.UtcNow - MinimumFileRetentionTime, // isSafeToDelete
                        f => (!Utility.IgnoreUploadFileList.Exists(x => x.Equals(f)) &&
                            f.LastWriteTimeUtc >= DateTime.UtcNow - this.folderProducerSettings.DataDeletionAge)); // shouldBeRetained
                }
            }
        }

        // Additional sections in the cluster manifest or application manifest from which
        // we read configuration information.
        public IEnumerable<string> AdditionalAppConfigSections
        {
            get
            {
                return this.additionalAppConfigSections;
            }
        }

        // Sections in the service manifest from which we read configuration information.
        public IEnumerable<string> ServiceConfigSections
        {
            get
            {
                return this.serviceConfigSections;
            }
        }

        public void FlushData()
        {
            // Not implemented
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

#if !DotNetCoreClr
            if (null != this.perfCounterFolderProcessor)
            {
                this.perfCounterFolderProcessor.Dispose();
            }
#endif

            this.diskSpaceManager.UnregisterFolders(this.logSourceId);
            GC.SuppressFinalize(this);
        }

        private static bool GetCrashDumpFolder(string exeName, out string crashDumpFolder)
        {
#if DotNetCoreClrLinux
            crashDumpFolder = Utility.CrashDumpsDirectory;
            return true;
#else
            string exeNameWithoutPath = Path.GetFileName(exeName);
            string werKeyForExe = Path.Combine(WERKeyName, exeNameWithoutPath);
            crashDumpFolder = (string)Utility.GetRegistryValue(
                                          Registry.LocalMachine,
                                          werKeyForExe,
                                          DumpFolderValueName,
                                          false);
            return null != crashDumpFolder;
#endif
        }

        private bool IsDiskSpaceManagementEnabled()
        {
            return FolderProducerType.WindowsFabricCrashDumps == this.folderProducerSettings.Type ||
                FolderProducerType.WindowsFabricPerformanceCounters == this.folderProducerSettings.Type;
        }

        private void GetSettings()
        {
            this.folderProducerSettings.Paths = new List<string>();

            // Check for values in settings.xml
            this.folderProducerSettings.Enabled = this.configReader.GetUnencryptedConfigValue(
                                                      this.initParam.SectionName,
                                                      FolderProducerValidator.EnabledParamName,
                                                      FolderProducerConstants.FolderProducerEnabledByDefault);
            if (this.folderProducerSettings.Enabled)
            {
                if (false == this.GetFolderType())
                {
                    return;
                }

                if (FolderProducerType.WindowsFabricCrashDumps == this.folderProducerSettings.Type)
                {
                    this.GetCrashDumpFolders();
                }
                else if (FolderProducerType.WindowsFabricPerformanceCounters == this.folderProducerSettings.Type)
                {
                    string perfCounterDirectoryName = string.Concat(
                                                            FolderProducerConstants.PerformanceCountersDirectoryPrefix,
                                                            this.initParam.SectionName);
                    string perfCounterPath = Path.Combine(
                                                 this.initParam.LogDirectory,
                                                 perfCounterDirectoryName);
                    FabricDirectory.CreateDirectory(perfCounterPath);
                    this.folderProducerSettings.Paths.Add(perfCounterPath);
                }
                else
                {
                    if (false == this.GetFolderPath())
                    {
                        return;
                    }
                }

                // If the folder type is custom and deletion age is not specified, we
                // choose a value that is so large that deletion is effectively disabled.
                // We do this because we don't know what files the folder contains. So
                // we do not want to delete any files by default because we don't know
                // how long the files need to be kept around.
                var defaultDataDeletionAge =
                        ((FolderProducerType.WindowsFabricCrashDumps == this.folderProducerSettings.Type) ||
                         (FolderProducerType.WindowsFabricPerformanceCounters == this.folderProducerSettings.Type)) ?
                            FolderProducerConstants.DefaultDataDeletionAge :
                            FolderProducerConstants.MaxDataDeletionAge;

                var dataDeletionAge = TimeSpan.FromDays(this.configReader.GetUnencryptedConfigValue(
                                              this.initParam.SectionName,
                                              FolderProducerValidator.DataDeletionAgeParamName,
                                              (int)defaultDataDeletionAge.TotalDays));
                if (dataDeletionAge > FolderProducerConstants.MaxDataDeletionAge)
                {
                    this.traceSource.WriteWarning(
                        this.logSourceId,
                        "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                        dataDeletionAge,
                        this.initParam.SectionName,
                        FolderProducerValidator.DataDeletionAgeParamName,
                        FolderProducerConstants.MaxDataDeletionAge);
                    dataDeletionAge = FolderProducerConstants.MaxDataDeletionAge;
                }

                this.folderProducerSettings.DataDeletionAge = dataDeletionAge;

                // Check for test settings
                var logDeletionAgeTestValue = TimeSpan.FromMinutes(this.configReader.GetUnencryptedConfigValue(
                                                  this.initParam.SectionName,
                                                  FolderProducerValidator.TestDataDeletionAgeParamName,
                                                  0));
                if (logDeletionAgeTestValue != TimeSpan.Zero)
                {
                    if (logDeletionAgeTestValue > FolderProducerConstants.MaxDataDeletionAge)
                    {
                        this.traceSource.WriteWarning(
                            this.logSourceId,
                            "The value {0} specified for section {1}, parameter {2} is greater than the maximum permitted value {3}. Therefore the maximum permitted value will be used instead.",
                            logDeletionAgeTestValue,
                            this.initParam.SectionName,
                            FolderProducerValidator.TestDataDeletionAgeParamName,
                            FolderProducerConstants.MaxDataDeletionAge);
                        logDeletionAgeTestValue = FolderProducerConstants.MaxDataDeletionAge;
                    }

                    this.folderProducerSettings.DataDeletionAge = logDeletionAgeTestValue;
                }

                string pathList = string.Join(", ", this.folderProducerSettings.Paths);

                // Write settings to log
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Folder processing enabled: Folder paths: {0}, Data deletion age ({1}): {2}",
                    pathList,
                    (logDeletionAgeTestValue == TimeSpan.Zero) ? "days" : "minutes",
                    (logDeletionAgeTestValue == TimeSpan.Zero) ? dataDeletionAge : logDeletionAgeTestValue);
            }
            else
            {
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Folder processing not enabled");
            }
        }

        private bool GetFolderType()
        {
            if (this.configReader.IsReadingFromApplicationManifest)
            {
                // Figure out the folder type from the application manifest.
                if (this.initParam.SectionName.StartsWith(
                        AppConfig.FolderSourceElement,
                        StringComparison.Ordinal))
                {
                    this.folderProducerSettings.Type = FolderProducerType.CustomFolder;
                }
                else if (this.initParam.SectionName.StartsWith(
                            AppConfig.CrashDumpSourceElement,
                            StringComparison.Ordinal))
                {
                    this.folderProducerSettings.Type = FolderProducerType.WindowsFabricCrashDumps;
                }
                else
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to determine the value of the parameter {0}.",
                        FolderProducerValidator.FolderTypeParamName);
                    this.folderProducerSettings.Enabled = false;
                    return false;
                }
            }
            else
            {
                // Get the folder type from the cluster manifest
                string folderTypeAsString = this.configReader.GetUnencryptedConfigValue(
                                                 this.initParam.SectionName,
                                                 FolderProducerValidator.FolderTypeParamName,
                                                 string.Empty);
                if (string.IsNullOrEmpty(folderTypeAsString))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Required parameter {0} was not specified in section {1}.",
                        FolderProducerValidator.FolderTypeParamName,
                        this.initParam.SectionName);
                    this.folderProducerSettings.Enabled = false;
                    return false;
                }

                this.folderProducerSettings.Type = this.GetFolderTypeFromString(
                                                        folderTypeAsString);

                if (FolderProducerType.WindowsFabricCrashDumps == this.folderProducerSettings.Type)
                {
                    this.folderProducerSettings.KernelCrashUploadEnabled = this.configReader.GetUnencryptedConfigValue(
                                                     this.initParam.SectionName,
                                                     FolderProducerValidator.KernelCrashUploadEnabled,
                                                     FolderProducerConstants.KernelCrashUploadEnabledDefault);
                }

                if (FolderProducerType.Invalid == this.folderProducerSettings.Type)
                {
                    this.folderProducerSettings.Enabled = false;
                    return false;
                }
            }

            return true;
        }

        private FolderProducerType GetFolderTypeFromString(string folderTypeAsString)
        {
            FolderProducerType folderProducerType = FolderProducerType.Invalid;

            if (folderTypeAsString.Equals(
                    FolderProducerValidator.WindowsFabricCrashDumps,
                    StringComparison.Ordinal) ||
                folderTypeAsString.Equals(
                    FolderProducerValidator.ServiceFabricCrashDumps,
                    StringComparison.Ordinal))
            {
                folderProducerType = FolderProducerType.WindowsFabricCrashDumps;
            }
            else if (folderTypeAsString.Equals(
                         FolderProducerValidator.WindowsFabricPerformanceCounters,
                         StringComparison.Ordinal) ||
                folderTypeAsString.Equals(
                    FolderProducerValidator.ServiceFabricPerformanceCounters,
                         StringComparison.Ordinal))
            {
                folderProducerType = FolderProducerType.WindowsFabricPerformanceCounters;
            }
            else if (folderTypeAsString.Equals(
                FolderProducerValidator.CustomFolder,
                         StringComparison.Ordinal))
            {
                folderProducerType = FolderProducerType.CustomFolder;
            }
            else
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Folder type {0} specified in section {1} parameter {2} is not supported.",
                    folderTypeAsString,
                    this.initParam.SectionName,
                    FolderProducerValidator.FolderTypeParamName);
            }

            return folderProducerType;
        }

        private bool GetFolderPath()
        {
            if (this.configReader.IsReadingFromApplicationManifest)
            {
                // Get the folder path from the application manifest
                string relativePath = this.configReader.GetUnencryptedConfigValue(
                                          this.initParam.SectionName,
                                          FolderProducerValidator.AppRelativeFolderPathParamName,
                                          string.Empty);
                string folderPath = Path.Combine(
                                        this.configReader.GetApplicationLogFolder(),
                                        relativePath);
                this.folderProducerSettings.Paths.Add(folderPath);
            }
            else
            {
                // Get the folder path from the cluster manifest
                Debug.Assert(FolderProducerType.CustomFolder == this.folderProducerSettings.Type, "Custom folder is the only other type of FolderProducer allowed.");
                string folderPath = this.configReader.GetUnencryptedConfigValue(
                                        this.initParam.SectionName,
                                        FolderProducerValidator.FolderPathParamName,
                                        string.Empty);
                if (string.IsNullOrEmpty(folderPath))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Parameter {0} was specified in section {1} but parameter {2} was not.",
                        FolderProducerValidator.FolderTypeParamName,
                        this.initParam.SectionName,
                        FolderProducerValidator.FolderPathParamName);
                    this.folderProducerSettings.Enabled = false;
                    return false;
                }

                this.folderProducerSettings.Paths.Add(folderPath);
            }

            return true;
        }

        private void GetCrashDumpFolders()
        {
            if (this.configReader.IsReadingFromApplicationManifest)
            {
                HashSet<string> exeNames = new HashSet<string>();
                Dictionary<string, ServiceConfig> serviceConfigs = ConfigReader.GetServiceConfigsForApp(
                                                                       this.initParam.ApplicationInstanceId);
                foreach (ServiceConfig serviceConfig in serviceConfigs.Values)
                {
                    IEnumerable<string> exesForService = serviceConfig.ExeNames;
                    exeNames.UnionWith(exesForService);
                }

                foreach (string exeName in exeNames)
                {
                    string crashDumpFolder;
                    if (GetCrashDumpFolder(exeName, out crashDumpFolder))
                    {
                        this.folderProducerSettings.Paths.Add(crashDumpFolder);
                    }
                    else
                    {
                        // Application could have been deleted by the time we try to enable
                        // crash dump collection. So it is possible that we may not find
                        // the crash dump folder for that application.
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Unable to retrieve the folder containing crash dumps for {0}",
                            exeName);
                    }
                }
            }
            else
            {
                string folderPath = Path.Combine(
                                        this.initParam.LogDirectory,
                                        FolderProducerConstants.CrashDumpsDirectory);
                this.folderProducerSettings.Paths.Add(folderPath);

                // Order of folder addition in list is important.
                // Only the first element needs the permissions set up correctly.
                if (this.folderProducerSettings.KernelCrashUploadEnabled)
                {
                    string linuxKernelCrashFolderPath =
                        FolderProducerHelper.GetKernelCrashFolder(this.traceSource, this.logSourceId);
                    if (linuxKernelCrashFolderPath != string.Empty)
                    {
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Found kernel crash dump folder {0}",
                            linuxKernelCrashFolderPath);
                        this.folderProducerSettings.Paths.Add(linuxKernelCrashFolderPath);
                    }
                    else
                    {
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Kernel crash upload is configured but failed to get kernel crash dump folder.");
                    }
                }
            }
        }

        // Settings related to folder producers
        private struct FolderProducerSettings
        {
            internal bool Enabled;
            internal FolderProducerType Type;
            internal List<string> Paths;
            internal TimeSpan DataDeletionAge;

            // Add Linux Specific settings here.
            internal bool KernelCrashUploadEnabled;
        }
    }
}