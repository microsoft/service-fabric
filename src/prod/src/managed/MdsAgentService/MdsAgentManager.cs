// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMdsAgentSvc
{
    using System;
    using System.Collections.Specialized;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Health;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;
    using AzureUploaderCommon;

    // This class implements the functionality to launch the MDS agent and manage its lifetime
    internal class MdsAgentManager
    {
        private const uint CtrlBreakEventCode = 1;
        private const string MonAgentLauncherHealthProperty = "MonAgentLauncher";
        private const string MonAgentCoreHealthProperty = "MonAgentCore";
        private const string MonAgentCoreIsNotRunningMessage = "MonAgentCore is not running.";
        private const string MonAgentCoreIsRunning = "MonAgentCore is running.";

        private readonly StatelessServiceInitializationParameters initializationParameters;
        private readonly CodePackageActivationContext codePackageActivationContext;
        private readonly string maDataPackagePath;
        private readonly string maConfigFileFullPath;
        private readonly string maWorkFolder;

        private readonly string maTenant;
        private readonly string maRole;
        private readonly string maApp;
        private readonly string maRoleInstance;
        private readonly string maNodeName;
        private readonly string maDeploymentId;
        private readonly string maDataCenter;

        private readonly string maXStoreAccounts;
        private readonly string maMdmAccountName;
        private readonly string maMdmNamespace;
        private readonly TimeSpan maRestartDelay;

        private readonly object mdsAgentStateLock;
        private readonly HealthReporter healthReporter;
        private readonly MonitoringAgentServiceEvent trace;

        private readonly Timer maResumeTimer;
        private readonly EventWaitHandle maStoppedEvent;
        private readonly EventWaitHandle maStartedEvent;
        private readonly Thread maStatusHealthMonitor;
        private readonly string maStoppedEventHandleName;
        private readonly string maStartedEventHandleName;
        private readonly string mdsLogDirectoryFullPath;

        private Process mdsAgentLauncherProcess;
        private MdsAgentState mdsAgentState;
        private TaskCompletionSource<object> stopTaskCompletionSource;

        internal MdsAgentManager(
            StatelessServiceInitializationParameters initializationParameters, 
            MonitoringAgentServiceEvent trace)
        {
            this.stopTaskCompletionSource = null;
            this.initializationParameters = initializationParameters;
            this.trace = trace;
            this.codePackageActivationContext = this.initializationParameters.CodePackageActivationContext;

            this.codePackageActivationContext.ConfigurationPackageModifiedEvent += ConfigPackageModified;
            this.codePackageActivationContext.DataPackageModifiedEvent += DataPackageModified;
            string maConfigFileName = ConfigReader.GetConfigValue(
                                            Constants.ConfigSectionName,
                                            Constants.MAConfigFileParamName,
                                            Constants.DefaultMAConfigFileName);
            DataPackage maDataPackage = this.codePackageActivationContext.GetDataPackageObject(Constants.MdsAgentDataPackageName);
            this.maDataPackagePath = maDataPackage.Path;
            this.maConfigFileFullPath = Path.Combine(this.maDataPackagePath, maConfigFileName);

            this.maWorkFolder = Path.Combine(this.codePackageActivationContext.WorkDirectory, Constants.MdsAgentWorkSubFolderName);
            Directory.CreateDirectory(this.maWorkFolder);

            this.maTenant = ConfigReader.GetConfigValue(
                                Constants.ConfigSectionName,
                                Constants.ClusterNameParamName,
                                Constants.DefaultClusterName);
            this.maDataCenter = ConfigReader.GetConfigValue(
                                Constants.ConfigSectionName,
                                Constants.DataCenterParamName,
                                Constants.DefaultDataCenterName);

            // FixDisallowedCharacters replaces special chars in application name with underscore.
            // This is required since MA cannot handle all special chars in identity fields.
            // MA usually allows chars that are supported in a URI. 
            this.maApp = this.codePackageActivationContext.ApplicationName.FixDisallowedCharacters();
            this.maRole = this.maApp;

            // The actual nodeName as reported by the FabricClient. This may contain chars like '.' (dot).
            // In current MA identity, the dot is replaced by and underscore in Role and RoleInstance value
            // however, the node name itself needs to retain its original value to match that reported by the HM for consistency in downstream processing.
            this.maNodeName = FabricRuntime.GetNodeContext().NodeName;
            this.maRoleInstance = this.maNodeName.FixDisallowedCharacters();
            this.maDeploymentId = AzureRegistryStore.DeploymentId;

            this.maXStoreAccounts = ConfigReader.GetConfigValue(
                                        Constants.ConfigSectionName,
                                        Constants.MAXStoreAccountsParamName,
                                        string.Empty);

            using (var fc = new FabricClient())
            {
                this.mdsLogDirectoryFullPath = ClusterManifestParser.ParseMdsUploaderPath(fc.ClusterManager.GetClusterManifestAsync().Result);
            }

            if (!string.IsNullOrEmpty(this.mdsLogDirectoryFullPath))
            {
                this.trace.Message(
                    "MdsAgentManager.ctor: Fabric Geneva Warm Path event generation detected at {0}.", 
                    this.mdsLogDirectoryFullPath);
            }

            this.maMdmAccountName = ConfigReader.GetConfigValue(
                Constants.ConfigSectionName,
                Constants.MAMdmAccountNameParamName,
                string.Empty);

            this.maMdmNamespace = ConfigReader.GetConfigValue(
                Constants.ConfigSectionName,
                Constants.MAMdmNamespaceParamName,
                string.Empty);

            int maRestartDelaySeconds = ConfigReader.GetConfigValue(
                                            Constants.ConfigSectionName,
                                            Constants.MARestartDelayInSecondsParamName,
                                            Constants.DefaultMARestartDelayInSeconds);
            this.maRestartDelay = TimeSpan.FromSeconds(maRestartDelaySeconds);

            this.mdsAgentState = MdsAgentState.Initialized;
            this.mdsAgentStateLock = new object();

            this.healthReporter = new HealthReporter(initializationParameters.PartitionId, initializationParameters.InstanceId, initializationParameters.ServiceTypeName);
            this.maStoppedEventHandleName = string.Format(
                "MAStoppedEvent{0}{1}",
                this.initializationParameters.PartitionId,
                this.initializationParameters.InstanceId);
            this.maStartedEventHandleName = string.Format(
                "MAStartedEvent{0}{1}",
                this.initializationParameters.PartitionId,
                this.initializationParameters.InstanceId);

            this.maStoppedEvent = new EventWaitHandle(false, EventResetMode.AutoReset, this.maStoppedEventHandleName);
            this.maStartedEvent = new EventWaitHandle(false, EventResetMode.AutoReset, this.maStartedEventHandleName);
            this.maStatusHealthMonitor = new Thread(this.MAStatusHealthMonitor) { IsBackground = false };
            this.maStatusHealthMonitor.Start();

            this.maResumeTimer = new Timer(this.Resume);
        }

        private enum MdsAgentState
        {
            Initialized,
            Running,
            Paused,
            Stopping,
            Stopped
        }

        internal void Start()
        {
            lock (this.mdsAgentStateLock)
            {
                switch (this.mdsAgentState)
                {
                    case MdsAgentState.Initialized:
                        this.StartWorker();
                        this.mdsAgentState = MdsAgentState.Running;
                        break;

                    default:
                        var unexpectedStateMessage =
                            string.Format(
                                "MdsAgentManager.Start: MonAgentLauncher could not be started because we were in an unexpected state {0}.",
                                this.mdsAgentState);
                        this.trace.Error(unexpectedStateMessage);
                        Environment.FailFast(unexpectedStateMessage);
                        break;
                }
            }
        }

        internal Task StopAsync()
        {
            Task stopTask = null;
            lock (this.mdsAgentStateLock)
            {
                switch (this.mdsAgentState)
                {
                    case MdsAgentState.Paused:
                        this.PreventResume();
                        this.mdsAgentState = MdsAgentState.Stopped;

                        // Create a task that is already completed
                        stopTask = Utility.CreateCompletedTask<object>(null);
                        break;
                    case MdsAgentState.Running:
                        this.SendCtrlBreak();
                        this.mdsAgentState = MdsAgentState.Stopping;

                        // Create a task that will be completed when we have fully stopped
                        this.stopTaskCompletionSource = new TaskCompletionSource<object>();
                        stopTask = this.stopTaskCompletionSource.Task;
                        break;
                    default:
                        var unexpectedStateMessage =
                            string.Format(
                                "MdsAgentManager.StopAsync: Could not trigger MonAgentLauncher stop because we were in an unexpected state {0}.",
                                this.mdsAgentState);
                        this.trace.Error(unexpectedStateMessage);
                        Environment.FailFast(unexpectedStateMessage);
                        break;
                }
            }

            return stopTask;
        }

        private static void DataPackageModified(object sender, PackageModifiedEventArgs<DataPackage> e)
        {
            Environment.Exit(0);
        }

        private static void ConfigPackageModified(object sender, PackageModifiedEventArgs<ConfigurationPackage> e)
        {
            Environment.Exit(0);
        }

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool GenerateConsoleCtrlEvent(uint ctrlEvent, uint processGroupId);

        /// <summary>
        /// Monitors reported running status of MonAgentCore.
        /// Reports error by default until MA is running.
        /// </summary>
        /// <param name="context">Unused parameter.</param>
        private void MAStatusHealthMonitor(object context)
        {
            this.healthReporter.ReportHealth(
                HealthState.Error,
                MonAgentCoreHealthProperty,
                MonAgentCoreIsNotRunningMessage);

            while (true)
            {
                this.maStartedEvent.WaitOne();
                this.healthReporter.ReportHealth(
                    HealthState.Ok,
                    MonAgentCoreHealthProperty,
                    MonAgentCoreIsRunning);

                this.maStoppedEvent.WaitOne();
                this.healthReporter.ReportHealth(
                    HealthState.Error,
                    MonAgentCoreHealthProperty,
                    MonAgentCoreIsNotRunningMessage);
            }
        }

        private void MdsAgentLauncherProcessExited(object sender, EventArgs e)
        {
            var message = string.Format(
                "MdsAgentManager: MonAgentLauncher has exited with error code {0}.",
                this.mdsAgentLauncherProcess.ExitCode);
            this.trace.Error(message);

            bool completeStopTask = false;
            lock (this.mdsAgentStateLock)
            {
                switch (this.mdsAgentState)
                {
                    case MdsAgentState.Running:
                        // The MonAgentLauncher exited even though we didn't ask it to. This 
                        // means that it was asked to exit by some external entity. Most likely,
                        // it was because of a Fabric upgrade, which includes installing the 
                        // MSI (among other things). The MSI installation likely needed to replace
                        // some Windows Fabric file that the MA had an open handle to. Therefore,
                        // the Restart Manager probably caused the MA and hence the MonAgentLauncher
                        // to exit, so that the MSI installation can replace that file.
                        //
                        // In the above situation, the Fabric upgrade will soon cause us to exit and
                        // when we come back up we will start the MonAgentLauncher/MA again as always.
                        //
                        // However, we cannot be 100% sure that the MonAgentLauncher really exited due
                        // to Fabric upgrade. Therefore, to be on the safe side, we schedule the 
                        // MonAgentLauncher to be started again after a delay. This is to cover for 
                        // the possibility that our assumption about the Fabric upgrade is wrong.
                        this.ScheduleResume();
                        this.mdsAgentLauncherProcess.Dispose();
                        this.mdsAgentState = MdsAgentState.Paused;
                        this.healthReporter.ReportHealth(
                            HealthState.Error,
                            MonAgentLauncherHealthProperty,
                            message);
                        break;

                    case MdsAgentState.Stopping:
                        this.mdsAgentLauncherProcess.Dispose();
                        completeStopTask = true;
                        this.mdsAgentState = MdsAgentState.Stopped;
                        break;

                    default:
                        var unexpectedStateMessage =
                            string.Format(
                                "MdsAgentManager: MonAgentLauncher exit notification was received when we were in an unexpected state {0}.",
                                this.mdsAgentState);
                        this.trace.Error(unexpectedStateMessage);
                        Environment.FailFast(unexpectedStateMessage);
                        break;
                }
            }

            if (completeStopTask)
            {
                this.stopTaskCompletionSource.SetResult(null);
            }
        }

        private void StartWorker()
        {
            this.StartMdsAgentLauncher();

            this.trace.Message("MdsAgentManager.StartWorker: MonAgentLauncher is running.");

            this.healthReporter.ReportHealth(
                HealthState.Ok,
                MonAgentLauncherHealthProperty,
                "MdsAgentManager.StartWorker: MonAgentLauncher is running.");
        }

        private void StartMdsAgentLauncher()
        {
            this.mdsAgentLauncherProcess = new Process();
            this.mdsAgentLauncherProcess.EnableRaisingEvents = true;
            this.mdsAgentLauncherProcess.Exited += this.MdsAgentLauncherProcessExited;
            this.mdsAgentLauncherProcess.StartInfo.UseShellExecute = false;
            this.mdsAgentLauncherProcess.StartInfo.FileName = Path.Combine(this.maDataPackagePath, Constants.MdsAgentLauncherExeName);
            this.mdsAgentLauncherProcess.StartInfo.Arguments = string.Format("-useenv -StopEvent {0} -StartEvent {1}", this.maStoppedEventHandleName, this.maStartedEventHandleName);
            this.mdsAgentLauncherProcess.StartInfo.WorkingDirectory = this.maWorkFolder;

            var launcherEnvironmentVariables = this.mdsAgentLauncherProcess.StartInfo.EnvironmentVariables;

            this.AddEnvironmentVariable(launcherEnvironmentVariables, Constants.MonitoringDataDirEnvVar, this.maWorkFolder);
            this.AddEnvironmentVariable(launcherEnvironmentVariables, Constants.MonitoringIdentityEnvVar, Constants.MonitoringIdentityValue);
            this.AddEnvironmentVariable(launcherEnvironmentVariables, Constants.MonitoringInitConfigEnvVar, this.maConfigFileFullPath);
            this.AddEnvironmentVariable(launcherEnvironmentVariables, Constants.MonitoringVersionEnvVar, Constants.MonitoringVersionValue);
            this.AddEnvironmentVariable(launcherEnvironmentVariables, Constants.MonitoringTenantEnvVar, this.maTenant);
            this.AddEnvironmentVariable(launcherEnvironmentVariables, Constants.MonitoringDataCenterEnvVar, this.maDataCenter);
            this.AddEnvironmentVariable(launcherEnvironmentVariables, Constants.MonitoringRoleEnvVar, this.maRole);
            this.AddEnvironmentVariable(launcherEnvironmentVariables, Constants.MonitoringAppEnvVar, this.maApp);
            this.AddEnvironmentVariable(launcherEnvironmentVariables, Constants.MonitoringRoleInstanceEnvVar, this.maRoleInstance);
            this.AddEnvironmentVariable(launcherEnvironmentVariables, Constants.MonitoringNodeNameEnvVar, this.maNodeName);
            this.AddEnvironmentVariable(launcherEnvironmentVariables, Constants.MonitoringDeploymentIdEnvVar, this.maDeploymentId);
            this.AddEnvironmentVariable(launcherEnvironmentVariables, Constants.MonitoringXStoreAccountsEnvVar, this.maXStoreAccounts);
            this.AddEnvironmentVariable(launcherEnvironmentVariables, Constants.MonitoringMdmAccountNameEnvVar, this.maMdmAccountName);
            this.AddEnvironmentVariable(launcherEnvironmentVariables, Constants.MonitoringMdmNamespaceEnvVar, this.maMdmNamespace);

            // GCS parameters
            this.AddGcsParameter(launcherEnvironmentVariables, "MONITORING_GCS_ENVIRONMENT", "GCSEnvironment");
            this.AddGcsParameter(launcherEnvironmentVariables, "MONITORING_GCS_ACCOUNT", "GCSAccount");
            this.AddGcsParameter(launcherEnvironmentVariables, "MONITORING_GCS_NAMESPACE", "GCSNamespace");
            this.AddGcsParameter(launcherEnvironmentVariables, "MONITORING_GCS_REGION", "GCSRegion");
            this.AddGcsParameter(launcherEnvironmentVariables, "MONITORING_GCS_ENDPOINT", "GCSEndpoint");
            this.AddGcsParameter(launcherEnvironmentVariables, "MONITORING_GCS_THUMBPRINT", "GCSThumbprint");
            this.AddGcsParameter(launcherEnvironmentVariables, "MONITORING_GCS_CERTSTORE", "GCSCertStore");
            this.AddGcsParameter(launcherEnvironmentVariables, "MONITORING_GCS_DSMSURL", "GCSDsmsUrl");
            this.AddGcsParameter(launcherEnvironmentVariables, "MONITORING_AGENT_VERSION", "GCSAgentVersion");
            this.AddGcsParameter(launcherEnvironmentVariables, "MONITORING_USE_GENEVA_CONFIG_SERVICE", "UseGCS");
            this.AddGcsParameter(launcherEnvironmentVariables, "MONITORING_CONFIG_VERSION", "GCSConfigVersion");
            this.AddGcsParameter(launcherEnvironmentVariables, "MONITORING_GCS_EXACT_VERSION", "GCSExactVersion");

            // optional AzSecPack parameters
            this.AddGcsParameter(launcherEnvironmentVariables, "AZSECPACK_PILOT_FEATURES", "AZSECPACK_PILOT_FEATURES");
            this.AddGcsParameter(launcherEnvironmentVariables, "AZSECPACK_CUSTOMPOLICY_LOCATION", "AZSECPACK_CUSTOMPOLICY_LOCATION");
            this.AddGcsParameter(launcherEnvironmentVariables, "AZSECPACK_DISABLED_FEATURES", "AZSECPACK_DISABLED_FEATURES");
            this.AddGcsParameter(launcherEnvironmentVariables, "AZSECPACK_DISABLED_SCENARIOS", "AZSECPACK_DISABLED_SCENARIOS");

            this.AddEnvironmentVariable(launcherEnvironmentVariables, Constants.LogDirectoryEnvVar, this.mdsLogDirectoryFullPath);

            this.AddEnvironmentVariable(
                launcherEnvironmentVariables, 
                "MONITORING_SF_IS_LOG_DIRECTORY_SET", 
                string.IsNullOrWhiteSpace(this.mdsLogDirectoryFullPath) ? "0" : "1");

            this.mdsAgentLauncherProcess.Start();

            this.trace.Message(
                "MdsAgentManager.StartMdsAgentLauncher: MonAgentLauncher started. Process ID: {0}",
                this.mdsAgentLauncherProcess.Id);
        }

        private void AddEnvironmentVariable(StringDictionary environmentVariables, string envVarName, string envVarValue)
        {
            if (!environmentVariables.ContainsKey(envVarName))
            {
                this.trace.Message(
                    "MdsAgentManager.AddEnvironmentVariable: Adding {0}={1}", envVarName, envVarValue);

                environmentVariables.Add(envVarName, envVarValue);
            }
            else
            {
                this.trace.Message(
                    "MdsAgentManager.AddEnvironmentVariable: Skipping {0}. Variable already set globally.", envVarName);
            }
        }

        private void AddGcsParameter(StringDictionary environmentVariables, string envVarName, string configParamName)
        {
            var paramValue = this.GetConfigValue(configParamName);
            if (!string.IsNullOrEmpty(paramValue))
            {
                this.AddEnvironmentVariable(environmentVariables, envVarName, paramValue);
            }
        }

        private void SendCtrlBreak()
        {
            if (!GenerateConsoleCtrlEvent(CtrlBreakEventCode, (uint)this.mdsAgentLauncherProcess.Id))
            {
                int win32Error = Marshal.GetLastWin32Error();
                this.trace.Error(
                    "MdsAgentManager.SendCtrlBreak: Could not send Ctrl+Break to MonAgentLauncher. Error {0} encountered.",
                    win32Error);
            }
            else
            {
                this.trace.Message(
                    "MdsAgentManager.SendCtrlBreak: Ctrl+Break signal was sent to MonAgentLauncher.");
            }
        }

        private void ScheduleResume()
        {
            if (!this.maResumeTimer.Change(this.maRestartDelay, TimeSpan.MaxValue))
            {
                this.trace.Error(
                    "MdsAgentManager.ScheduleResume: Unable to start timer to schedule the resuming of MonAgentLauncher.");
                throw new InvalidOperationException("Unable to start timer to schedule the resuming of MonAgentLauncher.");
            }
            else
            {
                this.trace.Message(
                    "MdsAgentManager.ScheduleResume: MonAgentLauncher is rescheduled to resume after {0} milliseconds.",
                    this.maRestartDelay.TotalMilliseconds);
            }
        }

        private void PreventResume()
        {
            this.maResumeTimer.Change(Timeout.Infinite, Timeout.Infinite);
            this.trace.Error(
                "MdsAgentManager.PreventResume: Resuming of MonAgentLauncher is canceled because we are stopping.");
        }

        private void Resume(object state)
        {
            lock (this.mdsAgentStateLock)
            {
                switch (this.mdsAgentState)
                {
                    case MdsAgentState.Paused:
                        this.StartWorker();
                        this.mdsAgentState = MdsAgentState.Running;
                        break;
                    case MdsAgentState.Stopped:
                        // The MDS agent exited on its own and before we could restart it,
                        // we ourselves were asked to stop. Therefore, we tried to cancel
                        // the restart timer that we had started, but it was too late. Just
                        // ignore the resume instructions in this case.
                        break;
                    default:
                        this.trace.Error(
                            "MdsAgentManager.Resume: Could not resume MonAgentLauncher because we were in an unexpected state {0}.",
                            this.mdsAgentState);
                        break;
                }
            }
        }

        private string GetConfigValue(string paramName, string defaultValue = "")
        {
            return ConfigReader.GetConfigValue(Constants.ConfigSectionName, paramName, defaultValue);
        }
    }
}