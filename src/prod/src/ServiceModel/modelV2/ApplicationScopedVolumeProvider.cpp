// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel::ModelV2;

void ApplicationScopedVolumeProvider::WriteToTextWriter(__in TextWriter & w, Enum const & val)
{
    switch (val)
    {
    case ApplicationScopedVolumeProvider::Invalid:
        w << L"Invalid";
        return;
    case ApplicationScopedVolumeProvider::ServiceFabricVolumeDisk:
        w << L"ServiceFabricVolumeDisk";
        return;
    default:
        w << wformatString("Unknown ApplicationScopedVolumeProvider value {0}", static_cast<int>(val));
    }
}
