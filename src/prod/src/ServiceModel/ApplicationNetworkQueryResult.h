// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ApplicationNetworkQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ApplicationNetworkQueryResult();
        
        ApplicationNetworkQueryResult(std::wstring const & networkName);

        ApplicationNetworkQueryResult(ApplicationNetworkQueryResult && other);

        ApplicationNetworkQueryResult const & operator = (ApplicationNetworkQueryResult && other);

        ~ApplicationNetworkQueryResult() = default;

        Common::ErrorCode FromPublicApi(__in FABRIC_APPLICATION_NETWORK_QUERY_RESULT_ITEM const & applicationNetworkQueryResult);

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_APPLICATION_NETWORK_QUERY_RESULT_ITEM & publicApplicationNetworkQueryResult) const;

        __declspec(property(get = get_NetworkName)) std::wstring const& NetworkName;
        std::wstring const& get_NetworkName() const { return networkName_; }

        FABRIC_FIELDS_01(networkName_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
          SERIALIZABLE_PROPERTY(Constants::NetworkNameCamelCase, networkName_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring networkName_;
    };

    // Used to serialize results in REST
    QUERY_JSON_LIST(ApplicationNetworkList, ApplicationNetworkQueryResult)
}
