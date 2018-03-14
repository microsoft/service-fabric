// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

void ReplicaInfo::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->ReplicaInfo(
        contextSequenceId,
		Common::wformatString(replicaDescription_.PreviousConfigurationRole),
		Common::wformatString(icRole_),
		Common::wformatString(replicaDescription_.CurrentConfigurationRole),
		replicaDescription_);                
}
