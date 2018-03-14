// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;

Common::Guid const MessageId::localProcessGuid_ = Common::Guid::NewGuid();
Common::atomic_long MessageId::localProcessCounter_(0);

MessageId::MessageId()
{
    guid_ = MessageId::localProcessGuid_;
    index_ = static_cast<DWORD>(++MessageId::localProcessCounter_);
}

MessageId::MessageId(MessageId const & rhs)
{
    guid_ = rhs.guid_;
    index_ = rhs.index_;
}

MessageId::MessageId(Common::Guid const & guid, DWORD index)
{
    guid_ = guid;
    index_ = index;
}

Common::Guid const & MessageId::get_Guid() const { return guid_; }
DWORD MessageId::get_Index() const { return index_; }

void MessageId::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{ 
    w.Write("{0}:{1}", guid_, index_);
}

bool MessageId::operator == (MessageId const & rhs) const
{
    return (index_ == rhs.index_) && (guid_ == rhs.guid_);
}

bool MessageId::operator != (MessageId const rhs) const
{
    return !(*this == rhs);
}

bool MessageId::operator < (MessageId const & rhs) const
{
    if (guid_ < rhs.guid_)
    {
        return true;
    }
    else if (guid_ == rhs.guid_)
    {
        return (index_ < rhs.index_);
    }
    else
    {
        return false;
    }
}

bool MessageId::IsEmpty() const
{
    return (this->index_ == 0 && this->guid_ == Common::Guid::Empty());
}

size_t MessageId::GetHash() const
{
    return guid_.GetHashCode() + index_;
}

std::string MessageId::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    traceEvent.AddField<Common::Guid>(name + ".id");
    traceEvent.AddField<uint32>(name + ".index");
            
    return "{0}:{1}";
}

void MessageId::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<Common::Guid>(guid_);
    context.Write<uint32>(index_);
}
