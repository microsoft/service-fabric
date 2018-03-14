// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

void ReplicaUpMessageBody::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->ReplicaUpMessageBody(
        contextSequenceId,
        isLastReplicaUpMessage_,
        isFromFMM_,
		replicaList_,
        droppedReplicas_);                
}
