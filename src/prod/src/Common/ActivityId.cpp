// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    using namespace std;

    ActivityId ActivityId::Empty(Guid::Empty());

    ActivityId::ActivityId ()
      : guid_(Common::Guid::NewGuid())
      , index_(0)
    {
    }

    ActivityId::ActivityId(Common::Guid const & guid)
      : guid_(guid)
      , index_(0)
    {
    }

    ActivityId::ActivityId(Common::Guid && guid)
      : guid_(move(guid))
      , index_(0)
    {
    }

    ActivityId::ActivityId(ActivityId const & other)
      : guid_(other.guid_)
      , index_(other.index_)
    {
    }

    ActivityId::ActivityId(ActivityId && other)
      : guid_(move(other.guid_))
      , index_(move(other.index_))
    {
    }

    ActivityId::ActivityId(ActivityId const & other, uint64 shiftIndex)
        : guid_(other.guid_)
        , index_(other.index_ + shiftIndex)
    {
    }

    bool ActivityId::operator < (ActivityId const & other) const
    {
        if (guid_ < other.guid_)
        {
            return true;
        }
        else if (other.guid_ < guid_)
        {
            return false;
        }
        else if (index_ < other.index_)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool ActivityId::operator == (ActivityId const & other) const
    {
        if (guid_ == other.guid_ && index_ == other.index_)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool ActivityId::operator <= (ActivityId const & other) const
    {
        return (*this < other || *this == other);
    }

    bool ActivityId::operator != (ActivityId const & other) const
    {
        return !(*this == other);
    }

    bool ActivityId::operator > (ActivityId const & other) const
    {
        return (other < *this);
    }

    bool ActivityId::operator >= (ActivityId const & other) const
    {
        return (other < *this || *this == other);
    }

    void ActivityId::WriteTo(TextWriter& w, FormatOptions const&) const
    {
        w.Write("{0}:{1}", guid_, index_);
    }

    wstring ActivityId::ToString() const
    {
        return wformatString("{0}:{1}", guid_, index_);
    }

    string ActivityId::AddField(TraceEvent & traceEvent, std::string const & name)
    {
        traceEvent.AddField<Common::Guid>(name + ".id");
        traceEvent.AddField<uint64>(name + ".index");
        
        return "{0}:{1}";
    }

    void ActivityId::FillEventData(TraceEventContext & context) const
    {
        context.Write<Common::Guid>(guid_);
        context.Write<uint64>(index_);
    }

    void ActivityId::IncrementIndex()
    {
        ++index_;
    }

    ActivityId ActivityId::GetNestedActivity() const
    {
        ActivityId nested(*this);

        nested.IncrementIndex();

        return nested;
    }
}
