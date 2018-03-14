// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;

void ReplicaUpReplyJobItemContext::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << wformatString("ReplicaUpReply IsInDroppedList = {0}. FT: \r\n {1}\r\n{2}", isInDroppedList_, ftInfo_.FailoverUnitDescription, ftInfo_.Replicas);
}

void ReplicaUpReplyJobItemContext::Trace(std::string const & id, Federation::NodeInstance const & nodeInstance, std::wstring const & activityId) const
{
    RAEventSource::Events->ReplicaUpReplyAction(id, nodeInstance, activityId, isInDroppedList_, ftInfo_.FailoverUnitDescription, ftInfo_.Replicas);
}

