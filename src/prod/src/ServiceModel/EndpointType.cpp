// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

void EndpointType::WriteToTextWriter(__in TextWriter & w, EndpointType::Enum const & val)
{
    switch (val)
    {
    case EndpointType::Input:
        w << L"Input";
        return;
    case EndpointType::Internal:
        w << L"Internal";
        return;
    default:
        Common::Assert::CodingError("Unknown EndpointType value {0}", static_cast<int>(val));
    }
}

EndpointType::Enum EndpointType::GetEndpointType(std::wstring const & val)
{
    if (StringUtility::AreEqualCaseInsensitive(val, L"Input"))
    {
        return EndpointType::Input;
    }
    else if (StringUtility::AreEqualCaseInsensitive(val, L"Internal"))
    {
        return EndpointType::Internal;
    }
    else
    {
        Common::Assert::CodingError("Unknown EndpointType value {0}", val);
    }
}

wstring EndpointType::EnumToString(EndpointType::Enum value)
{
	switch (value)
	{
	case EndpointType::Input:
		return L"Input";
	case EndpointType::Internal:
		return L"Internal";
	default:
		Common::Assert::CodingError("Unknown EndpointType value {0}", value);
	}

}
