// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Autopilot
{
    using System;
    using System.Globalization;

    internal class ServiceFabricAutopilotAgentStatus
    {
        public ServiceFabricAutopilotAgentStatus()
        {
            this.StatusLock = new object();

            this.State = ServiceFabricAutopilotAgentState.Started;

            this.DeployedTopology = null;

            this.ExpectedTopology = null;

            this.ExpectedTopologyRunning = false;

            this.LastFailedDeploymentAction = DateTime.MinValue;
        }

        public object StatusLock { get; private set; }

        public ServiceFabricAutopilotAgentState State { get; set; }

        // The topology that is currently running.
        public ClusterTopology DeployedTopology { get; set; }

        // The topology that is expected to be running.
        public ClusterTopology ExpectedTopology { get; set; }

        // Whether the expected topology is running.
        public bool ExpectedTopologyRunning { get; set; }

        public DateTime LastFailedDeploymentAction { get; set; }

        public void InitializeAgentStatus()
        {
            lock (this.StatusLock)
            {
                this.State = ServiceFabricAutopilotAgentState.ManagingNetworkConfiguration;

                this.DeployedTopology = null;

                // For now, we assume the topology is static.     
                // TODO: when adding dynamic topology support, the expected topology does not need to be discovered.
                this.ExpectedTopology = null;

                this.ExpectedTopologyRunning = false;

                this.LastFailedDeploymentAction = DateTime.MinValue;
            }
        }

        public ServiceFabricAutopilotAgentState GetCurrentState()
        {
            lock (this.StatusLock)
            {
                return this.State;
            }
        }

        public void SetNextState(ServiceFabricAutopilotAgentState nextState, bool toIgnoreStateTransitionWhenStopping = true)
        {
            lock (this.StatusLock)
            {
                if (toIgnoreStateTransitionWhenStopping && this.State < ServiceFabricAutopilotAgentState.Started)
                {
                    TextLogger.LogVerbose("Application is stopping. Ignore state transition request to {0}.", nextState);

                    return;
                }

                TextLogger.LogInfo("State transition: {0} -> {1}", this.State, nextState);

                this.State = nextState;
            }
        }

        public ClusterTopology GetTopologyToDeploy()
        {
            lock (this.StatusLock)
            {
                return this.ExpectedTopology;
            }
        }

        public override string ToString()
        {
            lock (this.StatusLock)
            {
                return string.Format(
                            CultureInfo.InvariantCulture,
                            "ServiceFabricAutopilotAgentStatus : [ State : {0}, ExpectedTopologyRunning : {1}, LastFailedDeploymentAction : {2} ]",
                            this.State,
                            this.ExpectedTopologyRunning,
                            this.LastFailedDeploymentAction);
            }
        }
    }

    // State of Service Fabric Autopilot Agent.
    // Note that the order of the values matters.
    internal enum ServiceFabricAutopilotAgentState
    {
        None,

        Stopped,

        Stopping,

        Started,

        ManagingNetworkConfiguration,

        InstallingProduct,

        DiscoveringTopology,

        ConfiguringNode,

        StartingNode,

        NodeStarted
    }
}