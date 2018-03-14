// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

namespace Api
{
  ComGetUnplacedReplicaInformationResult::ComGetUnplacedReplicaInformationResult(
    UnplacedReplicaInformationQueryResult const & unplacedReplicaInformation)
        : unplacedReplicaInformation_()
        , heap_()
    {
      unplacedReplicaInformation_ = heap_.AddItem<FABRIC_UNPLACED_REPLICA_INFORMATION>();

      unplacedReplicaInformation.ToPublicApi(heap_, *unplacedReplicaInformation_);
    }

    const FABRIC_UNPLACED_REPLICA_INFORMATION *STDMETHODCALLTYPE ComGetUnplacedReplicaInformationResult::get_UnplacedReplicaInformation(void)
    {
        return unplacedReplicaInformation_.GetRawPointer();
    }
}
