// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

BackgroundThreadContext::BackgroundThreadContext(wstring const & contextId)
    :   contextId_(contextId)
{
}

void BackgroundThreadContext::TransferUnprocessedFailoverUnits(BackgroundThreadContext & original)
{
    swap(unprocessedFailoverUnits_, original.unprocessedFailoverUnits_);
}

bool BackgroundThreadContext::MergeUnprocessedFailoverUnits(set<FailoverUnitId> const & failoverUnitIds)
{
    if (unprocessedFailoverUnits_.size() == 0)
    {
        unprocessedFailoverUnits_ = failoverUnitIds;
    }
    else
    {
        for (auto it = unprocessedFailoverUnits_.begin(); it != unprocessedFailoverUnits_.end();)
        {
            if (failoverUnitIds.find(*it) == failoverUnitIds.end())
            {
                unprocessedFailoverUnits_.erase(it++);
            }
            else
            {
                ++it;
            }
        }
    }

    return (unprocessedFailoverUnits_.size() == 0);
}

void BackgroundThreadContext::ClearUnprocessedFailoverUnits()
{
    unprocessedFailoverUnits_.clear();
}

void BackgroundThreadContext::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write("{0}: Unprocessed={1:3}", contextId_, unprocessedFailoverUnits_);
}
