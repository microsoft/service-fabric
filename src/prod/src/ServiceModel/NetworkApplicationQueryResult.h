// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class NetworkApplicationQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        NetworkApplicationQueryResult();

        NetworkApplicationQueryResult(std::wstring const & applicationName);

        NetworkApplicationQueryResult(NetworkApplicationQueryResult && other);

        NetworkApplicationQueryResult const & operator = (NetworkApplicationQueryResult && other);        

        ~NetworkApplicationQueryResult() = default;

        Common::ErrorCode FromPublicApi(__in FABRIC_NETWORK_APPLICATION_QUERY_RESULT_ITEM const & networkApplicationQueryResult);

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_NETWORK_APPLICATION_QUERY_RESULT_ITEM & publicNetworkApplicationQueryResult) const;

        __declspec(property(get = get_ApplicationName)) std::wstring const& ApplicationName;
        std::wstring const& get_ApplicationName() const { return applicationName_; }        

        FABRIC_FIELDS_01(applicationName_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
          SERIALIZABLE_PROPERTY(Constants::ApplicationName, applicationName_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring applicationName_;
    };

    // Used to serialize results in REST
    QUERY_JSON_LIST(NetworkApplicationList, NetworkApplicationQueryResult)
}
