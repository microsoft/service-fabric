// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // Class used by HttpGateway to serialize POST body for cluster health policy and app health policy map
    struct ClusterHealthPolicies 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    private:
        DENY_COPY(ClusterHealthPolicies);
    public:
        ClusterHealthPolicies();

        ClusterHealthPolicies(ClusterHealthPolicies && other) = default;
        ClusterHealthPolicies & operator = (ClusterHealthPolicies && other) = default;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ApplicationHealthPolicyMap, ApplicationHealthPolicies)
            SERIALIZABLE_PROPERTY(Constants::ClusterHealthPolicy, ClusterHealthPolicyEntry)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_02(ApplicationHealthPolicies, ClusterHealthPolicyEntry);

        std::map<std::wstring, ApplicationHealthPolicy> ApplicationHealthPolicies;
        ClusterHealthPolicy ClusterHealthPolicyEntry;
    };
}

