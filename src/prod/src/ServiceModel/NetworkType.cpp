// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

void NetworkType::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    wstring result = EnumToString(val);
    w << result;
}

std::wstring NetworkType::EnumToString(ServiceModel::NetworkType::Enum val)
{
    wstring result;
    if ((val & NetworkType::Open) == NetworkType::Open)
    {
        if (result.empty())
        {
            result = NetworkType::OpenStr;
        }
        else
        {
            result = wformatString("{0}{1}{2}", result, NetworkType::NetworkSeparator, NetworkType::OpenStr);
        }
    }

    if ((val & NetworkType::Other) == NetworkType::Other)
    {
        if (result.empty())
        {
            result = NetworkType::OtherStr;
        }
        else
        {
            result = wformatString("{0}{1}{2}", result, NetworkType::NetworkSeparator, NetworkType::OtherStr);
        }
    }
        
    if ((val & NetworkType::Isolated) == NetworkType::Isolated)
    {
        if (result.empty())
        {
            result = NetworkType::IsolatedStr;
        }
        else
        {
            result = wformatString("{0}{1}{2}", result, NetworkType::NetworkSeparator, NetworkType::IsolatedStr);
        }
    }

    return result;
}

ErrorCode NetworkType::FromString(wstring const & value, __out Enum & val)
{
    vector<wstring> enumValues;
    StringUtility::Split<wstring>(value, enumValues, NetworkType::NetworkSeparator, true);
    bool updated = false;

    for (int i = 0; i < enumValues.size(); i++)
    {
        if (enumValues[i] == NetworkType::OpenStr || enumValues[i] == L"default")
        {
            val = val | NetworkType::Open;
            updated = true;
        }

        if (enumValues[i] == NetworkType::OtherStr)
        {
            val = val | NetworkType::Other;
            updated = true;
        }

        if (enumValues[i] == NetworkType::IsolatedStr)
        {
            val = val | NetworkType::Isolated;
            updated = true;
        }
    }

    if (updated)
    {
        return ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        return ErrorCode::FromHResult(E_INVALIDARG);
    }
}

FABRIC_CONTAINER_NETWORK_TYPE NetworkType::ToPublicApi(Enum networkType)
{
    FABRIC_CONTAINER_NETWORK_TYPE containerNetworkType = FABRIC_CONTAINER_NETWORK_TYPE_OTHER;

    if ((networkType & NetworkType::Open) == NetworkType::Open)
    {
        containerNetworkType = (FABRIC_CONTAINER_NETWORK_TYPE)(containerNetworkType | FABRIC_CONTAINER_NETWORK_TYPE_OPEN);
    }

    if ((networkType & NetworkType::Isolated) == NetworkType::Isolated)
    {
        containerNetworkType = (FABRIC_CONTAINER_NETWORK_TYPE)(containerNetworkType | FABRIC_CONTAINER_NETWORK_TYPE_ISOLATED);
    }
    
    return containerNetworkType;
}

bool NetworkType::IsMultiNetwork(wstring const & networkingMode)
{
    return StringUtility::ContainsCaseInsensitive(networkingMode, NetworkType::NetworkSeparator);
}