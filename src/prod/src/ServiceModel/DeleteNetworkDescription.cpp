// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DeleteNetworkDescription::DeleteNetworkDescription()
    : networkName_()
{
}

DeleteNetworkDescription::DeleteNetworkDescription(wstring const & networkName)
    : networkName_(networkName)
{
}

DeleteNetworkDescription::DeleteNetworkDescription(DeleteNetworkDescription const & otherDeleteNetworkDescription)
    : networkName_(otherDeleteNetworkDescription.NetworkName)
{
}

Common::ErrorCode DeleteNetworkDescription::FromPublicApi(__in FABRIC_DELETE_NETWORK_DESCRIPTION const & deleteDescription)
{
    auto error = StringUtility::LpcwstrToWstring2(deleteDescription.NetworkName, false, networkName_);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}