// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;

EntityEntryBaseList LocalFailoverUnitMap::GetAllFailoverUnitEntries(bool excludeFM) const
{
    return GetEntries([excludeFM](EntityEntryBaseSPtr const & entry)
    {
        return !(excludeFM && static_cast<EntityEntry<FailoverUnit>&>(*entry).Id.Guid == Constants::FMServiceGuid);
    });
}

EntityEntryBaseList LocalFailoverUnitMap::GetFMFailoverUnitEntries() const
{
    EntityEntryBaseList v;
    auto entry = GetEntry(FailoverUnitId(Constants::FMServiceGuid));
    if (entry == nullptr)
    {
        return v;
    }

    v.push_back(move(entry));
    return v;
}

Infrastructure::EntityEntryBaseList LocalFailoverUnitMap::GetFailoverUnitEntries(
    FailoverManagerId const & owner) const
{
    if (owner == *FailoverManagerId::Fmm)
    {
        return GetFMFailoverUnitEntries();
    }
    else
    {
        return GetAllFailoverUnitEntries(true);
    }
}
