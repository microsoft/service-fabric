// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace HttpApplicationGateway;
using namespace HttpCommon;

bool ServiceEndpointsList::TryParse(wstring const &endpointsString, __out ServiceEndpointsList &endpointsList)
{
    auto error = JsonHelper::Deserialize(endpointsList, endpointsString);
    if (!error.IsSuccess())
    {
        return false;
    }

    return true; 
}

//
// Search and return the endpoint based on the parameters:
// 1. SecureOnlyMode: Reverse proxy config parameter.  Applies only when reverse proxy is listening on HTTPS.
// 2. gatewayProtocol: Reverse proxy protocol, HTTP/HTTPS. 
// If reverse proxy is unsecure (HTTP), only look for listenerName in HTTP endpoints. 
//    Deny any HTTP reverse proxy -> HTTPS backend service communication.
// If Reverse Proxy is HTTPS:
//    SecureOnlyMode = true	    ListenerName not specified	    Forward to first HTTPS endpoint.If no HTTPS endpoint available, fail the request.
//    SecureOnlyMode = true	    ListenerName = HTTP endpoint	Deny.
//    SecureOnlyMode = false	ListenerName not specified	    Forward to first endpoint.It could be HTTP / HTTPS.
//    SecureOnlyMode = false	ListenerName = HTTP endpoint	Allow.
//    SecureOnlyMode = false	ListenerName = HTTPS endpoint	Allow.
// 3. listenerName: Query param value passed by the client.
//
bool ServiceEndpointsList::TryGetEndpoint(__in bool isGatewaySecureOnlyMode, __in ServiceModel::ProtocolType::Enum const& gatewayProtocol, __in wstring const &listenerName, __out wstring & endpoint) const
{
    if (endPointsMap_.size() < 1)
    {
        return false;
    }

    auto result = std::find_if(endPointsMap_.begin(), endPointsMap_.end(), [isGatewaySecureOnlyMode, gatewayProtocol, listenerName](const std::pair<std::wstring, std::wstring> & t) -> bool
    {
        // If gatewayProtocol is HTTP
        if (gatewayProtocol == ServiceModel::ProtocolType::Enum::Http)
        {
            // If listener name is empty, get first HTTP endpoint
            // If listener name specified, get first HTTP endpoint that matches the provided listenerName.
            return HttpUtil::IsHttpUri(t.second) && (listenerName.empty() || StringUtility::AreEqualCaseInsensitive(t.first, listenerName));
        }
        else if (gatewayProtocol == ServiceModel::ProtocolType::Enum::Https)
        {
            if (isGatewaySecureOnlyMode)
            {
                // SecureOnlyMode. So only choose HTTPS endpoints:
                // If listener name is empty, get first HTTPS endpoint
                // If listener name specified, get first HTTPS endpoint that matches the provided listenerName.
                return HttpUtil::IsHttpsUri(t.second) && (listenerName.empty() || StringUtility::AreEqualCaseInsensitive(t.first, listenerName));
            }

            // SecureOnlyMode = false:
            // If listener name is empty, get first endpoint (Http/Https)
            // If listener name specified, get first endpoint (Http/Https) that matches the provided listenerName.
            return (HttpUtil::IsHttpUri(t.second) || HttpUtil::IsHttpsUri(t.second)) 
                && (listenerName.empty() || StringUtility::AreEqualCaseInsensitive(t.first, listenerName));

        }

        return false;
    });

    // If not found, return false.
    if (result == std::end(endPointsMap_))
    {
        return false;
    }

    endpoint = result->second;
    return true;
}
