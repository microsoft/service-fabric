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

HealthReportWrapper::HealthReportWrapper(ServiceModel::HealthReport && report)
    : report_(move(report))
    , reportState_(HealthReportClientState::PendingSend)
    , lastModifiedTime_(Stopwatch::Now())
{
}

HealthReportWrapper::HealthReportWrapper(HealthReportWrapper && other)
    : report_(move(other.report_))
    , reportState_(move(other.reportState_))
    , lastModifiedTime_(move(other.lastModifiedTime_))
{
}

HealthReportWrapper & HealthReportWrapper::operator=(HealthReportWrapper && other)
{
    if (this != &other)
    {
        report_ = move(other.report_);
        reportState_ = move(other.reportState_);
        lastModifiedTime_ = move(other.lastModifiedTime_);
    }

    return *this;
}

HealthReportWrapper::~HealthReportWrapper()
{
}

bool HealthReportWrapper::AddReportForSend(
    __inout vector<HealthReport> & reports,
    StopwatchTime const & now,
    TimeSpan const & retryTime)
{
    if (reportState_ == HealthReportClientState::PendingAck && 
        lastModifiedTime_ + retryTime > now)
    {
        // The report was sent less than retry time ago, do not add again
        return false;
    }

    reportState_ = HealthReportClientState::PendingAck;
    lastModifiedTime_ = now;
    reports.push_back(report_);
    return true;
}

void HealthReportWrapper::AddReportForSend(
    __inout vector<HealthReport> & reports)
{
    reportState_ = HealthReportClientState::PendingAck;
    lastModifiedTime_ = Stopwatch::Now();
    reports.push_back(report_);
}

bool HealthReportWrapper::operator<(HealthReportWrapper const & other) const
{
    return this->Compare(other) < 0;
}

int HealthReportWrapper::Compare(HealthReportWrapper const & other) const
{
    if (this->Report.SequenceNumber < other.Report.SequenceNumber)
    {
        return -1;
    }
    else if (this->Report.SequenceNumber == other.Report.SequenceNumber)
    {
        // Same sequence number can be used for different properties
        if (this->Report.EntityPropertyId < other.Report.EntityPropertyId)
        {
            return -1;
        }
        else if (this->Report.EntityPropertyId == other.Report.EntityPropertyId)
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        return 1;
    }
}

void HealthReportWrapper::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("{0} [sn {1}, {2} at {3}]", report_.EntityPropertyId, report_.SequenceNumber, reportState_, lastModifiedTime_);
}

std::string HealthReportWrapper::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    traceEvent.AddField<wstring>(name + ".reportId");
    traceEvent.AddField<FABRIC_SEQUENCE_NUMBER>(name + ".sn");
    HealthReportClientState::Trace::AddField(traceEvent, name + ".state");

    return "{0} [sn {1}, {2}]";
}

void HealthReportWrapper::FillEventData(TraceEventContext & context) const
{
    context.Write<wstring>(report_.EntityPropertyId);
    context.Write<FABRIC_SEQUENCE_NUMBER>(report_.SequenceNumber);
    context.Write<HealthReportClientState::Enum>(reportState_);
}
