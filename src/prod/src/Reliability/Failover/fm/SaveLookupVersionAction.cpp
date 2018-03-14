// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

SaveLookupVersionAction::SaveLookupVersionAction(int64 lookupVersion)
    : lookupVersion_(lookupVersion)
{
}

int SaveLookupVersionAction::OnPerformAction(FailoverManager& failoverManager)
{
    failoverManager.FailoverUnitCacheObj.SaveLookupVersion(lookupVersion_);

    return 0;
}

void SaveLookupVersionAction::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("SaveLookupVersionAction");
}

void SaveLookupVersionAction::WriteToEtw(uint16 contextSequenceId) const
{
    FailoverManager::FMEventSource->StateMachineAction(contextSequenceId, L"SaveLookupVersionAction");
}
