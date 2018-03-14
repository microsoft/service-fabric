// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Client;
using namespace ServiceModel;
using namespace Management::HealthManager;
using namespace Reliability;

HealthReportSortedList::HealthReportSortedList()
    : list_()
{
}

HealthReportSortedList::HealthReportSortedList(HealthReportSortedList && other)
    : list_(move(other.list_))
{
}

HealthReportSortedList & HealthReportSortedList::operator= (HealthReportSortedList && other)
{
    if (this != &other)
    {
        list_ = move(other.list_);
    }

    return *this;
}

HealthReportSortedList::~HealthReportSortedList()
{
}

void HealthReportSortedList::Clear()
{
    list_.clear();
}

bool HealthReportSortedList::IsEmpty() const
{
    return list_.empty();
}

size_t HealthReportSortedList::Count() const
{
    return list_.size();
}

HealthReportWrapperPtr HealthReportSortedList::GetLastReport() const
{
    if (list_.empty())
    {
        return nullptr;
    }

    return *list_.rbegin();
}

void HealthReportSortedList::GetContentUpTo(
    size_t maxNumberOfReports,
    FABRIC_SEQUENCE_NUMBER lastAcceptedSN,
    __out vector<HealthReport> & reports,
    __out size_t & currentReportCount,
    __out FABRIC_SEQUENCE_NUMBER & lastLsn) const
{
    currentReportCount = 0;
    lastLsn = lastAcceptedSN;
    for (auto it = list_.begin(); it != list_.end() && currentReportCount < maxNumberOfReports && (*it)->Report.SequenceNumber < lastAcceptedSN; ++it)
    {
        (*it)->AddReportForSend(reports);
        ++currentReportCount;
    }

    if (currentReportCount > 0)
    {
        lastLsn = reports.back().SequenceNumber + 1;
    }
}

std::wstring HealthReportSortedList::ToString() const
{
    wstring details;
    StringWriter writer(details);
    for (auto it = list_.begin(); it != list_.end(); ++it)
    {
        writer.Write("{0}-{1}, ", (*it)->Report.EntityPropertyId, (*it)->Report.SequenceNumber);
    }

    return details;
}


void HealthReportSortedList::CheckInvariants() const
{
#ifdef DBG
    if (list_.empty())
    {
        return;
    }

    for (auto itFirst = list_.begin(), itSecond = ++list_.begin(); itSecond != list_.end(); ++itFirst, ++itSecond)
    {
        auto left = *itFirst;
        auto right = *itSecond;
        ASSERT_IF(left == nullptr || right == nullptr, "Comparator: null pointers are not allowed");
        
        if (left->Compare(*right) != -1)
        {
            Assert::CodingError("The list is not sorted: {0}", this->ToString());
        }
    }
#endif
}

Common::ErrorCode HealthReportSortedList::Add(HealthReportWrapperPtr const report)
{
    auto result = list_.insert(report);
    if (!result.second)
    {
        return ErrorCode(ErrorCodeValue::AlreadyExists);
    }

    CheckInvariants();
    return ErrorCode::Success();
}

Common::ErrorCode HealthReportSortedList::Remove(HealthReportWrapperPtr const report)
{
    auto count = list_.erase(report);
    if (count == 0)
    {
        return ErrorCode(ErrorCodeValue::NotFound);
    }

    CheckInvariants();
    return ErrorCode::Success();
}
