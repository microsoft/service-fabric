// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
#if !DotNetCoreClr
    using Diagnostics.Eventing.Reader;
#endif
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.IO;
    using System.Linq;
    using System.Runtime.InteropServices;

    internal class RemoveOperation : DeploymentOperation
    {
        protected override void OnExecuteOperation(DeploymentParameters parameters, ClusterManifestType clusterManifest, Infrastructure infrastructure)
        {
            try
            {
                CleanupDeployment(parameters);
            }
            finally
            {
#if !DotNetCoreClr
                if (AccountHelper.IsAdminUser())
                {
                    CollectEventLogs();
                }
#endif
            }
        }

        protected void CleanupDeployment(DeploymentParameters parameters)
        {
#if !DotNetCoreClr
            if (AccountHelper.IsAdminUser())
            {
                FabricDeployerServiceController.DisableService();
                if (!parameters.SkipFirewallConfiguration)
                {
                    FirewallManager.DisableFirewallSettings();
                }
                DeployerTrace.WriteInfo("Stopping data collectors");
                PerformanceCounters.StopDataCollector();

                DeployerTrace.WriteInfo("Deleting data collectors");
                PerformanceCounters.DeleteDataCollector();
            }
            else
            {
                DeployerTrace.WriteWarning(
                    "Deployer is not run as Administrator. Skipping Firewall Management and Performance Counter Management. Possible Post remove cleanup required");
            }

            if (FabricDeployerServiceController.IsRunning(parameters.MachineName))
            {
                throw new InvalidDeploymentException(StringResources.Error_FabricDeployer_FabricHostStillRunning_Formatted);
            }
#else
            DeployerTrace.WriteInfo("CoreClr: Skipping Firewall Management and Performance Counter Management cleanup on CoreClr.");
#endif
            string targetInformationFileName = Path.Combine(parameters.FabricDataRoot, Constants.FileNames.TargetInformation);
            DeleteTargetInformationFile(targetInformationFileName);

#if DotNetCoreClr // Disable compiling on windows for now. Need to correct when porting FabricDeployer for windows.
            bool skipDeleteFabricDataRoot = Utility.GetSkipDeleteFabricDataRoot()
                || (parameters.SkipDeleteFabricDataRoot != null && string.Equals(parameters.SkipDeleteFabricDataRoot, "true", StringComparison.OrdinalIgnoreCase));

#else
            bool skipDeleteFabricDataRoot = Utility.GetSkipDeleteFabricDataRoot()
                || (parameters.SkipDeleteFabricDataRoot != null && string.Equals(parameters.SkipDeleteFabricDataRoot, "true", StringComparison.InvariantCultureIgnoreCase));
#endif

            if (skipDeleteFabricDataRoot)
            {
                DeployerTrace.WriteInfo("Skipping deletion of Data Root.");
            }
            else
            {
                NetCloseResource(parameters.FabricDataRoot);
                SafeDeleteDirectory(parameters.FabricDataRoot, parameters.FabricLogRoot, Path.Combine(parameters.FabricDataRoot, Constants.FileNames.FabricHostSettings));
                List<SettingsTypeSection> sections = new List<SettingsTypeSection>();
                sections.Add(new SettingsTypeSection() { Name = Constants.SectionNames.Setup });
                WriteFabricHostSettingsFile(parameters.FabricDataRoot, new SettingsType() { Section = sections.ToArray() }, parameters.MachineName);
            }

#if !DotNetCoreClr // Disable compiling on windows for now. Need to correct when porting FabricDeployer for windows.
            SpnManager.CleanupSpn();
#else
            DeployerTrace.WriteInfo("CoreClrLinux: SPN cleanning skipped for Linux");
#endif

#if !DotNetCoreClrIOT
            new DockerDnsHelper(parameters, string.Empty).CleanupAsync().GetAwaiter().GetResult();

            RemoveNetworks(parameters);
#endif
        }

#if !DotNetCoreClr // Disable compiling on windows for now. Need to correct when porting FabricDeployer for windows.
        private static void CollectEventLogs()
        {
            DeployerTrace.WriteInfo(StringResources.Info_CollectingErrorLogs);
            EventLogQuery adminQuery = new EventLogQuery(Constants.ServiceFabricAdminEventLogName, PathType.LogName, "*[System/Level=2]");
            EventLogQuery operationalQuery = new EventLogQuery(Constants.ServiceFabricOperationalEventLogName, PathType.LogName, "*[System/Level=2]");

            try
            {
                using (EventLogReader adminReader = new EventLogReader(adminQuery), operationalReader = new EventLogReader(operationalQuery))
                {
                    string path = Path.Combine(Utility.GetTempPath(Constants.LocalHostMachineName), Constants.EventLogsFileName);
                    if (File.Exists(path))
                    {
                        File.Delete(path);
                    }

                    DeployerTrace.WriteInfo(StringResources.Info_CopyingEventLogs, path);
                    using (StreamWriter tw = File.CreateText(path))
                    {
                        EventRecord adminEventRecord;
                        EventRecord operationalEventRecord;
                        while ((adminEventRecord = adminReader.ReadEvent()) != null)
                        {
                            tw.WriteLine(String.Format("{0} - {1} - {2} - {3}",
                                adminEventRecord.TimeCreated,
                                Environment.MachineName,
                                adminEventRecord.LevelDisplayName,
                                adminEventRecord.FormatDescription()));
                        }

                        while ((operationalEventRecord = operationalReader.ReadEvent()) != null)
                        {
                            tw.WriteLine(String.Format("{0} - {1} - {2} - {3}",
                                operationalEventRecord.TimeCreated,
                                Environment.MachineName,
                                operationalEventRecord.LevelDisplayName,
                                operationalEventRecord.FormatDescription()));
                        }
                    }
                }
            }
            catch (EventLogException e)
            {
                DeployerTrace.WriteWarning(StringResources.Error_IssueOccursWhenGetEventLogs, e);
            }
        }
#endif

        private static void DeleteTargetInformationFile(string targetInformationFile)
        {
            if (FabricFile.Exists(targetInformationFile))
            {
                try
                {
                    DeployerTrace.WriteInfo("Attempting to delete TargetInformationFile.");
                    FabricFile.Delete(targetInformationFile, true);
                }
                catch (Exception ex)
                {
                    DeployerTrace.WriteError("Failed to delete: {0}. Exception: {1}.", targetInformationFile, ex);
                    throw;
                }
            }
            else
            {
                DeployerTrace.WriteWarning("TargetInformationFile does not exist.");
            }
        }

        private static void SafeDeleteDirectory(string directoryName, string directoryFilter, string fileFilter)
        {
#if DotNetCoreClr // Disable compiling on windows for now. Need to correct when porting FabricDeployer for windows.
            if (!Directory.Exists(directoryName)
                || IsDirectorySymbolicLink(directoryName)
                || string.Equals(directoryName, directoryFilter, StringComparison.OrdinalIgnoreCase))
            {
                return;
            }
            // Avoid deleting any file locks
            var files = Directory.GetFiles(directoryName).Where(
                (f => !f.EndsWith(FileLock.ReaderLockExtension, StringComparison.OrdinalIgnoreCase) && !f.EndsWith(FileLock.WriterLockExtension, StringComparison.CurrentCultureIgnoreCase)));
#else
            if (!Directory.Exists(directoryName)
                || IsDirectorySymbolicLink(directoryName)
                || string.Equals(directoryName, directoryFilter, StringComparison.InvariantCultureIgnoreCase))
            {
                return;
            }
            // Avoid deleting any file locks
            var files = Directory.GetFiles(directoryName).Where(
                (f => !f.EndsWith(FileLock.ReaderLockExtension, StringComparison.InvariantCultureIgnoreCase) && !f.EndsWith(FileLock.WriterLockExtension, StringComparison.InvariantCultureIgnoreCase)));
#endif

            foreach (var file in files)
            {
                if (FabricFile.Exists(file) && !string.Equals(file, fileFilter, StringComparison.OrdinalIgnoreCase))
                {
                    try
                    {
                        FabricFile.Delete(file, true);
                    }
                    catch (Exception ex)
                    {
                        DeployerTrace.WriteError("SafeDeleteDirectory failed deleting file: {0}. Exception: {1}.", file, ex);
                        throw;
                    }

                }
            }

            foreach (var subDirectoryName in Directory.GetDirectories(directoryName))
            {
                SafeDeleteDirectory(subDirectoryName, directoryFilter, fileFilter);
            }

            if (!Directory.GetDirectories(directoryName).Any() && !Directory.GetFiles(directoryName).Any())
            {
                try
                {
                    TimeSpan retryInterval = TimeSpan.FromSeconds(5);
                    int retryCount = 12;
                    Type[] exceptionTypes = { typeof(System.IO.FileLoadException) };
                    Helpers.PerformWithRetry(() =>
                        {
                            try
                            {
                                FabricDirectory.Delete(directoryName);
                            }
                            catch(Exception ex)
                            {
                                DeployerTrace.WriteWarning("Directory Delete failed for {0} due to: {1}.", directoryName, ex.Message);
                                throw;
                            }
                        },
                        exceptionTypes,
                        retryInterval,
                        retryCount);
                }
                catch (Exception ex)
                {
                    DeployerTrace.WriteError("SafeDeleteDirectory failed deleting directory: {0}. Exception: {1}.", directoryName, ex);
                    throw;
                }
            }

            DeployerTrace.WriteInfo("Removed folder: {0}", directoryName);
        }

        private static bool IsDirectorySymbolicLink(string directoryName)
        {
            DirectoryInfo info = new DirectoryInfo(directoryName);
            return info.Attributes.HasFlag(FileAttributes.ReparsePoint);
        }

        private static void NetCloseResource(string path)
        {
            const int MAX_PREFERRED_LENGTH = -1;
            int readEntries;
            int totalEntries;
            IntPtr buffer = IntPtr.Zero;

            // Enumerate all resouces in this path that are open remotly
            int enumerateStatus = NativeMethods.NetFileEnum(null, path, null, 3, ref buffer, MAX_PREFERRED_LENGTH, out readEntries, out totalEntries, IntPtr.Zero);
            if (enumerateStatus == NativeMethods.ERROR_SUCCESS)
            {
                NativeMethods.FILE_INFO_3 fileInfo = new NativeMethods.FILE_INFO_3();
                for (int index = 0; index < readEntries; index++)
                {
                    IntPtr bufferPtr = new IntPtr(buffer.ToInt64() + (index * Marshal.SizeOf(fileInfo)));
                    fileInfo = (NativeMethods.FILE_INFO_3)Marshal.PtrToStructure(bufferPtr, typeof(NativeMethods.FILE_INFO_3));
                    int fileCloseStatus = NativeMethods.NetFileClose(null, fileInfo.fi3_id);
                    if (fileCloseStatus != NativeMethods.ERROR_SUCCESS)
                    {
                        DeployerTrace.WriteWarning(string.Format("Could not close resource {0}. Error code: {1}", fileInfo.fi3_pathname, fileCloseStatus));
                    }
                }

                NativeMethods.NetApiBufferFree(buffer);
            }
        }

#if !DotNetCoreClrIOT
        private static void RemoveNetworks(DeploymentParameters parameters)
        {
            var containerServiceArguments = (parameters.UseContainerServiceArguments) ? (parameters.ContainerServiceArguments) : (FlatNetworkConstants.ContainerServiceArguments);

#if !DotNetCoreClrLinux
            containerServiceArguments = (parameters.EnableContainerServiceDebugMode) 
                ? (string.Format("{0} {1}", containerServiceArguments, FlatNetworkConstants.ContainerProviderServiceDebugModeArg))
                : containerServiceArguments;
#endif

            bool exceptionOccurred = false;
            string exceptionMsg = string.Empty;
            try
            {
                // Clean up container network set up
                var containerNetworkCleanupOperation = new ContainerNetworkCleanupOperation();
                containerNetworkCleanupOperation.ExecuteOperation(parameters.ContainerNetworkName, containerServiceArguments, parameters.FabricDataRoot);
            }
            catch (Exception ex)
            {
                // we will throw this exception after isolated network clean up.
                exceptionOccurred = true;
                exceptionMsg = ex.Message;
            }

            try
            {
                // Clean up isolated network set up
                var isolatedNetworkCleanupOperation = new IsolatedNetworkCleanupOperation();
                isolatedNetworkCleanupOperation.ExecuteOperation(parameters.IsolatedNetworkName, parameters.FabricBinRoot);
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("Error occurred while cleaning up isolated network setup exception {0}", ex);
            }

            if (exceptionOccurred)
            {
                throw new InvalidDeploymentException(exceptionMsg);
            }
        }
#endif
    }
}