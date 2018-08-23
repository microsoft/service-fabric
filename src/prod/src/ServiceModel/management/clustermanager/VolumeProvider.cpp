// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::ClusterManager;

void VolumeProvider::WriteToTextWriter(__in TextWriter & w, Enum const & val)
{
    switch (val)
    {
    case VolumeProvider::Invalid:
        w << L"Invalid";
        return;
    case VolumeProvider::AzureFile:
        w << L"AzureFile";
        return;
    case VolumeProvider::ServiceFabricVolumeDisk:
        w << L"ServiceFabricVolumeDisk";
        return;
    default:
        w << wformatString("Unknown VolumeProvider value {0}", static_cast<int>(val));
    }
}
