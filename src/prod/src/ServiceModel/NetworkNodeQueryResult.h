// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class NetworkNodeQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        NetworkNodeQueryResult();

        NetworkNodeQueryResult(std::wstring const & nodeName);

        NetworkNodeQueryResult(NetworkNodeQueryResult && other);

        NetworkNodeQueryResult const & operator = (NetworkNodeQueryResult && other);

        ~NetworkNodeQueryResult() = default;

        Common::ErrorCode FromPublicApi(__in FABRIC_NETWORK_NODE_QUERY_RESULT_ITEM const & networkNodeQueryResult);

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_NETWORK_NODE_QUERY_RESULT_ITEM & publicNetworkNodeQueryResult) const;

        __declspec(property(get = get_NodeName)) std::wstring const& NodeName;
        std::wstring const& get_NodeName() const { return nodeName_; }

        FABRIC_FIELDS_01(nodeName_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
          SERIALIZABLE_PROPERTY(Constants::NodeNameCamelCase, nodeName_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring nodeName_;
    };

    // Used to serialize results in REST
    QUERY_JSON_LIST(NetworkNodeList, NetworkNodeQueryResult)
}
