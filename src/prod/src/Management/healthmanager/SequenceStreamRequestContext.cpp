// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Management::HealthManager;

SequenceStreamRequestContext::SequenceStreamRequestContext(
    Store::ReplicaActivityId const & replicaActivityId,
    Common::AsyncOperationSPtr const & owner,
    __in ISequenceStreamRequestContextProcessor & requestProcessor,
    SequenceStreamInformation && sequenceStream)
    : RequestContext(replicaActivityId, owner)
    , sequenceStream_(move(sequenceStream))
    , requestProcessor_(requestProcessor)
{
}

SequenceStreamRequestContext::SequenceStreamRequestContext(SequenceStreamRequestContext && other)
    : RequestContext(move(other))
    , sequenceStream_(move(other.sequenceStream_))
    , requestProcessor_(other.requestProcessor_)
{
}

SequenceStreamRequestContext & SequenceStreamRequestContext::operator = (SequenceStreamRequestContext && other)
{
    if (this != &other)
    {
        sequenceStream_ = move(other.sequenceStream_);
        requestProcessor_ = other.requestProcessor_;
    }

    RequestContext::operator=(move(other));
    return *this;
}

SequenceStreamRequestContext::~SequenceStreamRequestContext()
{
}

void SequenceStreamRequestContext::OnRequestCompleted()
{
    requestProcessor_.OnSequenceStreamProcessed(this->Owner, *this, this->Error);
}

void SequenceStreamRequestContext::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "{0}:SequenceStreamContext({1}+{2}:{3} [{4}, {5}), invalidate:{6})", 
        this->ReplicaActivityId,
        sequenceStream_.SourceId,
        wformatString(sequenceStream_.Kind),
        sequenceStream_.Instance,
        sequenceStream_.FromLsn,
        sequenceStream_.UpToLsn,
        sequenceStream_.InvalidateLsn);
}

void SequenceStreamRequestContext::WriteToEtw(uint16 contextSequenceId) const
{
    HMCommonEvents::Trace->SequenceStreamRequestContextTrace(
        contextSequenceId,
        this->ReplicaActivityId,
        0, /*TODO: remove, not used*/
        sequenceStream_.SourceId,
        wformatString(sequenceStream_.Kind),
        sequenceStream_.Instance,
        sequenceStream_.FromLsn,
        sequenceStream_.UpToLsn,
        sequenceStream_.InvalidateLsn);
}

size_t SequenceStreamRequestContext::EstimateSize() const
{
    return sequenceStream_.EstimateSize();
}
