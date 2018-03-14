// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace ServiceModel;
using namespace Store;
using namespace Management::HealthManager;

GlobalWString CleanUpState = make_global<wstring>(L"cleanedUp,deleted");
GlobalWString DeletedState = make_global<wstring>(L"deleted");
GlobalWString UpdatedState = make_global<wstring>(L"updated");
GlobalWString InvalidatedState = make_global<wstring>(L"invalidated");

EntityStateFlags::EntityStateFlags(bool markForDeletion)
    : value_(0)
    , lastModifiedTime_(DateTime::Now())
    , isStale_(false)
    , hasSystemReport_(false)
    , expectSystemReports_(true)
    , systemErrorCount_(0)
{
    if (markForDeletion)
    {
        value_ |= MarkedForDeletion;
    }
}

EntityStateFlags::~EntityStateFlags()
{
}

void EntityStateFlags::set_ExpectSystemReports(bool value) 
{ 
    expectSystemReports_ = value; 
    if (!expectSystemReports_)
    {
        this->hasSystemReport_ = true;
    }
}

bool EntityStateFlags::HasNoChanges(Common::TimeSpan const & period) const
{
    return lastModifiedTime_.AddWithMaxValueCheck(period) <= Common::DateTime::Now();
}

bool EntityStateFlags::ShouldBeCleanedUp() const
{
    return IsMarkedForDeletion && HasNoChanges(ManagementConfig::GetConfig().HealthStoreCleanupGraceInterval);
}

bool EntityStateFlags::IsRecentlyInvalidated() const
{
    return IsInvalidated && !HasNoChanges(ManagementConfig::GetConfig().HealthStoreInvalidatedEntityGraceInterval);
}

void EntityStateFlags::UpdateStateModifiedTime()
{
    lastModifiedTime_ = DateTime::Now();
    // Reset both bits in Invalidated (includes CleanedUp)
    value_ &= (~Invalidated);
}

void EntityStateFlags::MarkForDeletion()
{
    value_ |= MarkedForDeletion;
    value_ &= (~Invalidated);
    lastModifiedTime_ = DateTime::Now();
    isStale_ = false;
}

void EntityStateFlags::MarkAsCleanedUp()
{
    // Update that it's cleaned up, but do not change the time - it should mark the moment 
    // it was marked for deletion
    // If the attributes are not marked for deletion (it happens on Invalidate calls),
    // update the time
    if (!IsMarkedForDeletion)
    {
        lastModifiedTime_ = DateTime::Now();
        value_ |= Invalidated;
    }
    else
    {
        value_ |= CleanedUp;
    }

    this->ResetSystemInfo();
}

void EntityStateFlags::MarkAsStale() 
{
    isStale_ = true; 
}

void EntityStateFlags::ResetSystemInfo() 
{ 
    if (expectSystemReports_)
    {
        hasSystemReport_ = false;
    }

    systemErrorCount_ = 0;
    isStale_ = false;
}

void EntityStateFlags::UpdateSystemErrorCount(int diff)
{
    if (diff != 0)
    {
        systemErrorCount_ += diff;
        ASSERT_IF(systemErrorCount_ < 0, "SystemErrorCount can't be negative: {0}", *this);
    }
}

void EntityStateFlags::Reset()
{
    ResetSystemInfo();
    value_ = None;
    lastModifiedTime_ = DateTime::Now();
    isStale_ = false;
}

void EntityStateFlags::PrepareCleanup()
{
    // Keep the last modified time
    // Reset the cleanup bit and set deleted bit (it may already be set)
    value_ &= (~Invalidated);
    value_ |= MarkedForDeletion;
    isStale_ = false;
}

void EntityStateFlags::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    if (IsInvalidated)
    {
        w.Write(*InvalidatedState);
    }
    else if (IsCleanedUp)
    {
        w.Write(*CleanUpState);
    }
    else if (IsMarkedForDeletion)
    {
        w.Write(*DeletedState);
    }
    else
    {
        w.Write(*UpdatedState);
    }

    w.Write("@{0} Stale={1}/systemReport={2}/systemError={3}", lastModifiedTime_, isStale_, hasSystemReport_, systemErrorCount_);
}

std::string EntityStateFlags::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    traceEvent.AddField<std::wstring>(name + ".state");
    traceEvent.AddField<Common::DateTime>(name + ".lmt");
    traceEvent.AddField<bool>(name + ".stale");
    traceEvent.AddField<bool>(name + ".systemReport");
    traceEvent.AddField<int>(name + ".systemError");
            
    return "{0}@{1} {2}/{3}/{4}";
}

void EntityStateFlags::FillEventData(TraceEventContext & context) const
{
    if (IsInvalidated)
    {
        context.Write<std::wstring>(*InvalidatedState);
    }
    else if (IsCleanedUp)
    {
        context.Write<std::wstring>(*CleanUpState);
    }
    else if (IsMarkedForDeletion)
    {
        context.Write<std::wstring>(*DeletedState);
    }
    else
    {
        context.Write<std::wstring>(*UpdatedState);
    }
    
    context.Write<Common::DateTime>(lastModifiedTime_);

    context.Write<bool>(isStale_);
    context.Write<bool>(hasSystemReport_);
    context.Write<int>(systemErrorCount_);
}
