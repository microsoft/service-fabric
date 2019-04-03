// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeOrchestration.Service
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.Fabric.UpgradeService;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Net.Http;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Microsoft.ServiceFabric.DeploymentManager.Model;
    using Newtonsoft.Json;

    using DMConstants = Microsoft.ServiceFabric.DeploymentManager.Constants;

    internal class StandaloneGoalStateProvisioner
    {
        private const string TraceType = "UpgradeOrchestration.GoalStateProvisioner";

        private static readonly TimeSpan Time24Hrs = new TimeSpan(24, 0, 0);
        private static readonly Version ZeroVersion = new Version("0.0.0.0");

        private readonly StoreManager storeManager;
        private UpgradeOrchestrator orchestrator;
        private CancellationToken cancellationToken;

        private FabricEvents.ExtensionsEvents traceSource = UpgradeOrchestrationTrace.TraceSource;
        private TimeSpan provisioningTimeOfDay = DMConstants.DefaultProvisioningTimeOfDay;
        private TimeSpan timerInterval = Time24Hrs;

        private object provisionLock = new object();

        public StandaloneGoalStateProvisioner(
            StoreManager storeManager,
            UpgradeOrchestrator orchestrator,
            CancellationToken cancellationToken)
        {
            this.storeManager = storeManager;
            this.orchestrator = orchestrator;
            this.cancellationToken = cancellationToken;
        }

        public bool IsAutoupgradeEnabled()
        {
            NativeConfigStore configStore = NativeConfigStore.FabricGetConfigStore();
            return configStore.ReadUnencryptedBool(DMConstants.UpgradeOrchestrationServiceConfigSectionName, DMConstants.AutoupgradeEnabledName, UpgradeOrchestrationTrace.TraceSource, TraceType, true /*throwIfInvalid*/);
        }

        public bool IsAutoupgradeInstallEnabled()
        {
            NativeConfigStore configStore = NativeConfigStore.FabricGetConfigStore();
            return configStore.ReadUnencryptedBool(DMConstants.UpgradeOrchestrationServiceConfigSectionName, DMConstants.AutoupgradeInstallEnabledName, UpgradeOrchestrationTrace.TraceSource, TraceType, false /*throwIfInvalid*/);
        }

        public async Task SetUpGoalStatePoll(CancellationToken cancellationToken, bool skipInitialPoll = false)
        {
            ReleaseAssert.AssertIfNot(Monitor.TryEnter(this.provisionLock), "StandaloneGoalStateProvisioner polling can only run one at a time.");
            try
            {
                while (!cancellationToken.IsCancellationRequested)
                {
                    this.traceSource.WriteInfo(TraceType, "Starting SetUpGoalStatePoll.");
                    this.UpdateTimers();
                    if (skipInitialPoll)
                    {
                        skipInitialPoll = false;
                    }
                    else
                    {
                        await this.PollGoalStateForCodePackagesAsync(cancellationToken).ConfigureAwait(false);
                    }

                    DateTime current = DateTime.Now;
                    TimeSpan timeToInvoke;
                    if (this.timerInterval == Time24Hrs)
                    {
                        timeToInvoke = this.provisioningTimeOfDay - current.TimeOfDay;
                        if (timeToInvoke < TimeSpan.Zero)
                        {
                            timeToInvoke += Time24Hrs;
                        }
                    }
                    else
                    {
                        timeToInvoke = this.timerInterval;
                    }

                    this.traceSource.WriteInfo(TraceType, "Scheduling SetUpGoalStatePoll to continue in {0}.", timeToInvoke);
                    await Task.Delay(timeToInvoke, cancellationToken).ConfigureAwait(false);
                }
            }
            finally
            {
                Monitor.Exit(this.provisionLock);
            }
        }

        internal void EmitGoalStateReachableHealth(FabricClient fabricClient, bool success)
        {
            this.traceSource.WriteInfo(TraceType, "Reporting GoalStateReachable Health: {0}", success);

            if (success)
            {
                fabricClient.HealthManager.ReportHealth(new ClusterHealthReport(new HealthInformation(
                        DMConstants.UpgradeOrchestrationHealthSourceId,
                        DMConstants.ClusterGoalStateReachableHealthProperty,
                        HealthState.Ok)
                {
                    TimeToLive = TimeSpan.FromMilliseconds(1),
                    RemoveWhenExpired = true
                }));
            }
            else
            {
                fabricClient.HealthManager.ReportHealth(new ClusterHealthReport(new HealthInformation(
                        DMConstants.UpgradeOrchestrationHealthSourceId,
                        DMConstants.ClusterGoalStateReachableHealthProperty,
                        HealthState.Warning)
                {
                    Description = StringResources.WarningHealth_ClusterGoalStateReachable,
                    TimeToLive = TimeSpan.FromHours(48),
                    RemoveWhenExpired = true
                }));
            }
        }

        internal void EmitClusterVersionSupportedHealth(FabricClient fabricClient, bool success, string currentVersion = null, DateTime? expiryDate = null)
        {
            this.traceSource.WriteInfo(TraceType, "Reporting ClusterVersionSupported Health: {0}", success);
            if (!success)
            {
                fabricClient.HealthManager.ReportHealth(new ClusterHealthReport(new HealthInformation(
                        DMConstants.UpgradeOrchestrationHealthSourceId,
                        DMConstants.ClusterVersionSupportHealthProperty,
                        HealthState.Warning)
                {
                    Description = string.Format(
                        StringResources.WarningHealth_ClusterVersionSupport, currentVersion, expiryDate),
                    TimeToLive = TimeSpan.FromHours(48),
                    RemoveWhenExpired = true
                }));
            }
            else
            {
                fabricClient.HealthManager.ReportHealth(new ClusterHealthReport(new HealthInformation(
                        DMConstants.UpgradeOrchestrationHealthSourceId,
                        DMConstants.ClusterVersionSupportHealthProperty,
                        HealthState.Ok)
                {
                    TimeToLive = TimeSpan.FromMilliseconds(1),
                    RemoveWhenExpired = true
                }));
            }
        }

        private static bool IsVersionUpgradeable(string version, GoalStateModel model)
        {
            PackageDetails package = model.Packages.Find(p => p.Version == version);
            return package == null || !package.IsUpgradeDisabled;
        }

        private static async Task ProvisionPackageAsync(string targetCodeVersion, string localPackagePath, CancellationToken cancellationToken)
        {
            var commandParameter = new ClusterUpgradeCommandParameter
            {
                ConfigFilePath = null,
                ConfigVersion = null,
                CodeFilePath = localPackagePath,
                CodeVersion = targetCodeVersion
            };

            var timeoutHelper = new System.Fabric.UpgradeService.TimeoutHelper(TimeSpan.MaxValue, System.Fabric.UpgradeService.Constants.MaxOperationTimeout);
            var commandProcessor = new FabricClientWrapper();
            await commandProcessor.CopyAndProvisionUpgradePackageAsync(
                commandParameter,
                commandParameter.CodeVersion,
                null,
                timeoutHelper,
                cancellationToken).ConfigureAwait(false);
        }

        private async Task PollGoalStateForCodePackagesAsync(CancellationToken cancellationToken)
        {
            var fabricClient = new FabricClient();

            //// region: Initialization Parameters

            FabricUpgradeProgress upgradeProgress;
            try
            {
                upgradeProgress = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync<FabricUpgradeProgress>(
                    () =>
                    fabricClient.ClusterManager.GetFabricUpgradeProgressAsync(TimeSpan.FromMinutes(DMConstants.FabricOperationTimeoutInMinutes), cancellationToken),
                    TimeSpan.FromMinutes(DMConstants.FabricOperationTimeoutInMinutes),
                    cancellationToken).ConfigureAwait(false);
            }
            catch (Exception ex)
            {
                this.traceSource.WriteError(TraceType, "GetFabricUpgradeProgressAsync threw: {0}", ex.ToString());
                this.timerInterval = TimeSpan.FromMinutes(DMConstants.FabricUpgradeStatePollIntervalInMinutes);
                return;
            }

            if (!FabricClientWrapper.IsUpgradeCompleted(upgradeProgress.UpgradeState))
            {
                this.timerInterval = TimeSpan.FromMinutes(DMConstants.FabricUpgradeStatePollIntervalInMinutes);
                this.traceSource.WriteInfo(TraceType, "Cannot retrieve cluster version; FabricUpgradeState is currently: {0}. Will retry in {1}.", upgradeProgress.UpgradeState.ToString(), this.timerInterval.ToString());
                return;
            }

            string currentVersionStr = upgradeProgress.TargetCodeVersion;
            Version currentVersion = Version.Parse(currentVersionStr);
            string packageDropDir = System.Fabric.Common.Helpers.GetNewTempPath();
            NativeConfigStore configStore = NativeConfigStore.FabricGetConfigStore();
            string goalStateUriStr = configStore.ReadUnencryptedString(DMConstants.UpgradeOrchestrationServiceConfigSectionName, DMConstants.GoalStateFileUrlName);
            if (string.IsNullOrEmpty(goalStateUriStr))
            {
                goalStateUriStr = DMConstants.DefaultGoalStateFileUrl;
            }

            //// endregion

            this.traceSource.WriteInfo(TraceType, "PollCodePackagesFromGoalState currentVersion: {0}, packageDropDir: {1} goalStateUri: {2}", currentVersionStr, packageDropDir, goalStateUriStr);
            try
            {
                Uri goalStateUri = null;
                string goalStateJson = null;
                if (!Uri.TryCreate(goalStateUriStr, UriKind.Absolute, out goalStateUri))
                {
                    string errorMessage = string.Format("Cannot parse GoalStateUri: {0}", goalStateUriStr);
                    this.traceSource.WriteError(TraceType, errorMessage);
                    ReleaseAssert.Fail(errorMessage);
                    return;
                }

                if (!(await StandaloneUtility.IsUriReachableAsync(goalStateUri).ConfigureAwait(false)))
                {
                    this.timerInterval = TimeSpan.FromMinutes(DMConstants.FabricGoalStateReachablePollIntervalInMinutes);
                    this.traceSource.WriteWarning(TraceType, "Cannot reach download uri for goal state file: {0}. Will retry in {1}.", goalStateUri.AbsoluteUri, this.timerInterval.ToString());
                    this.EmitGoalStateReachableHealth(fabricClient, false);
                    return;
                }
                else
                {
                    this.EmitGoalStateReachableHealth(fabricClient, true);
                }

                goalStateJson = await GoalStateModel.GetGoalStateFileJsonAsync(goalStateUri).ConfigureAwait(false);                               

                if (string.IsNullOrEmpty(goalStateJson))
                {
                    this.traceSource.WriteWarning(TraceType, "Loaded goal state JSON is empty.");
                    return;
                }

                GoalStateModel model = GoalStateModel.GetGoalStateModelFromJson(goalStateJson);
                if (model == null || model.Packages == null)
                {
                    this.traceSource.WriteWarning(TraceType, "Goal state JSON could not be deserialized:\n{0}", goalStateJson);
                    return;
                }

                string availableGoalStateVersions = string.Format("Available versions in goal state file: {0}", model.ToString());
                this.traceSource.WriteInfo(TraceType, availableGoalStateVersions);

                IEnumerable<PackageDetails> candidatePackages = model.Packages.Where(
                            package => currentVersion < Version.Parse(package.Version));

                if (candidatePackages.Count() == 0)
                {
                    this.traceSource.WriteInfo(TraceType, "No upgrades available.");
                }

                string versionCurrentStr = currentVersionStr;
                Version versionCurrent = currentVersion;
                PackageDetails targetPackage = null;
                for (IEnumerable<PackageDetails> availableCandidatePackages = candidatePackages.Where(
                            package => (package.MinVersion == null || Version.Parse(package.MinVersion) <= versionCurrent));
                     availableCandidatePackages.Count() > 0;
                     versionCurrentStr = targetPackage.Version,
                     versionCurrent = Version.Parse(versionCurrentStr),
                     availableCandidatePackages = candidatePackages.Where(
                                                      package => (versionCurrent < Version.Parse(package.Version)
                                                              && (package.MinVersion == null || Version.Parse(package.MinVersion) <= versionCurrent))))
                {
                    if (!IsVersionUpgradeable(versionCurrentStr, model))
                    {
                        this.traceSource.WriteInfo(TraceType, "Version {0} is not upgradeable.", versionCurrentStr);
                        break;
                    }

                    int numPackages = availableCandidatePackages.Count();
                    if (numPackages == 0)
                    {
                        this.traceSource.WriteInfo(TraceType, "No upgrade available.");
                        return;
                    }
                    else if (numPackages == 1)
                    {
                        targetPackage = availableCandidatePackages.First();
                    }
                    else
                    {
                        Version maxVersion = StandaloneGoalStateProvisioner.ZeroVersion;
                        foreach (PackageDetails package in availableCandidatePackages)
                        {
                            Version targetVersion;
                            if (!Version.TryParse(package.Version, out targetVersion))
                            {
                                this.traceSource.WriteWarning(TraceType, "Package {0} version could not be parsed. Trying another one.", package.Version);
                                continue;
                            }

                            if (targetVersion > maxVersion)
                            {
                                targetPackage = package;
                                maxVersion = targetVersion;
                            }
                        }
                    }

                    try
                    {
                        if (await this.IsPackageProvisionedAsync(targetPackage.Version, fabricClient, cancellationToken).ConfigureAwait(false))
                        {
                            this.traceSource.WriteInfo(TraceType, "Package {0} is already provisioned.", targetPackage.Version);
                            continue;
                        }
                    }
                    catch (Exception ex)
                    {
                        this.traceSource.WriteError(TraceType, "PackageIsProvisioned for {0} threw: {1}", targetPackage.Version, ex.ToString());
                    }

                    string packageLocalDownloadPath = Path.Combine(packageDropDir, string.Format(DMConstants.SFPackageDropNameFormat, targetPackage.Version));
                    if (await StandaloneUtility.DownloadPackageAsync(targetPackage.Version, targetPackage.TargetPackageLocation, packageLocalDownloadPath).ConfigureAwait(false))
                    {
                        try
                        {
                            this.traceSource.WriteInfo(TraceType, "Copying and provisioning version {0} from {1}.", targetPackage.Version, packageLocalDownloadPath);
                            await ProvisionPackageAsync(targetPackage.Version, packageLocalDownloadPath, cancellationToken).ConfigureAwait(false);
                        }
                        catch (Exception ex)
                        {
                            this.traceSource.WriteError(TraceType, "ProvisionPackageAsync for {0}, package {1} threw: {2}", targetPackage.Version, packageLocalDownloadPath, ex.ToString());
                        }
                    }
                }

                // Determine if current version is latest/doesn't have expiry date. If package expiring, post cluster health warning.
                PackageDetails currentPackage = model.Packages.Where(package => package.Version == currentVersionStr).FirstOrDefault();
                if (currentPackage != null && currentPackage.SupportExpiryDate != null && !currentPackage.IsGoalPackage && !this.IsAutoupgradeInstallEnabled() && this.IsPackageNearExpiry(currentPackage))
                {
                    this.traceSource.WriteWarning(TraceType, "Current version {0} expires {1}; emitting cluster warning.", currentVersionStr, currentPackage.SupportExpiryDate);
                    this.EmitClusterVersionSupportedHealth(fabricClient, false, currentVersionStr, currentPackage.SupportExpiryDate);
                }
                else
                {
                    this.traceSource.WriteInfo(TraceType, "Current version {0} is supported. Cluster health OK.", currentVersionStr);
                    this.EmitClusterVersionSupportedHealth(fabricClient, true, null, null);
                }

                if (targetPackage != null &&
                    !string.IsNullOrEmpty(targetPackage.Version) &&
                    this.IsAutoupgradeInstallEnabled())
                {
                    try
                    {
                        // fire off an upgrade.
                        this.StartCodeUpgrade(targetPackage.Version, fabricClient);
                    }
                    catch (Exception ex)
                    {
                        this.traceSource.WriteError(TraceType, "StartCodeUpgrade for version {0} threw: {1}", targetPackage.Version, ex.ToString());
                    }
                }
            }
            finally
            {
                if (Directory.Exists(packageDropDir))
                {
                    try
                    {
                        FabricDirectory.Delete(packageDropDir, true, true);
                    }
                    catch
                    {
                    }
                }
            }
        }

        private bool IsPackageNearExpiry(PackageDetails currentPackage)
        {
            bool isNearExpiry = false;
            NativeConfigStore configStore = NativeConfigStore.FabricGetConfigStore();

            string goalStateExpirationReminderInDaysString = configStore.ReadUnencryptedString(DMConstants.UpgradeOrchestrationServiceConfigSectionName, DMConstants.GoalStateExpirationReminderInDays);
            uint goalStateExpirationReminderInDays;
            if (string.IsNullOrEmpty(goalStateExpirationReminderInDaysString) || !uint.TryParse(goalStateExpirationReminderInDaysString, out goalStateExpirationReminderInDays))
            {
                this.traceSource.WriteInfo(
                    TraceType,
                    "goalStateExpirationReminderInDays value invalid: \"{0}\". Defaulting to {1}.",
                    goalStateExpirationReminderInDaysString,
                    DMConstants.DefaultGoalStateExpirationReminderInDays);
                goalStateExpirationReminderInDays = DMConstants.DefaultGoalStateExpirationReminderInDays;
            }

            if (currentPackage != null && currentPackage.SupportExpiryDate != null)
            {
                isNearExpiry = (currentPackage.SupportExpiryDate - DateTime.UtcNow).Value.TotalDays < goalStateExpirationReminderInDays;
            }

            return isNearExpiry;
        }

        private async void StartCodeUpgrade(string targetCodeVersion, FabricClient fabricClient)
        {
            this.traceSource.WriteInfo(TraceType, "StartCodeUpgrade with target code version: {0}", targetCodeVersion);

            var clusterHealth = await fabricClient.HealthManager.GetClusterHealthAsync().ConfigureAwait(false);
            if (clusterHealth.AggregatedHealthState != HealthState.Ok)
            {
                this.traceSource.WriteInfo(TraceType, "Aggregate Health State: {0} is not OK. Skipping code upgrade with target code version: {1}", clusterHealth.AggregatedHealthState, targetCodeVersion);
                return;
            }

            this.traceSource.WriteInfo(TraceType, "Retrieve current cluster resource from StoreManager.");
            StandAloneCluster cluster = await this.storeManager.GetClusterResourceAsync(
                Constants.ClusterReliableDictionaryKey, this.cancellationToken).ConfigureAwait(false);

            if (cluster == null)
            {
                return;
            }

            if (cluster.TargetCsmConfig == null)
            {
                // use the current csm config if no targetcsmconfig exists
                cluster.TargetCsmConfig = cluster.Current.CSMConfig;
            }

            cluster.TargetCsmConfig.CodeVersion = targetCodeVersion;
            await this.storeManager.PersistClusterResourceAsync(Constants.ClusterReliableDictionaryKey, cluster, this.cancellationToken).ConfigureAwait(false);

            CodeUpgradeDetail packageDetails = new CodeUpgradeDetail()
            {
                CodeVersion = targetCodeVersion,
                IsUserInitiated = false
            };
            await this.storeManager.PersistCodeUpgradePackageDetailsAsync(Constants.CodeUpgradePackageDetails, packageDetails, this.cancellationToken);

            this.traceSource.WriteInfo(TraceType, "Invoking Orchestrator.StartUpgradeAsync from GoalStateProvisioner");
            await this.orchestrator.StartUpgradeAsync(cluster, this.cancellationToken, new Description.ConfigurationUpgradeDescription());
        }

        private void UpdateTimers()
        {
            try
            {
                NativeConfigStore configStore = NativeConfigStore.FabricGetConfigStore();

                string goalStateFetchIntervalInSecondsString = configStore.ReadUnencryptedString(DMConstants.UpgradeOrchestrationServiceConfigSectionName, DMConstants.GoalStateFetchIntervalInSecondsName);
                int goalStateFetchIntervalInSeconds;
                if (string.IsNullOrEmpty(goalStateFetchIntervalInSecondsString) || !int.TryParse(goalStateFetchIntervalInSecondsString, out goalStateFetchIntervalInSeconds))
                {
                    this.traceSource.WriteInfo(
                        TraceType,
                        "goalStateFetchIntervalInSeconds value invalid: \"{0}\". Defaulting to {1}.",
                        goalStateFetchIntervalInSecondsString,
                        DMConstants.DefaultGoalStateFetchIntervalInSeconds);
                    goalStateFetchIntervalInSeconds = DMConstants.DefaultGoalStateFetchIntervalInSeconds;
                }

                this.timerInterval = TimeSpan.FromSeconds(goalStateFetchIntervalInSeconds);

                string goalStateProvisioningTimeOfDayString = configStore.ReadUnencryptedString(DMConstants.UpgradeOrchestrationServiceConfigSectionName, DMConstants.GoalStateProvisioningTimeOfDayName);
                TimeSpan goalStateProvisioningTimeOfDay;
                if (string.IsNullOrEmpty(goalStateProvisioningTimeOfDayString) || !TimeSpan.TryParse(goalStateProvisioningTimeOfDayString, out goalStateProvisioningTimeOfDay))
                {
                    this.traceSource.WriteInfo(
                        TraceType,
                        "goalStateProvisioningTimeOfDay value invalid: \"{0}\". Defaulting to {1}.",
                        goalStateProvisioningTimeOfDayString,
                        DMConstants.DefaultProvisioningTimeOfDay);
                    goalStateProvisioningTimeOfDay = DMConstants.DefaultProvisioningTimeOfDay;
                }

                this.provisioningTimeOfDay = goalStateProvisioningTimeOfDay;
                this.traceSource.WriteInfo(
                    TraceType,
                    "Timers set to: TimerInterval: {0}, ProvisioningTimeOfDay: {1}.",
                    this.timerInterval.ToString(),
                    this.provisioningTimeOfDay.ToString());
            }
            catch (Exception ex)
            {
                this.traceSource.WriteError(TraceType, "UpdateTimers threw {0}", ex.ToString());
                throw;
            }
        }

        private async Task<bool> IsPackageProvisionedAsync(string targetCodeVersion, FabricClient fabricClient, CancellationToken cancellationToken)
        {
            ProvisionedFabricCodeVersionList provisionedCodeList = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync<ProvisionedFabricCodeVersionList>(
                () =>
                fabricClient.QueryManager.GetProvisionedFabricCodeVersionListAsync(targetCodeVersion),
                TimeSpan.FromMinutes(DMConstants.FabricOperationTimeoutInMinutes),
                cancellationToken).ConfigureAwait(false);
            if (provisionedCodeList.Count != 0)
            {
                this.traceSource.WriteInfo(TraceType, "PackageIsProvisioned: {0}", targetCodeVersion);
                return true;
            }

            return false;
        }
    }
}