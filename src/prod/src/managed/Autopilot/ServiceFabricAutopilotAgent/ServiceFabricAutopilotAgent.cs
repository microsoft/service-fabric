// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Autopilot
{
    using System;
    using System.Fabric.Management.ServiceModel;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.ServiceProcess;
    using System.Threading;
    using System.Threading.Tasks;

    using Microsoft.Fabric.InfrastructureWrapper;

    using Microsoft.Search.Autopilot;   

    internal class ServiceFabricAutopilotAgent
    {       
        private readonly Action<bool> exitCallback;
        
        private readonly ServiceFabricAutopilotAgentStatus agentStatus;

        private ServiceFabricLocalNodeManager localNodeManager;

        private Task backgroundStateMachineTask;
        private CancellationTokenSource backgroundStateMachineCancellationTokenSource;               

        public ServiceFabricAutopilotAgent(Action<bool> exitCallback)
        {
            this.exitCallback = exitCallback;            

            this.agentStatus = new ServiceFabricAutopilotAgentStatus();

            this.backgroundStateMachineCancellationTokenSource = new CancellationTokenSource();

            this.backgroundStateMachineTask = null;        
        }

        /* 
         * Test mode allows testing of ServiceFabricAutopilotAgent outside an Autopilot hosting environment.
         * In test mode, all dependencies on AP Runtime are replaced by test environment equivalences:
         *      Logging => text log
         *      Configuration => default test configurations
         */
        public void Start(bool useTestMode = false)
        {
            string agentApplicationDirectory = Path.GetDirectoryName(Assembly.GetEntryAssembly().Location);

            if (!useTestMode)
            {
                APRuntime.Initialize(StringConstants.ApplicationConfigurationFileName);
            }

            TextLogger.InitializeLogging(StringConstants.CustomLogIdName, useTestMode, StringConstants.TestModeLogFileNameTemplate, StringConstants.TestModeLogDirectoryName, StringConstants.TestModeTraceListenerName);

            TextLogger.LogInfo("Starting Service Fabric Autopilot Agent.");

            if (!ConfigurationManager.GetCurrentConfigurations(useTestMode))
            {
                this.Exit(false);

                return;
            }

            TextLogger.SetLogLevel(ConfigurationManager.LogLevel);

            ConfigurationManager.LogCurrentConfigurations();

            ServiceFabricLocalNodeManager localNodeManager;
            if (!ServiceFabricLocalNodeManager.Create(agentApplicationDirectory, useTestMode, out localNodeManager))
            {
                this.Exit(false);

                return;
            }

            this.localNodeManager = localNodeManager;

            this.agentStatus.InitializeAgentStatus();

            this.RegisterConsoleCancelEventNotifications();

            this.backgroundStateMachineTask = Task.Factory.StartNew(
                () => this.RunBackgroundStateMachine(this.backgroundStateMachineCancellationTokenSource.Token),
                this.backgroundStateMachineCancellationTokenSource.Token,
                TaskCreationOptions.LongRunning,
                TaskScheduler.Default);
        }

        private async Task RunBackgroundStateMachine(CancellationToken cancellationToken)
        {
            while (!cancellationToken.IsCancellationRequested)
            {
                this.RunStateMachine();

                await Task.Delay(TimeSpan.FromSeconds(ConfigurationManager.BackgroundStateMachineRunIntervalSeconds), cancellationToken);
            }
        }

        private void ConsoleCancelEventHanlder(object sender, ConsoleCancelEventArgs args)
        {
            TextLogger.LogInfo("Cancel event (Ctrl + C / Ctrl + Break) received. Stopping Service Fabric Autopilot Agent.");

            if (this.agentStatus.GetCurrentState() < ServiceFabricAutopilotAgentState.Started)
            {
                return;
            }

            this.agentStatus.SetNextState(ServiceFabricAutopilotAgentState.Stopping, false);            

            this.backgroundStateMachineCancellationTokenSource.Cancel();

            this.localNodeManager.StopLocalNode();            

            this.agentStatus.SetNextState(ServiceFabricAutopilotAgentState.Stopped, false);

            TextLogger.LogInfo("Service Fabric Autopilot Agent has stopped. Exiting the process.");

            this.Exit(true);
        }

        private void RunStateMachine()
        {
            bool toRunStateMachineCore = false;

            lock (this.agentStatus.StatusLock)
            {
                if (this.agentStatus.State < ServiceFabricAutopilotAgentState.Started)
                {
                    return;
                }

                TextLogger.LogVerbose("RunStateMachine : {0}.", this.agentStatus);

                if (this.agentStatus.LastFailedDeploymentAction != DateTime.MinValue && (DateTime.UtcNow - this.agentStatus.LastFailedDeploymentAction).TotalSeconds < ConfigurationManager.DeploymentRetryIntervalSeconds)
                {
                    return;
                }

                // Note that when a deployer operation (node configuration creation or update) fails, it would be retried based on the same cluster manifest (bootstrap or current).
                // I.e a retry would happen even when a previous operation failed with the same cluster manifest.
                if ((this.agentStatus.State > ServiceFabricAutopilotAgentState.Started && agentStatus.State < ServiceFabricAutopilotAgentState.DiscoveringTopology) ||
                    (this.agentStatus.State == ServiceFabricAutopilotAgentState.DiscoveringTopology && ConfigurationManager.BootstrapClusterManifestLocation != BootstrapClusterManifestLocationType.NotReady) ||
                    (this.agentStatus.State > ServiceFabricAutopilotAgentState.DiscoveringTopology && agentStatus.State < ServiceFabricAutopilotAgentState.NodeStarted) ||
                    (this.agentStatus.State == ServiceFabricAutopilotAgentState.NodeStarted && !this.agentStatus.ExpectedTopologyRunning))
                {
                    TextLogger.LogInfo(
                        "RunStateMachine: current agent state necessitates RunStateMachineCore. Agent status = {0}, ExpectedTopologyRunning = {1}, LastFailedDeploymentAction = {2}, BootstrapClusterManifestLocation = {3}.",
                        this.agentStatus.State,
                        this.agentStatus.ExpectedTopologyRunning,
                        this.agentStatus.LastFailedDeploymentAction,
                        ConfigurationManager.BootstrapClusterManifestLocation);

                    toRunStateMachineCore = true;           
                }                
            }

            if (toRunStateMachineCore)
            {
                bool coreStateMachineRunResult;
                ServiceFabricAutopilotAgentState currentState;

                do
                {
                    currentState = this.agentStatus.GetCurrentState();

                    if (currentState < ServiceFabricAutopilotAgentState.Started)
                    {
                        return;
                    }

                    coreStateMachineRunResult = this.RunStateMachineCore(currentState);

                    TextLogger.LogInfo("RunStateMachine: RunStateMachineCore completed with result {0}. State prior to the run : {1}.", coreStateMachineRunResult, currentState);

                    if (!coreStateMachineRunResult)
                    {
                        lock (agentStatus.StatusLock)
                        {
                            this.agentStatus.LastFailedDeploymentAction = DateTime.UtcNow;
                        }
                    }
                    else
                    {
                        lock (agentStatus.StatusLock)
                        {
                            this.agentStatus.LastFailedDeploymentAction = DateTime.MinValue;
                        }
                    }

                    currentState = this.agentStatus.GetCurrentState();
                }
                while (currentState > ServiceFabricAutopilotAgentState.Started && currentState < ServiceFabricAutopilotAgentState.NodeStarted && coreStateMachineRunResult);
            }
        }

        private bool RunStateMachineCore(ServiceFabricAutopilotAgentState currentState)
        {
            TextLogger.LogInfo("Starting RunStateMachineCore. Current state : {0}.", currentState);

            switch (currentState)
            {
                case ServiceFabricAutopilotAgentState.ManagingNetworkConfiguration:
                    {
                        if (!this.localNodeManager.ManageNetworkConfiguration())
                        {
                            return false;
                        }

                        ServiceFabricAutopilotAgentState nextState;
                        if (!this.ComputeNextStateFromManagingNetworkConfigurationState(out nextState))
                        {
                            return false;
                        }

                        this.agentStatus.SetNextState(nextState);

                        break;
                    }

                case ServiceFabricAutopilotAgentState.InstallingProduct:
                    {
                        if (!this.localNodeManager.InstallProduct())
                        {
                            return false;
                        }                       

                        // For now, we assume the topology is static.     
                        // TODO: when adding dynamic topology support, change state machine transition here.                   
                        this.agentStatus.SetNextState(ServiceFabricAutopilotAgentState.DiscoveringTopology);

                        break;
                    }

                case ServiceFabricAutopilotAgentState.DiscoveringTopology:
                    {
                        ClusterTopology expectedTopology;
                        if (!this.localNodeManager.DiscoverTopology(out expectedTopology))
                        {
                            return false;
                        }

                        lock (this.agentStatus.StatusLock)
                        {
                            this.agentStatus.ExpectedTopology = expectedTopology;

                            this.agentStatus.ExpectedTopologyRunning = false;
                        }

                        this.agentStatus.SetNextState(ServiceFabricAutopilotAgentState.ConfiguringNode);

                        break;
                    }

                case ServiceFabricAutopilotAgentState.ConfiguringNode:
                    {
                        ClusterTopology topologyToDeploy = this.agentStatus.GetTopologyToDeploy();                      

                        if (!this.localNodeManager.ConfigureLocalNode(topologyToDeploy))
                        {
                            return false;
                        }

                        lock (this.agentStatus.StatusLock)
                        {
                            this.agentStatus.DeployedTopology = topologyToDeploy;

                            this.agentStatus.ExpectedTopologyRunning = !this.agentStatus.DeployedTopology.IsClusterTopologyChanged(this.agentStatus.ExpectedTopology);
                        }

                        this.agentStatus.SetNextState(ServiceFabricAutopilotAgentState.StartingNode);

                        break;
                    }

                case ServiceFabricAutopilotAgentState.StartingNode:
                    {
                        if (!this.localNodeManager.StartLocalNode())
                        {
                            return false;
                        }

                        this.agentStatus.SetNextState(ServiceFabricAutopilotAgentState.NodeStarted);

                        break;
                    }

                case ServiceFabricAutopilotAgentState.NodeStarted:
                    {
                        // TODO: this only applies when DynamicTopologyUpdateMode == None or OnNodeConfigurationOnly.
                        // When we want to update topology whenever topology changes, this needs to change.
                        lock (this.agentStatus.StatusLock)
                        {
                            this.agentStatus.ExpectedTopologyRunning = true;
                            this.agentStatus.ExpectedTopology = ClusterTopology.GetClusterTopology();
                            this.agentStatus.DeployedTopology = this.agentStatus.ExpectedTopology;
                        }
                        
                        break;
                    }
            }

            return true;
        }

        private bool ComputeNextStateFromManagingNetworkConfigurationState(out ServiceFabricAutopilotAgentState nextState)
        {
            nextState = ServiceFabricAutopilotAgentState.None;

            try
            {
                bool serviceFabricNodeConfigurationCompleted = Utilities.IsWindowsFabricNodeConfigurationCompleted(false);

                TextLogger.LogInfo("Service Fabric node configuration completed locally :  {0}.", serviceFabricNodeConfigurationCompleted);

                if (!serviceFabricNodeConfigurationCompleted)
                {
                    nextState = ServiceFabricAutopilotAgentState.InstallingProduct;
                }
                else
                {
                    ServiceController fabricHostService = this.localNodeManager.GetInstalledService(Constants.FabricHostServiceName);

                    if (fabricHostService == null)
                    {
                        // TODO: consider assert instead of an error message
                        TextLogger.LogError("Service Fabric node configuration has completed locally while Fabric host service does not exist.");

                        return false;
                    }

                    if (fabricHostService.Status != ServiceControllerStatus.Running)
                    {
                        nextState = ServiceFabricAutopilotAgentState.StartingNode;
                    }
                    else
                    {
                        nextState = ServiceFabricAutopilotAgentState.NodeStarted;
                    }
                }

                return true;
            }
            catch (Exception e)
            {
                TextLogger.LogError("Failed to compute next state from {0} state : {1}", ServiceFabricAutopilotAgentState.ManagingNetworkConfiguration, e);             

                return false;
            }            
        }

        private void RegisterConsoleCancelEventNotifications()
        {
            Console.CancelKeyPress += new ConsoleCancelEventHandler(ConsoleCancelEventHanlder);
        }

        private void Exit(bool succeeded)
        {
            TextLogger.Flush();

            this.exitCallback(succeeded);
        }
    }
}