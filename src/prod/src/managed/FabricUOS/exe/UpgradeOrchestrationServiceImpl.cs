// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeOrchestration.Service
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    internal sealed class UpgradeOrchestrationServiceImpl : IUpgradeOrchestrationService
    {
        private const string TraceType = "UpgradeOrchestrationServiceImpl";

        private Dictionary<string, ActionHandler> actionMapping;

        public UpgradeOrchestrationServiceImpl(UpgradeOrchestrationMessageProcessor messageProcessor)
        {
            this.MessageProcessor = messageProcessor;
            this.LoadActionMapping();
        }

        private delegate Task<string> ActionHandler(string inputBlob, TimeSpan timeout, CancellationToken cancellationToken);

        internal UpgradeOrchestrationMessageProcessor MessageProcessor
        {
            get;
            private set;
        }

        public async Task StartClusterConfigurationUpgrade(
            ConfigurationUpgradeDescription configurationUpgradeDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Enter StartClusterConfigurationUpgrade.");
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(
                TraceType,
                "startUpgradeDescription: {0}",
                configurationUpgradeDescription.ClusterConfiguration);

            try
            {
                await this.MessageProcessor.ProcessStartClusterConfigurationUpgradeAsync(configurationUpgradeDescription, timeout, cancellationToken);
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "{0} - Exception occurred", e);
                throw;
            }

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exit StartClusterConfigurationUpgrade.");
        }

        public async Task<FabricOrchestrationUpgradeProgress> GetClusterConfigurationUpgradeStatus(TimeSpan timeout, CancellationToken cancellationToken)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Enter GetClusterConfigurationUpgradeStatus.");
            FabricOrchestrationUpgradeProgress upgradeProgress = null;

            try
            {
                upgradeProgress = await this.MessageProcessor.ProcessGetClusterConfigurationUpgradeProgressAsync(timeout, cancellationToken);
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "{0} - Exception occurred", e);
                throw;
            }

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exit GetClusterConfigurationUpgradeStatus.");
            return upgradeProgress;
        }

        public async Task<string> GetClusterConfiguration(string apiVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Enter GetClusterConfiguration.");
            string clusterConfig = null;

            try
            {
                clusterConfig = await this.MessageProcessor.ProcessGetClusterConfigurationAsync(apiVersion, timeout, cancellationToken);
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "{0} - Exception occurred", e);
                throw;
            }

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exit GetClusterConfiguration.");
            return clusterConfig;
        }

        // TODO: Not implemented yet
        public async Task GetUpgradesPendingApproval(TimeSpan timeout, CancellationToken cancellationToken)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Enter GetUpgradesPendingApproval.");
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exit GetUpgradesPendingApproval.");
            await Task.Delay(2);
        }

        // TODO: Not implemented yet
        public async Task StartApprovedUpgrades(TimeSpan timeout, CancellationToken cancellationToken)
        {
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Enter StartApprovedUpgrades.");
            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exit StartApprovedUpgrades.");
            await Task.Delay(2);
        }

        public async Task<string> CallSystemService(string action, string inputBlob, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string outputBlob = null;

            try
            {
                UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Enter CallSystemService for {0}.", action);

                if (this.actionMapping.ContainsKey(action))
                {
                    outputBlob = await this.actionMapping[action](inputBlob, timeout, cancellationToken).ConfigureAwait(false);
                }
                else
                {
                    throw UpgradeOrchestrationMessageProcessor.ConvertToComException(new NotImplementedException(action + "is not implemented"));
                }
            }
            catch (Exception e)
            {
                UpgradeOrchestrationTrace.TraceSource.WriteWarning(TraceType, "CallSystemService error: {0}", e);
                throw;
            }

            UpgradeOrchestrationTrace.TraceSource.WriteInfo(TraceType, "Exit CallSystemService for {0}.", action);
            return outputBlob;
        }

        internal void LoadActionMapping()
        {
            // The key is the action name defined in XXXTcpMessage.h
            this.actionMapping = new Dictionary<string, ActionHandler>(StringComparer.InvariantCultureIgnoreCase)
            {
                { "GetUpgradeOrchestrationServiceStateAction", this.MessageProcessor.ProcessGetUpgradeOrchestrationServiceStateAsync },
                { "SetUpgradeOrchestrationServiceStateAction", this.MessageProcessor.ProcessSetUpgradeOrchestrationServiceStateAsync },
            };
        }
    }
}