// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace std;
using namespace Common;

Common::WStringLiteral const UnreliableTransportSpecification::MatchAll(L"*");

UnreliableTransportSpecification::UnreliableTransportSpecification(
    std::wstring const & name,
    std::wstring const & source,
    std::wstring const & destination, 
    std::wstring const & action, 
    double probabilityToApply, 
    Common::TimeSpan delay, 
    Common::TimeSpan delaySpan,
    int priority, 
    int applyCount,
    Common::StringMap const &  filters)
    :   name_(name),
        source_(source == MatchAll ? L"" : source),
        destination_(destination == MatchAll ? L"" : destination),
        action_(action == MatchAll ? L"" : action),
        probabilityToApply_(probabilityToApply),
        delay_(delay),
        delaySpan_(static_cast<double>(delaySpan.TotalMilliseconds())),
        priority_(priority),
        applyCount_(static_cast<LONG>(applyCount)),
        filters_(filters),
        countActive_(applyCount > 0),
        random_()
{
    ASSERT_IF(probabilityToApply <= 0 || probabilityToApply > 1, "invalid probabilityToApply, must be (0, 1]");
}

bool UnreliableTransportSpecification::Match(std::wstring const & source, wstring const & destination, wstring const & action)
{
    return ((source_.empty() || source_ == source) &&
            (destination_.empty() || destination_ == destination) &&
            (action_.empty() || action_ == action));
}

bool UnreliableTransportSpecification::Match(std::wstring const & source, wstring const & destination, Transport::Message const & message)
{
    bool match = Match(source, destination, message.Action);

    // look at filters if above match is true
    if (match && !filters_.empty())
    {        
        for (auto iter = filters_.begin(); iter != filters_.end(); ++iter)
        {
            wstring propertyValue;

            // each property in filter must match. Currently only PartitionID is supported.
            if (StringUtility::Compare(iter->first, *Constants::PartitionIdWString) == 0)
            {
                if (message.TryGetProperty<Common::Guid>(Constants::PartitionIdString.begin()) != nullptr)
                {                    
                    propertyValue = message.GetProperty<Common::Guid>(Constants::PartitionIdString.begin()).ToString();
                }
            }
            else
            {
                match = false;
                break;
            }

            // Property value must match with filter
            if(!StringUtility::Compare(iter->second, propertyValue) == 0)
            {
                match = false;
                break;
            }            
        }
    }

    return match;
}

TimeSpan UnreliableTransportSpecification::GetDelay()
{
    AcquireWriteLock grab(lock_);
    if (random_.NextDouble() <= this->probabilityToApply_)
    {
        if (countActive_ && --applyCount_ < 0)
        {
            priority_ = -1;
            return TimeSpan::Zero;
        }

        if (this->delay_ == TimeSpan::MaxValue)
        {
            // user should drop message
            return TimeSpan::MaxValue;
        }

        TimeSpan partial = TimeSpan::FromMilliseconds(random_.NextDouble() * this->delaySpan_);
        return this->delay_ + partial;
    }

    return TimeSpan::Zero;
}

void UnreliableTransportSpecification::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    wstring filterInfo;
    for (auto iter = filters_.begin(); iter != filters_.end(); ++iter)
    {
        filterInfo += wformatString("{0}:{1},", iter->first, iter->second);
    }

    // trim "," from end of filters
    if (filterInfo.length() > 0)
    {
        filterInfo.pop_back();
    }    

    w.Write("{0}: {1}-{2} action:{3} delay:{4}/{5} probability:{6}, applyCount:{7}, Filters:{8}",
            name_, source_, destination_, action_, delay_, delaySpan_, probabilityToApply_, applyCount_, filterInfo);
}
