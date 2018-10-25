//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ContainerGroupConfig
        : public Common::IFabricJsonSerializable
        {
        public:
            ContainerGroupConfig()
            : RemoveServiceFabricRuntimeAccess(ManagementConfig::GetConfig().RemoveServiceFabricRuntimeAccess)
            , ReplicaRestartWaitDuration(ManagementConfig::GetConfig().CG_ReplicaRestartWaitDurationSeconds)
            , QuorumLossWaitDuration(ManagementConfig::GetConfig().CG_QuorumLossWaitDurationSeconds)
            , AzureFilePluginName(ManagementConfig::GetConfig().AzureFileVolumePluginName)
            , ContainersRetentionCount(ManagementConfig::GetConfig().CG_ContainersRetentionCount)
            , IsolationLevel(ManagementConfig::GetConfig().CG_IsolationLevel)
            {
            }

            virtual ~ContainerGroupConfig() = default;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::RemoveServiceFabricRuntimeAccess, RemoveServiceFabricRuntimeAccess)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ReplicaRestartWaitDurationSeconds, ReplicaRestartWaitDuration)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::QuorumLossWaitDurationSeconds, QuorumLossWaitDuration)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::AzureFilePluginName, AzureFilePluginName)
                SERIALIZABLE_PROPERTY(L"ContainersRetentionCount", ContainersRetentionCount)
                SERIALIZABLE_PROPERTY(L"IsolationLevel", IsolationLevel)
            END_JSON_SERIALIZABLE_PROPERTIES()

            bool RemoveServiceFabricRuntimeAccess;
            int ReplicaRestartWaitDuration;
            int QuorumLossWaitDuration;
            std::wstring AzureFilePluginName;
            std::wstring ContainersRetentionCount;
            std::wstring IsolationLevel;
        };
    }
}
