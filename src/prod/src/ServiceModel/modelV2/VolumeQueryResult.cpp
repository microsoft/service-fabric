// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::ClusterManager;
using namespace ServiceModel;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(VolumeQueryResult)

VolumeQueryResult::VolumeQueryResult(VolumeDescriptionSPtr const & volumeDescription)
    : volumeName_((volumeDescription == nullptr) ? L"" : volumeDescription->VolumeName)
    , volumeDescription_(volumeDescription)
{

}

std::wstring VolumeQueryResult::CreateContinuationToken() const
{
    return volumeName_;
}
