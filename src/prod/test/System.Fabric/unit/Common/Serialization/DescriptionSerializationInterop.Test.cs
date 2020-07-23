// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Common.Serialization
{
    using Newtonsoft.Json;
    using System.Fabric.Description;
    using System.Fabric.Health;

    /// Json properties of this class should match json properties of native class defined in file:
    /// %SDXROOT%\services\winfab\prod\test\System.Fabric\Common\NativeAndManagedSerializationInteropTest\DescriptionSerializationInteropTest.h
    internal class DescriptionSerializationInteropTest
    {
        [JsonIgnore]
        Random random = new Random((int)DateTime.Now.Ticks);

        ServiceTypeDescription FABRIC_SERVICE_TYPE_DESCRIPTION_ { get; set; }

        ServicePlacementPolicyDescription FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION_ { get; set; }

        DescriptionExtension FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION_ { get; set; }

        ApplicationDescription FABRIC_APPLICATION_DESCRIPTION_ { get; set; }

        PartitionSchemeDescription FABRIC_PARTITION_SCHEME_ { get; set; }

        ServiceDescription FABRIC_SERVICE_DESCRIPTION_ { get; set; }

        ServiceGroupTypeDescription FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_ { get; set; }

        ServiceGroupTypeMemberDescription FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION_ { get; set; }

        ServiceGroupMemberDescription FABRIC_SERVICE_GROUP_MEMBER_DESCRIPTION_ { get; set; }

        ServiceLoadMetricDescription FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION_ { get; set; }

        FabricUpgradeDescription FABRIC_UPGRADE_DESCRIPTION_ { get; set; }

        FabricUpgradeUpdateDescription FABRIC_UPGRADE_UPDATE_DESCRIPTION_ { get; set; }

        ApplicationUpgradeDescription FABRIC_APPLICATION_UPGRADE_DESCRIPTION_ { get; set; }

        ApplicationUpgradeUpdateDescription FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION_ { get; set; }

        RollingUpgradeMonitoringPolicy FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_ { get; set; }

        ApplicationHealthPolicyMap FABRIC_APPLICATION_HEALTH_POLICY_MAP_ { get; set; }

        public void Init()
        {
            var random = new Random((int)DateTime.Now.Ticks);

            FABRIC_SERVICE_TYPE_DESCRIPTION_ = this.random.CreateRandom<ServiceTypeDescription>();
            FABRIC_SERVICE_TYPE_DESCRIPTION_.Extensions.Add("extensionKey1", "extensionValue1");
            FABRIC_SERVICE_TYPE_DESCRIPTION_.Extensions.Add("extensionKey2", "extensionValue2");
            FABRIC_SERVICE_TYPE_DESCRIPTION_.LoadMetrics.Add(random.CreateRandom<ServiceLoadMetricDescription>());
            FABRIC_APPLICATION_DESCRIPTION_ = new ApplicationDescription(
                this.random.CreateRandom<Uri>(),
                this.random.CreateRandom<string>(),
                this.random.CreateRandom<string>());


            FABRIC_APPLICATION_DESCRIPTION_.ApplicationParameters.Add("testParamName", "testValue");
            FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION_ = this.random.CreateRandom<ServiceLoadMetricDescription>();
            
            FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION_ = this.random.CreateRandom<ServiceGroupTypeMemberDescription>();
            FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION_.LoadMetrics.Add(this.random.CreateRandom<ServiceLoadMetricDescription>());
            FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_ = this.random.CreateRandom<ServiceGroupTypeDescription>();
            FABRIC_SERVICE_GROUP_MEMBER_DESCRIPTION_ = this.random.CreateRandom<ServiceGroupMemberDescription>();
            FABRIC_SERVICE_GROUP_MEMBER_DESCRIPTION_.InitializationData = new byte[] { 10, 20, 30 };
            FABRIC_PARTITION_SCHEME_ = this.random.CreateRandom<PartitionSchemeDescription>();
            FABRIC_SERVICE_DESCRIPTION_ = this.random.CreateRandom<ServiceDescription>();
            var sf = FABRIC_SERVICE_DESCRIPTION_ as StatefulServiceDescription;

            // If service is not persisted and stateful set properties accordingly.
            if (sf != null)
            {
                // Stateful
                if (sf.HasPersistedState == false)
                {
                    // set these values to null if service is not persisted
                    sf.QuorumLossWaitDuration = sf.ReplicaRestartWaitDuration = sf.StandByReplicaKeepDuration = null;
                }
                else
                {
                    // Persisted
                    // set these values so that sub-seconds part is zero because they will be serialized as seconds and then it can be compared correctly.
                    sf.QuorumLossWaitDuration = TimeSpan.FromTicks(this.random.Next() * TimeSpan.TicksPerSecond);
                    sf.ReplicaRestartWaitDuration = TimeSpan.FromTicks(this.random.Next() * TimeSpan.TicksPerSecond);
                    sf.StandByReplicaKeepDuration = TimeSpan.FromTicks(this.random.Next() * TimeSpan.TicksPerSecond);
                }
            }

            FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION_ = this.random.CreateRandom<ServicePlacementPolicyDescription>();
            FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_ = this.random.CreateRandom<RollingUpgradeMonitoringPolicy>();

            var appUpgradePolicy = this.random.CreateRandom<MonitoredRollingApplicationUpgradePolicyDescription>();
            appUpgradePolicy.UpgradeMode = RollingUpgradeMode.Monitored;

            FABRIC_APPLICATION_UPGRADE_DESCRIPTION_ = this.random.CreateRandom<ApplicationUpgradeDescription>();
            FABRIC_APPLICATION_UPGRADE_DESCRIPTION_.UpgradePolicyDescription = appUpgradePolicy;

            FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION_ = this.random.CreateRandom<ApplicationUpgradeUpdateDescription>();
            FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION_.UpgradeMode = RollingUpgradeMode.Monitored;

            var fabricUpgradePolicy = this.random.CreateRandom<MonitoredRollingFabricUpgradePolicyDescription>();
            fabricUpgradePolicy.UpgradeMode = RollingUpgradeMode.Monitored;
            FABRIC_UPGRADE_DESCRIPTION_ = this.random.CreateRandom<FabricUpgradeDescription>();
            FABRIC_UPGRADE_DESCRIPTION_.UpgradePolicyDescription = fabricUpgradePolicy;

            FABRIC_UPGRADE_UPDATE_DESCRIPTION_ = this.random.CreateRandom<FabricUpgradeUpdateDescription>();
            FABRIC_UPGRADE_UPDATE_DESCRIPTION_.UpgradeMode = RollingUpgradeMode.Monitored;

            FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION_ = this.random.CreateRandom<DescriptionExtension>();
            FABRIC_APPLICATION_HEALTH_POLICY_MAP_ = this.random.CreateRandom<ApplicationHealthPolicyMap>();
        }
    }
}