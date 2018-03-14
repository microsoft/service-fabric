// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::ClusterManager;


NodeTaskDescription::NodeTaskDescription()
    : nodeName_()
    , taskType_(NodeTask::Invalid)
{
}

NodeTaskDescription::NodeTaskDescription(NodeTaskDescription && other)
    : nodeName_(move(other.nodeName_))
    , taskType_(move(other.taskType_))
{
}

NodeTaskDescription::NodeTaskDescription(NodeTaskDescription const & other)
    : nodeName_(other.nodeName_)
    , taskType_(other.taskType_)
{
}

NodeTaskDescription::NodeTaskDescription(wstring const & nodeName, NodeTask::Enum taskType)
    : nodeName_(nodeName)
    , taskType_(taskType)
{
}

NodeTaskDescription& NodeTaskDescription::operator=(NodeTaskDescription && other)
{
    if (this != &other)
    {
        nodeName_ = std::move(other.nodeName_);
        taskType_ = other.taskType_;
    }
    return *this;
}

bool NodeTaskDescription::operator < (NodeTaskDescription const & other) const
{
    if (nodeName_ < other.nodeName_)
    {
        return true;
    }
    else if (nodeName_ == other.nodeName_ && taskType_ < other.taskType_)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool NodeTaskDescription::operator == (NodeTaskDescription const & other) const
{
    return (nodeName_ == other.nodeName_ && taskType_ == other.taskType_);
}

bool NodeTaskDescription::operator != (NodeTaskDescription const & other) const
{
    return !(*this == other);
}

void NodeTaskDescription::WriteToEtw(uint16 contextSequenceId) const
{
    ServiceModel::ServiceModelEventSource::Trace->NodeTaskDescription(contextSequenceId, nodeName_, taskType_);
}

void NodeTaskDescription::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("{0}:{1}", nodeName_, taskType_);
}

string NodeTaskDescription::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "{0}:{1}";
    size_t index = 0;

    traceEvent.AddEventField<wstring>(format, name + ".name", index);
    traceEvent.AddEventField<NodeTask::Trace>(format, name + ".type", index);

    return format;
}

void NodeTaskDescription::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<wstring>(nodeName_);
    context.Write<NodeTask::Trace>(taskType_);
}
