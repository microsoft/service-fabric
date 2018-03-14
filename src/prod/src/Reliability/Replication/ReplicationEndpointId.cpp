// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

bool ReplicationEndpointId::operator == (ReplicationEndpointId const & rhs) const
{
    return (this->replicaId_ == rhs.replicaId_) && 
        (this->partitionId_ == rhs.partitionId_) &&
        (this->incarnationId_ == rhs.incarnationId_);
}

bool ReplicationEndpointId::operator != (ReplicationEndpointId const & rhs) const
{
    return !(*this == rhs);
}

bool ReplicationEndpointId::operator < (ReplicationEndpointId const & rhs) const
{
    if (this->partitionId_ < rhs.partitionId_)
    {
        return true;
    }
    else if (this->partitionId_ == rhs.partitionId_)
    {
        if (this->replicaId_ < rhs.replicaId_)
        {
            return true;
        }
        else if (replicaId_ == rhs.replicaId_)
        {
            return this->incarnationId_ < rhs.incarnationId_;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

ReplicationEndpointId ReplicationEndpointId::FromString(
    std::wstring const & stringEndpointId)
{
    std::vector<std::wstring> tokens;
    auto indexAt = stringEndpointId.find_last_of(Constants::ReplicaIncarnationDelimiter);
    if (indexAt == std::wstring::npos)
    {
        Common::Assert::CodingError("Wrong format: {0}.  Cannot find @ between replicaId and incarnationId", stringEndpointId);
    }
    Common::Guid incarnationId(stringEndpointId.substr(indexAt + 1));

    auto findIter = stringEndpointId.find_last_of(Constants::PartitionReplicaDelimiter, indexAt);

    if (findIter == std::wstring::npos)
    {
        Common::Assert::CodingError("Wrong format: {0}", stringEndpointId);
    }

    Common::Guid guid(stringEndpointId.substr(0, findIter));
    ::FABRIC_REPLICA_ID id(0);

    id = Common::Int64_Parse<std::wstring>(stringEndpointId.substr(findIter + 1, indexAt - findIter - 1));

    return ReplicationEndpointId(guid, id, incarnationId);
}

std::wstring ReplicationEndpointId::ToString() const
{
    std::wstring id;
    Common::StringWriter writer(id);
    WriteTo(writer, Common::FormatOptions(0, false, ""));

    return id;
}

bool ReplicationEndpointId::IsEmpty() const
{
    return this->partitionId_ == Common::Guid::Empty() && 
           this->replicaId_ == 0 &&
           this->incarnationId_ == Common::Guid::Empty();
}

void ReplicationEndpointId::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const 
{
    // Trace out the parition id even though it is already available in the type of the trace because this method
    // is used for generating the endpoint as well (overloaded operator << for TextWriter)
    // It is okay to have this redundant info as the production environment will invoke AddField for tracing
    w.Write("{0}{1}{2}{3}{4}", partitionId_, Constants::PartitionReplicaDelimiter, replicaId_, Constants::ReplicaIncarnationDelimiter, incarnationId_);
}

std::string ReplicationEndpointId::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    std::string format = "{0};{1}";
    size_t index = 0;

    // Do not trace out the partition ID as it is redundant (It is already available in the type information of the trace)
    traceEvent.AddEventField<FABRIC_REPLICA_ID>(format, name + ".replicaId", index);
    traceEvent.AddEventField<Common::Guid>(format, name + ".incarnationId", index);

    return format;
}

void ReplicationEndpointId::FillEventData(Common::TraceEventContext & context) const
{
    // Do not trace out the partition ID as it is redundant (It is already available in the type information of the trace)
    context.WriteCopy<FABRIC_REPLICA_ID>(replicaId_);
    context.Write<Common::Guid>(incarnationId_);
}

}
}
