// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

ReplicaCloseMode const ReplicaCloseMode::None(ReplicaCloseModeName::None);
ReplicaCloseMode const ReplicaCloseMode::Close(ReplicaCloseModeName::Close);
ReplicaCloseMode const ReplicaCloseMode::Drop(ReplicaCloseModeName::Drop);
ReplicaCloseMode const ReplicaCloseMode::DeactivateNode(ReplicaCloseModeName::DeactivateNode);
ReplicaCloseMode const ReplicaCloseMode::Abort(ReplicaCloseModeName::Abort);
ReplicaCloseMode const ReplicaCloseMode::Restart(ReplicaCloseModeName::Restart);
ReplicaCloseMode const ReplicaCloseMode::Delete(ReplicaCloseModeName::Delete);
ReplicaCloseMode const ReplicaCloseMode::Deactivate(ReplicaCloseModeName::Deactivate);
ReplicaCloseMode const ReplicaCloseMode::ForceAbort(ReplicaCloseModeName::ForceAbort);
ReplicaCloseMode const ReplicaCloseMode::ForceDelete(ReplicaCloseModeName::ForceDelete);
ReplicaCloseMode const ReplicaCloseMode::QueuedDelete(ReplicaCloseModeName::QueuedDelete);
ReplicaCloseMode const ReplicaCloseMode::AppHostDown(ReplicaCloseModeName::AppHostDown);
ReplicaCloseMode const ReplicaCloseMode::Obliterate(ReplicaCloseModeName::Obliterate);

void ReplicaCloseMode::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w << name_;
}

std::string ReplicaCloseMode::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    string format = "{0}";
    size_t index = 0;

    traceEvent.AddEventField<ReplicaCloseModeName::Trace>(format, name + ".name", index);

    return format;
}

void ReplicaCloseMode::FillEventData(Common::TraceEventContext & context) const
{
    context.WriteCopy<uint>(static_cast<uint>(name_));
}
