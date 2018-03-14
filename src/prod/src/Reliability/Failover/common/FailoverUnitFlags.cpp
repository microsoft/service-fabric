// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace Common;
using namespace Reliability::FailoverUnitFlags;

Reliability::FailoverUnitFlags::Flags::Flags(
    bool stateful,
    bool persisted,
    bool noData,
    bool toBeDeleted,
    bool swapPrimary,
    bool orphaned,
    bool upgrading)
    : value_(0)
{
    if (stateful)
    {
        value_ |= Stateful;
    }

    if (persisted)
    {
        value_ |= Persisted;
    }

    if (noData)
    {
        value_ |= NoData;
    }

    if (toBeDeleted)
    {
        value_ |= ToBeDeleted;
    }

    if (swapPrimary)
    {
        value_ |= SwappingPrimary;
    }

    if (orphaned)
    {
        value_ |= Orphaned;
    }

    if (upgrading)
    {
        value_ |= Upgrading;
    }
}

void Reliability::FailoverUnitFlags::Flags::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    if (value_ == None)
    {
        writer.Write("-");
        return;
    }

    if (IsStateful())
    {
        writer.Write("S");
    }

    if (HasPersistedState())
    {
        writer.Write("P");
    }

    if (IsNoData())
    {
        writer.Write("E");
    }

    if (IsToBeDeleted())
    {
        writer.Write("D");
    }

    if (IsSwappingPrimary())
    {
        writer.Write("W");
    }

    if (IsOrphaned())
    {
        writer.Write("O");
    }

    if (IsUpgrading())
    {
        writer.Write("B");
    }
}
