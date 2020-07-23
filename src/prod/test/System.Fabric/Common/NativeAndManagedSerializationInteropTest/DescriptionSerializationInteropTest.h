// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"
#include "JsonSerializableTestBase.h"

namespace NativeAndManagedSerializationInteropTest
{    
    /// Json properties of this class should match json properties of managed class defined in file:
    /// %SDXROOT%\services\winfab\prod\test\System.Fabric\unit\Common\Serialization\DescriptionSerializationInterop.Test.cs
    class DescriptionSerializationInteropTest : public JsonSerializableTestBase
    {
    public:
        DescriptionSerializationInteropTest()
        {
            // TODO: remove when 4586918 is fixed
            Common::CommonConfig::GetConfig().EnableApplicationTypeHealthEvaluation = true;
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"FABRIC_SERVICE_TYPE_DESCRIPTION_", FABRIC_SERVICE_TYPE_DESCRIPTION_)
            SERIALIZABLE_PROPERTY(L"FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION_", FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION_)
            SERIALIZABLE_PROPERTY(L"FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION_", FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION_)
            SERIALIZABLE_PROPERTY(L"FABRIC_PARTITION_SCHEME_", FABRIC_PARTITION_SCHEME_)
            SERIALIZABLE_PROPERTY(L"FABRIC_SERVICE_DESCRIPTION_", FABRIC_SERVICE_DESCRIPTION_)
            SERIALIZABLE_PROPERTY(L"FABRIC_APPLICATION_DESCRIPTION_", FABRIC_APPLICATION_DESCRIPTION_)
            SERIALIZABLE_PROPERTY(L"FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_", FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_)
            SERIALIZABLE_PROPERTY(L"FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION_", FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION_)
            SERIALIZABLE_PROPERTY(L"FABRIC_SERVICE_GROUP_MEMBER_DESCRIPTION_", FABRIC_SERVICE_GROUP_MEMBER_DESCRIPTION_)
            SERIALIZABLE_PROPERTY(L"FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION_", FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION_)
            SERIALIZABLE_PROPERTY(L"FABRIC_APPLICATION_UPGRADE_DESCRIPTION_", FABRIC_APPLICATION_UPGRADE_DESCRIPTION_)
            SERIALIZABLE_PROPERTY(L"FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION_", FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION_)
            SERIALIZABLE_PROPERTY(L"FABRIC_UPGRADE_DESCRIPTION_", FABRIC_UPGRADE_DESCRIPTION_)
            SERIALIZABLE_PROPERTY(L"FABRIC_UPGRADE_UPDATE_DESCRIPTION_", FABRIC_UPGRADE_UPDATE_DESCRIPTION_)
            SERIALIZABLE_PROPERTY(L"FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_", FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_)
            SERIALIZABLE_PROPERTY(L"FABRIC_APPLICATION_HEALTH_POLICY_MAP_", FABRIC_APPLICATION_HEALTH_POLICY_MAP_)
       END_JSON_SERIALIZABLE_PROPERTIES()

        ServiceModel::ApplicationDescriptionWrapper FABRIC_APPLICATION_DESCRIPTION_;
        ServiceModel::ApplicationUpgradeDescriptionWrapper FABRIC_APPLICATION_UPGRADE_DESCRIPTION_;
        Management::ClusterManager::ApplicationUpgradeUpdateDescription FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION_;
        ServiceModel::FabricUpgradeDescriptionWrapper FABRIC_UPGRADE_DESCRIPTION_;
        Management::ClusterManager::FabricUpgradeUpdateDescription FABRIC_UPGRADE_UPDATE_DESCRIPTION_;
        ServiceModel::PartitionDescription FABRIC_PARTITION_SCHEME_;
        ServiceModel::PartitionedServiceDescWrapper FABRIC_SERVICE_DESCRIPTION_;

        ServiceModel::RollingUpgradeMonitoringPolicy FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_;
        ServiceModel::ServiceLoadMetricDescription FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION_;
        
        ServiceModel::ServiceGroupMemberDescriptionAdaptor FABRIC_SERVICE_GROUP_MEMBER_DESCRIPTION_;
        ServiceModel::ServiceGroupTypeDescription FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_;
        ServiceModel::ServiceGroupTypeMemberDescription FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION_;
        ServiceModel::ServicePlacementPolicyDescription FABRIC_SERVICE_PLACEMENT_POLICY_DESCRIPTION_;
        ServiceModel::ServiceTypeDescription FABRIC_SERVICE_TYPE_DESCRIPTION_;
        ServiceModel::DescriptionExtension FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION_;
        ServiceModel::ClusterHealthQueryDescription FABRIC_CLUSTER_HEALTH_QUERY_DESCRIPTION_;
        ServiceModel::ApplicationHealthPolicyMap FABRIC_APPLICATION_HEALTH_POLICY_MAP_;
    };
}
