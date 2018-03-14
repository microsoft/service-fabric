// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Management;
using namespace std;

DEFINE_SINGLETON_COMPONENT_CONFIG(ManagementConfig)

GlobalWString const ManagementConfig::ApplicationTypeMaxPercentUnhealthyApplicationsPrefix = make_global<wstring>(L"ApplicationTypeMaxPercentUnhealthyApplications-");

ServiceModel::ClusterHealthPolicy ManagementConfig::GetClusterHealthPolicy() const
{
    auto clusterHealthPolicy = ServiceModel::ClusterHealthPolicy(
        this->ConsiderWarningAsError,
        static_cast<BYTE>(this->MaxPercentUnhealthyNodes),
        static_cast<BYTE>(this->MaxPercentUnhealthyApplications));

    // Check if there are any per app type entries defined
    StringMap clusterHealthPolicyEntries;
    this->GetKeyValues(L"HealthManager/ClusterHealthPolicy", clusterHealthPolicyEntries);
    for (auto it = clusterHealthPolicyEntries.begin(); it != clusterHealthPolicyEntries.end(); ++it)
    {
        if (StringUtility::StartsWithCaseInsensitive<wstring>(it->first, *ApplicationTypeMaxPercentUnhealthyApplicationsPrefix))
        {
            // Get the application type name and the value
            wstring appTypeName = it->first;
            StringUtility::Replace<wstring>(appTypeName, *ApplicationTypeMaxPercentUnhealthyApplicationsPrefix, L"");
            
            int value;
            if (!StringUtility::TryFromWString<int>(it->second, value))
            {
                Assert::CodingError("Cluster manifest validation error: {0}: Error parsing app type MaxPercentUnhealthyApplication {1} as int", it->first, it->second);
            }

            auto error = clusterHealthPolicy.AddApplicationTypeHealthPolicy(move(appTypeName), static_cast<BYTE>(value));
            ASSERT_IFNOT(error.IsSuccess(), "Cluster manifest validation error: {0}: Error parsing app type MaxPercentUnhealthyApplication {1}", it->first, it->second);
        }
    }

    return clusterHealthPolicy;
}

ServiceModel::ClusterUpgradeHealthPolicy ManagementConfig::GetClusterUpgradeHealthPolicy() const
{
    return ServiceModel::ClusterUpgradeHealthPolicy(
        static_cast<BYTE>(this->MaxPercentDeltaUnhealthyNodes),
        static_cast<BYTE>(this->MaxPercentUpgradeDomainDeltaUnhealthyNodes));
}

void ManagementConfig::GetSections(StringCollection & sectionNames) const
{
    this->FabricGetConfigStore().GetSections(sectionNames);
}

void ManagementConfig::GetKeyValues(std::wstring const & section, StringMap & entries) const
{
    ComponentConfig::GetKeyValues(section, entries);
}
