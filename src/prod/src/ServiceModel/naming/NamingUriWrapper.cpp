// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

NamingUriWrapper::NamingUriWrapper(wstring const &uri)
    : uriString_(uri)
{
}

ErrorCode NamingUriWrapper::FromPublicApi(__in FABRIC_URI const & uriString)
{
    if (uriString == nullptr) { return ErrorCodeValue::InvalidArgument; }
    uriString_ = uriString;
    return ErrorCode::Success();
}

void NamingUriWrapper::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_URI & uriString) const
{
    uriString = heap.AddString(uriString_);
}
