// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

void ProtocolType::WriteToTextWriter(__in TextWriter & w, ProtocolType::Enum const & val)
{
    switch (val)
    {
    case ProtocolType::Tcp:
        w << L"tcp";
        return;
    case ProtocolType::Http:
        w << L"http";
        return;
    case ProtocolType::Https:
        w << L"https";
        return;
    case ProtocolType::Udp:
        w << L"udp";
        return;
    default:
        Common::Assert::CodingError("Unknown ProtocolType value {0}", static_cast<int>(val));
    }
}

ProtocolType::Enum ProtocolType::GetProtocolType(std::wstring const & val)
{
if (StringUtility::AreEqualCaseInsensitive(val, L"tcp"))
    {
        return ProtocolType::Tcp;
    }
    else if (StringUtility::AreEqualCaseInsensitive(val, L"http"))
    {
        return ProtocolType::Http;
    }
    else if (StringUtility::AreEqualCaseInsensitive(val, L"https"))
    {
        return ProtocolType::Https;
    }
    else if (StringUtility::AreEqualCaseInsensitive(val, L"udp"))
    {
        return ProtocolType::Udp;
    }
    else
    {
        Common::Assert::CodingError("Unknown ProtocolType value {0}", val);
    }
}

wstring ProtocolType::EnumToString(ProtocolType::Enum val)
{
	switch (val)
	{
	case ProtocolType::Enum::Tcp:
		return L"tcp";
	case ProtocolType::Enum::Http:
		return L"http";
	case ProtocolType::Enum::Https:
		return L"https";
    case ProtocolType::Enum::Udp:
        return L"udp";
	default:
		Common::Assert::CodingError("Unknown ProtocolType value {0}", val);
	}
}
