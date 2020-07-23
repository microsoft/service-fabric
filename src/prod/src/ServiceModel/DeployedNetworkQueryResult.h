// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeployedNetworkQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        DeployedNetworkQueryResult();

        DeployedNetworkQueryResult(std::wstring const & networkName);

        DeployedNetworkQueryResult(DeployedNetworkQueryResult && other);

        DeployedNetworkQueryResult const & operator = (DeployedNetworkQueryResult && other);        

        ~DeployedNetworkQueryResult() = default;

        std::wstring ToString() const;

        Common::ErrorCode FromPublicApi(__in FABRIC_DEPLOYED_NETWORK_QUERY_RESULT_ITEM const & deployedNetworkQueryResult);

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_DEPLOYED_NETWORK_QUERY_RESULT_ITEM & publicDeployedNetworkQueryResult) const;

        __declspec(property(get = get_NetworkName)) std::wstring const & NetworkName;
        std::wstring const & get_NetworkName() const { return networkName_; }

        FABRIC_FIELDS_01(networkName_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::NetworkName, networkName_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring networkName_;
    };

    // Used to serialize results in REST
    QUERY_JSON_LIST(DeployedNetworkList, DeployedNetworkQueryResult)
}
