// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(ServiceReplicaDescription)

ServiceReplicaDescription::ServiceReplicaDescription(
    wstring const & replicaName)
    : replicaName_(replicaName)
{
}
