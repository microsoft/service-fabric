// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace Common;
using namespace Reliability::ReplicaFlags;

void Reliability::ReplicaFlags::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    if (val == None)
    {
        w << "-";
        return;
    }

    if (val & ToBeDroppedByFM)
    {
        w << "D";
    }

    if (val & ToBeDroppedByPLB)
    {
        w << "R";
    }

    if (val & ToBePromoted)
    {
        w << "P";
    }

    if (val & PendingRemove)
    {
        w << "N";
    }

    if (val & Deleted)
    {
        w << "Z";
    }

    if (val & PreferredPrimaryLocation)
    {
        w << "L";
    }

    if (val & PreferredReplicaLocation)
    {
        w << "M";
    }

    if (val & PrimaryToBeSwappedOut)
    {
        w << "I";
    }

    if (val & PrimaryToBePlaced)
    {
        w << "J";
    }

    if (val & ReplicaToBePlaced)
    {
        w << "K";
    }

    if (val & EndpointAvailable)
    {
        w << "E";
    }

    if (val & MoveInProgress)
    {
        w << "V";
    }

    if (val & ToBeDroppedForNodeDeactivation)
    {
        w << "T";
    }
}
