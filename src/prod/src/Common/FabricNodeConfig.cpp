// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

FabricNodeConfig::NodeFaultDomainIdCollection FabricNodeConfig::NodeFaultDomainIdCollection::Parse(StringMap const & entries)
{
    FabricNodeConfig::NodeFaultDomainIdCollection result;

    for (auto it = entries.begin(); it != entries.end(); ++it)
    {
        if (it->first == L"NodeFaultDomainId" && !it->second.empty())
        {
            Uri uri;
            bool valid = Uri::TryParse(it->second, uri);
            ASSERT_IF(!valid, "Invalid fault domain URI configured: {0}", it->second);
            ASSERT_IF(uri.Segments.empty(), "Fault domain URI doesn't have path {0}", it->second);
            ASSERT_IFNOT(StringUtility::AreEqualCaseInsensitive(uri.Scheme, L"fd"), "Fault domain URI should have scheme ""fd""");

            result.push_back(uri);
            break;
        }
    }

    return result;
}

void FabricNodeConfig::NodeFaultDomainIdCollection::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write(*this);
}
