// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace std;

NamedRepartitionDescription::NamedRepartitionDescription()
    : RepartitionDescription(PartitionKind::Named)
    , namesToAdd_()
    , namesToRemove_()
{
}

NamedRepartitionDescription::NamedRepartitionDescription(vector<wstring> && toAdd, vector<wstring> && toRemove)
    : RepartitionDescription(PartitionKind::Named)
    , namesToAdd_(move(toAdd))
    , namesToRemove_(move(toRemove))
{
}

NamedRepartitionDescription::NamedRepartitionDescription(vector<wstring> const & toAdd, vector<wstring> const & toRemove)
    : RepartitionDescription(PartitionKind::Named)
    , namesToAdd_(toAdd)
    , namesToRemove_(toRemove)
{
}

NamedRepartitionDescription::NamedRepartitionDescription(NamedRepartitionDescription && other)
    : RepartitionDescription(PartitionKind::Named)
    , namesToAdd_(move(other.namesToAdd_))
    , namesToRemove_(move(other.namesToRemove_))
{
}

void NamedRepartitionDescription::OnWriteToEtw(uint16 contextSequenceId) const
{
    ServiceModel::ServiceModelEventSource::Trace->NamedRepartitionDescription(
        contextSequenceId,
        wformatString(namesToAdd_), 
        wformatString(namesToRemove_));
}

void NamedRepartitionDescription::WriteTo(__in Common::TextWriter& w, Common::FormatOptions const&) const
{
    w << "toAdd=" << namesToAdd_;
    w << ", toRemove=" << namesToRemove_;
}
