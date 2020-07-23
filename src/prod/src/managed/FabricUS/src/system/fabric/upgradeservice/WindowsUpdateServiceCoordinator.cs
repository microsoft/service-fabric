// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Health;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Runtime.Serialization;
    using System.Text;
    using System.Threading.Tasks;
    using System.Threading;
    using System.Xml;
    using Microsoft.Win32;
    using System.Collections.Generic;

    internal sealed class WindowsUpdateServiceCoordinator : IUpgradeCoordinator
    {
        private static TraceType TraceType = new TraceType("WindowsUpdateServiceCoordinator");
        private readonly string CurrentUpgradeKey = "CurrentUpgrade";
        private readonly FabricClientWrapper commandProcessor;
        private readonly KeyValueStoreReplica store;
        private readonly IStatefulServicePartition partition;
        private readonly bool testMode;
        private readonly string testSrcDir;
        private readonly TimeSpan waitTimeBeforePolling;
        private readonly TimeSpan windowsUpdateApiTimeout;
        private readonly IExceptionHandlingPolicy exceptionPolicy;

        private Task pollingTask;
        private UpdatePackageRetriever packageRetriever;
        private Uri serviceName;
        private IConfigStore configStore;
        private string configSectionName;

        public WindowsUpdateServiceCoordinator(IConfigStore configStore, string configSection, KeyValueStoreReplica kvsReplica, IStatefulServicePartition partition, IExceptionHandlingPolicy exceptionPolicy)
        {
            configStore.ThrowIfNull("configStore");
            configSection.ThrowIfNullOrWhiteSpace("configSectionName");
            kvsReplica.ThrowIfNull("kvsReplica");
            partition.ThrowIfNull("partition");
            exceptionPolicy.ThrowIfNull("exceptionPolicy");

            this.commandProcessor = new FabricClientWrapper();
            this.store = kvsReplica;
            this.partition = partition;
            this.packageRetriever = new UpdatePackageRetriever();
            this.serviceName = new Uri(Constants.SystemServicePrefix + configSection);
            this.configStore = configStore;
            this.configSectionName = configSection;
            this.exceptionPolicy = exceptionPolicy;

            // Read all the configuration values
            string coordinatorType = configStore.ReadUnencryptedString(configSection, Constants.ConfigurationSection.CoordinatorType);
            this.testMode = false;
            this.testSrcDir = string.Empty;
            if (coordinatorType.Equals(Constants.ConfigurationSection.WUTestCoordinator))
            {
                this.testSrcDir = configStore.ReadUnencryptedString(configSection, Constants.WUSCoordinator.TestCabFolderParam);
                this.testMode = true;
            }            

            this.waitTimeBeforePolling = TimeSpan.FromMinutes(Constants.WUSCoordinator.PollingIntervalInMinutesDefault);
            this.windowsUpdateApiTimeout = TimeSpan.FromMinutes(Constants.WUSCoordinator.WuApiTimeoutInMinutesDefault);
        }

        public string ListeningAddress
        {
            get { return string.Empty; }
        }

        public IEnumerable<Task> StartProcessing(CancellationToken token)
        {
            Trace.WriteInfo(TraceType, "StartProcessing called");
            if (this.pollingTask != null && !this.pollingTask.IsCompleted)
            {
                throw new InvalidOperationException();
            }

            this.pollingTask = this.DoWorkAsync(token);
            return new List<Task>() { this.pollingTask };
        }

        private async Task DoWorkAsync(CancellationToken token)
        {
            Trace.WriteInfo(TraceType, "Start the polling");
            while (!token.IsCancellationRequested)
            {
                token.ThrowIfCancellationRequested();
                await this.WaitForPartitionReadyAsync(token);
                var waitDuration = await this.UpgradeWorkAsync(token).ContinueWith(
                    (task) =>
                    {
                        if (task.IsCanceled)
                        {
                            Trace.WriteError(TraceType, "Polling task got cancelled");
                            return TimeSpan.MinValue;
                        }
                        else if (task.IsFaulted)
                        {
                            Trace.WriteError(TraceType, "Polling task faulted with error: {0}", task.Exception);
                            return this.waitTimeBeforePolling;
                        }
                        else
                        {
                            Trace.WriteInfo(TraceType, "Poll completed");
                            return task.Result;
                        }
                    }, token);

                if (waitDuration != TimeSpan.MinValue)
                {
                    await Task.Delay(waitDuration, token);
                }
            }
        }

        // Returns the duration to wait before trying again..
        private async Task<TimeSpan> UpgradeWorkAsync(CancellationToken token)
        {
            Trace.WriteInfo(TraceType, "UpgradeWorkAsync: Get fabric upgrade progress");
            var upgradeProgress = await this.commandProcessor.GetFabricUpgradeProgressAsync(Constants.MaxOperationTimeout, token);
            var upgradeState = upgradeProgress.UpgradeState;
            if (upgradeProgress.TargetCodeVersion == "0.0.0.0")
            {
                return await this.InitiateBaseUpgradeAsync(token);
            }
            else if (upgradeState == FabricUpgradeState.RollingBackCompleted || upgradeState == FabricUpgradeState.RollingForwardCompleted)
            {
                var baseUpgrade = this.configStore.ReadUnencryptedString(this.configSectionName, Constants.WUSCoordinator.OnlyBaseUpgradeParam);
                bool onlyBaseUpgrade = false;
                if (!bool.TryParse(baseUpgrade, out onlyBaseUpgrade))
                {
                    onlyBaseUpgrade = false;
                }

                if (onlyBaseUpgrade)
                {
                    Trace.WriteInfo(TraceType, "Only base upgrade");
                    return this.waitTimeBeforePolling;
                }

                var waitDuration = await this.GetWaitDurationAndUpdateHealth(upgradeProgress, token);
                if (waitDuration != TimeSpan.MinValue && waitDuration.TotalMinutes > 0)
                {
                    return waitDuration;
                }

                Trace.WriteInfo(TraceType, "UpgradeWorkAsync: Check for the next upgrade package");
                NativeMethods.IUpdateCollection updateCollections = null;
                if (!this.testMode)
                {
                    updateCollections = await this.packageRetriever.GetAvailableUpdates(this.windowsUpdateApiTimeout, token);
                }

                var upgradeStarted = await this.OnGettingUpdateCollectionAsync(updateCollections, upgradeProgress, token);
                if (upgradeStarted)
                {
                    return TimeSpan.MinValue;
                }
            }
            else if (upgradeState == FabricUpgradeState.RollingBackInProgress ||
                     upgradeState == FabricUpgradeState.RollingForwardInProgress)
            {
                Trace.WriteInfo(
                    TraceType, 
                    "UpgradeWorkAsync: upgrade to code version:{0} config version: {1} and state:{2} happening.", 
                    upgradeProgress.TargetCodeVersion, 
                    upgradeProgress.TargetConfigVersion,
                    upgradeProgress.UpgradeState);

                await this.UpdateReplicaStoreAfterUpgradeStartAsync(upgradeProgress, token);
                var lastUpgradeProgress = await this.commandProcessor.GetCurrentRunningUpgradeTaskAsync(TimeSpan.MaxValue, token);
                Trace.WriteInfo(TraceType,
                                "UpgradeWorkAsync: Upgrade completed state: {0}, Code version:{1}, Config version:{2}",
                                lastUpgradeProgress.UpgradeState,
                                lastUpgradeProgress.TargetCodeVersion,
                                lastUpgradeProgress.TargetConfigVersion);
                return TimeSpan.MinValue;
            }
            else
            {
                Trace.WriteError(
                    TraceType,
                    "UpgradeWorkAsync: Invalid Current upgrade state '{0}'",
                    upgradeState);
            }

            return this.waitTimeBeforePolling;
        }

        private async Task<TimeSpan> InitiateBaseUpgradeAsync(CancellationToken token)
        {
            string binRootPath = FabricEnvironment.GetBinRoot();
            var deployerExePath = Path.Combine(binRootPath, @"Fabric\Fabric.Code\FabricDeployer.exe");
            if (!File.Exists(deployerExePath))
            {
                throw new FileNotFoundException(deployerExePath);
            }

            var cabFile = Path.GetTempFileName();
            cabFile = Path.ChangeExtension(cabFile, "cab");

            using (CabFileCreator creator = new CabFileCreator())
            {
                creator.CreateFciContext(cabFile);
                creator.AddFile(deployerExePath);
                creator.FlushCabinet();
            }

            var fileVersion = FileVersionInfo.GetVersionInfo(deployerExePath);
            Trace.WriteInfo(TraceType, "Get current config file.");
            var commandParameter = new ClusterUpgradeCommandParameter
            {
                ConfigFilePath = await this.GetCurrentConfigFile(),
                CodeFilePath = cabFile,
                CodeVersion = string.Format(
                    CultureInfo.InvariantCulture,
                    "{0}.{1}.{2}.{3}",
                    fileVersion.FileMajorPart,
                    fileVersion.FileMinorPart,
                    fileVersion.FileBuildPart,
                    fileVersion.FilePrivatePart)
            };

            await this.commandProcessor.ClusterUpgradeAsync(commandParameter, TimeSpan.MaxValue, token);
            return TimeSpan.MinValue;
        }

        private async Task<TimeSpan> GetWaitDurationAndUpdateHealth(FabricUpgradeProgress upgradeProgress, CancellationToken token)
        {
            TimeSpan waitDuration = TimeSpan.MinValue;
            using (var tx = await this.store.CreateTransactionWithRetryAsync(this.exceptionPolicy, token))
            {
                KeyValueStoreItem storeItem = null;
                CurrentUpgradeInformation updateInformation = null;
                if (this.store.Contains(tx, this.CurrentUpgradeKey))
                {
                    try
                    {
                        storeItem = this.store.Get(tx, this.CurrentUpgradeKey);
                        updateInformation = CurrentUpgradeInformation.Deserialize(storeItem.Value);
                    }
                    catch (Exception ex)
                    {
                        Trace.WriteError(TraceType, "Update state deserialize failed with exception {0}", ex);
                        updateInformation = null;
                    }
                }

                if (updateInformation != null && 
                    (upgradeProgress.UpgradeState == FabricUpgradeState.Failed ||
                     upgradeProgress.UpgradeState == FabricUpgradeState.RollingBackCompleted))
                {
                    if (updateInformation.CurrentCodeVersion == upgradeProgress.TargetCodeVersion &&
                        updateInformation.CurrentConfigVersion == upgradeProgress.TargetConfigVersion)
                    {
                        // Error so report Warning health for the service
                        var description = string.Format(
                            CultureInfo.InvariantCulture,
                            "Upgrade failed for Update Id:{0} Target code version: {1}",
                            updateInformation.UpdateId,
                            updateInformation.TargetCodeVersion);
                        this.ReportHealthEvent(description, HealthState.Warning);
                        waitDuration = this.waitTimeBeforePolling;
                    }
                }
                else
                {
                    this.ReportHealthEvent("Health", HealthState.Ok);
                }
            }

            return waitDuration;
        }

        // Return true if upgrade started other wise false
        private async Task<bool> OnGettingUpdateCollectionAsync(
            NativeMethods.IUpdateCollection updateCollection,
            FabricUpgradeProgress upgradeProgress,
            CancellationToken token)
        {
            ClusterUpgradeCommandParameter commandParameter = null;
            string updateId = string.Empty;

            if (updateCollection != null)
            {
                NativeMethods.IUpdate updateToDownload = this.GetUpdateToDownload(updateCollection);
                if (updateToDownload != null)
                {
                    Trace.WriteInfo(TraceType, "OnGettingUpdateCollectionAsync: Update to download: {0}", updateToDownload.Identity.UpdateID);
                    commandParameter = await this.packageRetriever.DownloadWindowsUpdate(updateToDownload, this.windowsUpdateApiTimeout, token);
                    updateId = updateToDownload.Identity.UpdateID;
                }
                else
                {
                    Trace.WriteInfo(TraceType, "OnGettingUpdateCollectionAsync: No update found.");
                }
            }
            else
            {
                Trace.WriteInfo(TraceType, "OnGettingUpdateCollectionAsync: update collection is null.");
                if (this.testMode && Directory.Exists(this.testSrcDir))
                {
                    var srcCabFile = Directory.GetFiles(this.testSrcDir, "*.cab", SearchOption.TopDirectoryOnly).FirstOrDefault();
                    if (!string.IsNullOrWhiteSpace(srcCabFile))
                    {
                        Trace.WriteWarning(TraceType, "OnGettingUpdateCollectionAsync: Test cab file {0}", srcCabFile);
                        var dir = Path.Combine(Path.GetTempPath(), Path.GetRandomFileName());
                        var info = new DirectoryInfo(dir);
                        if (!info.Exists)
                        {
                            info.Create();
                        }

                        updateId = Guid.NewGuid().ToString();
                        var destCabFile = Path.Combine(dir, "test.cab");
                        File.Copy(srcCabFile, destCabFile);
                        Trace.WriteWarning(TraceType, "OnGettingUpdateCollectionAsync: Test dest file {0}", destCabFile);
                        var cabVersion = CabFileOperations.GetCabVersion(destCabFile);
                        Trace.WriteWarning(TraceType, "OnGettingUpdateCollectionAsync: Cab version {0}", cabVersion);
                        commandParameter = new ClusterUpgradeCommandParameter()
                        {
                            CodeFilePath = destCabFile,
                            CodeVersion = cabVersion
                        };
                    }
                }
            }

            if (commandParameter == null)
            {
                return false;
            }

            var updatedCommandParameter = await this.UpdateReplicaStoreBeforeUpgradeStartAsync(
                commandParameter,
                upgradeProgress,
                updateId,
                token);
            var upgradeTask = await this.commandProcessor.ClusterUpgradeAsync(updatedCommandParameter, TimeSpan.MaxValue, token).ContinueWith(
                (task) =>
                {
                    if (commandParameter != null)
                    {
                        DeleteFileDirectory(commandParameter.CodeFilePath);
                        DeleteFileDirectory(commandParameter.ConfigFilePath);
                    }

                    return task;
                });

            await upgradeTask;
            return true;
        }

        private NativeMethods.IUpdate GetUpdateToDownload(NativeMethods.IUpdateCollection updateCollection)
        {
            try
            {
                NativeMethods.IUpdate updateToDownload = null;
                if (updateCollection.Count > 0)
                {
                    foreach (NativeMethods.IUpdate update in updateCollection)
                    {
                        if (updateToDownload != null)
                        {
                            Trace.WriteWarning(TraceType, "GetUpdateToDownload: Update collection has more then one update: {0}", update.Identity.UpdateID);
                        }
                        else
                        {
                            updateToDownload = update;
                        }
                    }
                }

                // Taking the first one
                return updateToDownload;
            }
            catch (Exception ex)
            {
                Trace.WriteError(TraceType, "GetUpdateToDownload: Error : {0}", ex);
            }

            return null;
        }

        #region Health Reporting

        private void ReportHealthEvent(string description, HealthState healthState)
        {
            HealthInformation information = new HealthInformation()
            {
                HealthState = healthState,
                Description = description,
                Property = "UpgradeStatus",
                RemoveWhenExpired = false,
                SourceId = "System.UpgradeService",
            };

            ServiceHealthReport report = new ServiceHealthReport(this.serviceName, information);
            this.commandProcessor.FabricClient.HealthManager.ReportHealth(report);
        }

        #endregion
        #region Replica store update

        private async Task WaitForPartitionReadyAsync(CancellationToken token)
        {
            while (this.partition.WriteStatus != PartitionAccessStatus.Granted)
            {
                await Task.Delay(Constants.PartitionReadyWaitInMilliseconds, token);
            }
        }

        private async Task UpdateReplicaStoreAfterUpgradeStartAsync(
            FabricUpgradeProgress upgradeProgress,
            CancellationToken token)
        {
            try
            {
                await this.WaitForPartitionReadyAsync(token);
                using (var tx = await this.store.CreateTransactionWithRetryAsync(this.exceptionPolicy, token))
                {
                    KeyValueStoreItem storeItem = null;
                    CurrentUpgradeInformation updateInformation = null;
                    if (this.store.Contains(tx, CurrentUpgradeKey))
                    {
                        try
                        {
                            storeItem = this.store.Get(tx, CurrentUpgradeKey);
                            updateInformation = CurrentUpgradeInformation.Deserialize(storeItem.Value);
                            if (!updateInformation.UpgradeStarted)
                            {
                                updateInformation.UpgradeStarted = true;
                            }
                        }
                        catch (Exception ex)
                        {
                            Trace.WriteError(TraceType, "Update state deserialize failed with exception {0}", ex);
                            updateInformation = null;
                        }
                    }

                    if (updateInformation != null &&
                        !updateInformation.UpgradeStarted &&
                        updateInformation.TargetCodeVersion == upgradeProgress.TargetCodeVersion &&
                        (updateInformation.TargetConfigVersion == null ||
                         updateInformation.TargetConfigVersion == upgradeProgress.TargetConfigVersion))
                    {
                        updateInformation.UpgradeStarted = true;
                        this.store.Update(tx, CurrentUpgradeKey, updateInformation.Serialize(),
                            storeItem.Metadata.SequenceNumber);
                        await tx.CommitAsync();
                    }
                }
            }
            catch (Exception ex)
            {
                Trace.WriteError(TraceType, "UpdateReplicaStoreAfterUpgradeStartAsync: Error : {0}", ex);
            }
        }

        private async Task<ClusterUpgradeCommandParameter> UpdateReplicaStoreBeforeUpgradeStartAsync(
            ClusterUpgradeCommandParameter commandParameter,
            FabricUpgradeProgress upgradeProgress,
            string updateId, 
            CancellationToken token)
        {
            try
            {
                await this.WaitForPartitionReadyAsync(token);
                Trace.WriteInfo(TraceType, "UpdateReplicaStoreBeforeUpgradeStartAsync: Update replica store.");
                KeyValueStoreItem storeItem = null;
                using (var tx = await this.store.CreateTransactionWithRetryAsync(this.exceptionPolicy, token))
                {
                    CurrentUpgradeInformation updateInformation = null;
                    if (this.store.Contains(tx, CurrentUpgradeKey))
                    {
                        storeItem = this.store.Get(tx, CurrentUpgradeKey);
                        try
                        {
                            updateInformation = CurrentUpgradeInformation.Deserialize(storeItem.Value);
                            if (updateInformation.UpgradeStarted && updateId == updateInformation.UpdateId)
                            {
                                updateInformation.Retry++;
                                updateInformation.UpgradeStarted = false;
                            }
                        }
                        catch (Exception ex)
                        {
                            Trace.WriteError(TraceType, "Deserialize failed with exception {0}", ex);
                            updateInformation = null;
                        }
                    }

                    if (updateInformation == null || updateId != updateInformation.UpdateId)
                    {
                        updateInformation = new CurrentUpgradeInformation
                        {
                            Retry = 0,
                            TargetCodeVersion = commandParameter.CodeVersion,
                            TargetConfigVersion = commandParameter.ConfigVersion,
                            CurrentCodeVersion = upgradeProgress.TargetCodeVersion,
                            CurrentConfigVersion = upgradeProgress.TargetConfigVersion,
                            UpdateId = updateId,
                            UpgradeStarted = false
                        };
                    }

                    Trace.WriteInfo(TraceType, "Updating store for update Id: {0}, Retry: {1} ", updateId, updateInformation.Retry);
                    if (storeItem != null)
                    {
                        this.store.Update(tx, CurrentUpgradeKey, updateInformation.Serialize(), storeItem.Metadata.SequenceNumber);
                    }
                    else
                    {
                        this.store.Add(tx, CurrentUpgradeKey, updateInformation.Serialize());
                    }

                    await tx.CommitAsync();
                    Trace.WriteInfo(TraceType, "UpdateReplicaStoreBeforeUpgradeStartAsync: replica store updated");
                }
            }
            catch (Exception ex)
            {
                Trace.WriteError(TraceType, "UpdateReplicaStoreBeforeUpgradeStartAsync: Error : {0}", ex);
                commandParameter = null;
            }

            if (commandParameter != null)
            {
                if (commandParameter.UpgradeDescription == null)
                {
                    commandParameter.UpgradeDescription = new CommandProcessorClusterUpgradeDescription();
                }

                commandParameter.UpgradeDescription.ForceRestart = true;
            }

            return commandParameter;
        }

        #endregion

        private async Task<string> GetCurrentConfigFile()
        {
            return await this.commandProcessor.GetCurrentClusterManifestFile().ContinueWith(
                (task) =>
                {
                    if (task.IsCanceled || task.IsFaulted)
                    {
                        return string.Empty;
                    }

                    return task.Result;
                }
                );
        }

        private static void DeleteFileDirectory(string fileName)
        {
            try
            {
                if (!string.IsNullOrWhiteSpace(fileName))
                {
                    var codeFileDir = Path.GetDirectoryName(fileName);
                    if (Directory.Exists(codeFileDir))
                    {
                        Directory.Delete(codeFileDir, true);
                    }
                }
            }
            catch
            {
                Trace.WriteWarning(TraceType, "Cleanup failed: Directory {0} couldn't be deleted", fileName);
            }
        }

        #region CurrentUpgradeInformation class
        /// <summary>
        /// internal class to store current upgrade information
        /// </summary>
        [DataContract]
        internal class CurrentUpgradeInformation 
        {
            static readonly DataContractSerializer Serializer = new DataContractSerializer(typeof(CurrentUpgradeInformation));

            [DataMember]
            public string UpdateId { get; set; }

            [DataMember]
            public int Retry { get; set; }

            [DataMember]
            public string TargetCodeVersion { get; set; }

            [DataMember]
            public string TargetConfigVersion { get; set; }

            [DataMember]
            public string CurrentCodeVersion { get; set; }

            [DataMember]
            public string CurrentConfigVersion { get; set; }

            [DataMember]
            public bool UpgradeStarted { get; set; }

            internal byte[] Serialize()
            {
                using (var stringWriter = new StringWriter())
                {
                    using (var writer = XmlWriter.Create(stringWriter))
                    {
                        Serializer.WriteObject(writer, this);
                        writer.Flush();
                        return Encoding.UTF8.GetBytes(stringWriter.ToString());
                    }
                }
            }

            internal static CurrentUpgradeInformation Deserialize(byte[] bytes)
            {
                using (var reader = new StringReader(Encoding.UTF8.GetString(bytes)))
                {
                    XmlReaderSettings settings = new XmlReaderSettings();
                    settings.XmlResolver = null;
                    using (var xmlReader = XmlReader.Create(reader, settings))
                    {
                        return Serializer.ReadObject(xmlReader) as CurrentUpgradeInformation;
                    }
                }
            }
        }

        #endregion
    }
}