// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpApplicationGateway
{
    //
    // TODO: This should be moved to servicemodel\
    //
    class ServiceEndpointsList : public Common::IFabricJsonSerializable
    {
        DENY_COPY(ServiceEndpointsList)
    public:
        ServiceEndpointsList() {}

        static bool TryParse(std::wstring const &endpointsString, __out ServiceEndpointsList &endpointsList);

        //
        // Obtain the best matching endpoint based on the input parameters passed.
        //
        bool TryGetEndpoint(
            __in bool isGatewaySecureOnlyMode,
            __in ServiceModel::ProtocolType::Enum const& gatewayProtocol,
            __in std::wstring const &listenerName,
            __out std::wstring & endpoint) const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"Endpoints", endPointsMap_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::map<std::wstring, std::wstring> endPointsMap_;
    };
}
