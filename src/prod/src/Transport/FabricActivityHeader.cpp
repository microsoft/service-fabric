// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Transport
{
    using namespace std;
    using namespace Common;

    FabricActivityHeader::FabricActivityHeader ()
      : activityId_()
    {
    }

    FabricActivityHeader::FabricActivityHeader(Common::Guid const & guid)
      : activityId_(guid)
    {
    }

    FabricActivityHeader::FabricActivityHeader(Common::ActivityId const & guid)
      : activityId_(guid)
    {
    }

    FabricActivityHeader::FabricActivityHeader(FabricActivityHeader const & other)
      : activityId_(other.ActivityId)
    {
    }

    FabricActivityHeader FabricActivityHeader::FromMessage(Message & message)
    {
        FabricActivityHeader activityHeader;
        if (!message.Headers.TryReadFirst(activityHeader))
        {
            Assert::TestAssert("FabricActivityHeader not found {0}", message.MessageId);
        }
        return activityHeader;
    }

    void FabricActivityHeader::WriteTo(TextWriter& w, FormatOptions const& f) const
    {
        activityId_.WriteTo(w, f);
    }

    wstring FabricActivityHeader::ToString() const
    {
        return activityId_.ToString();
    }
}
