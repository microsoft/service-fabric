// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Management;
using namespace BackupRestoreAgentComponent;

BackupRestoreAgentProxyId::BackupRestoreAgentProxyId(Federation::NodeId const & nodeId, Common::Guid const & hostId)
    : nodeId_(nodeId),
    hostId_(hostId)
{
}

void BackupRestoreAgentProxyId::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer << "node " << nodeId_ << "/apphost " << hostId_;
}

string BackupRestoreAgentProxyId::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "node {0}/apphost {1}";
    size_t index = 0;

    traceEvent.AddEventField<Federation::NodeId>(format, name + ".nodeId", index);
    traceEvent.AddEventField<Common::Guid>(format, name + ".hostId", index);

    return format;
}

void BackupRestoreAgentProxyId::FillEventData(TraceEventContext & context) const
{
    context.Write(nodeId_);
    context.Write(hostId_);
}
