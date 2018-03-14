// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::HealthManager;

HealthReportResult::HealthReportResult()
    : kind_(FABRIC_HEALTH_REPORT_KIND_INVALID)
    , entityPropertyId_()
    , sourceId_()
    , sequenceNumber_(FABRIC_INVALID_SEQUENCE_NUMBER)
    , error_(Common::ErrorCodeValue::NotReady)
{
}

HealthReportResult::HealthReportResult(
    HealthReport const & healthReport)
    : kind_(healthReport.Kind)
    , entityPropertyId_(healthReport.EntityPropertyId)
    , sourceId_(healthReport.SourceId)
    , sequenceNumber_(healthReport.SequenceNumber)
    , error_(ErrorCodeValue::NotReady)
{
}

HealthReportResult::HealthReportResult(
    HealthReport const & healthReport,
    ErrorCodeValue::Enum error)
    : kind_(healthReport.Kind)
    , entityPropertyId_(healthReport.EntityPropertyId)
    , sourceId_(healthReport.SourceId)
    , sequenceNumber_(healthReport.SequenceNumber)
    , error_(error)
{
}

HealthReportResult::HealthReportResult(HealthReportResult && other)
    : kind_(move(other.kind_))
    , entityPropertyId_(other.entityPropertyId_)
    , sourceId_(other.sourceId_)
    , sequenceNumber_(move(other.sequenceNumber_))
    , error_(move(other.error_))
{
}

HealthReportResult & HealthReportResult::operator = (HealthReportResult && other)
{
    if (this != &other)
    {
        kind_ = move(other.kind_);
        entityPropertyId_ = move(other.entityPropertyId_);
        sourceId_ = move(other.sourceId_);
        sequenceNumber_ = move(other.sequenceNumber_);
        error_ = move(other.error_);
    }

    return *this;
}

void HealthReportResult::WriteTo(Common::TextWriter& writer, FormatOptions const &) const
{
    writer.Write("{0}-{1}: {2}, {3} {4}", 
        kind_,
        entityPropertyId_,
        sourceId_,
        sequenceNumber_,
        error_);
}

std::wstring HealthReportResult::ToString() const
{
    return wformatString(*this);
}
