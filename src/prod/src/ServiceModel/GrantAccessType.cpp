// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

void GrantAccessType::WriteToTextWriter(__in TextWriter & w, GrantAccessType::Enum const & val)
{
    switch (val)
    {
    case GrantAccessType::Read:
        w << L"Read";
        return;
    case GrantAccessType::Change:
        w << L"Change";
        return;
    case GrantAccessType::Full:
        w << L"Full";
        return;
    default:
        Common::Assert::CodingError("Unknown GrantAccessType value {0}", static_cast<int>(val));
    }
}

GrantAccessType::Enum GrantAccessType::GetGrantAccessType(std::wstring const & val)
{
    if (StringUtility::AreEqualCaseInsensitive(val, L"Read"))
    {
        return GrantAccessType::Read;
    }
    else if (StringUtility::AreEqualCaseInsensitive(val, L"Change"))
    {
        return GrantAccessType::Change;
    }
    else if (StringUtility::AreEqualCaseInsensitive(val, L"Full"))
    {
        return GrantAccessType::Full;
    }
    else
    {
        Common::Assert::CodingError("Unknown GrantAccessType value {0}", val);
    }
}

wstring GrantAccessType::ToString(GrantAccessType::Enum val)
{
	switch (val)
	{
	case GrantAccessType::Read:
		return L"Read";
	case GrantAccessType::Change:
		return L"Change";
	case GrantAccessType::Full:
		return L"Full";
	default:
		return L"Read";
	}
}
