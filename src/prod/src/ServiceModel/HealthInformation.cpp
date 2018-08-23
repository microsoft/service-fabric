// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("HealthInfo");

// Do not initialize the serialization overhead; each derived class should do that
INITIALIZE_BASE_DYNAMIC_SIZE_ESTIMATION(HealthInformation)

GlobalWString HealthInformation::DeleteProperty = make_global<wstring>(L"DeleteProperty");

HealthInformation::HealthInformation()
    : sourceId_()
    , property_()
    , timeToLive_(TimeSpan::MaxValue)
    , state_(FABRIC_HEALTH_STATE_INVALID)
    , description_()
    , sequenceNumber_(FABRIC_AUTO_SEQUENCE_NUMBER)
    , removeWhenExpired_(false)
    , sourceUtcTimestamp_(DateTime::Zero)
{
}

HealthInformation::HealthInformation(
    std::wstring const & sourceId,
    std::wstring const & property,
    Common::TimeSpan const & timeToLive,
    FABRIC_HEALTH_STATE state,
    std::wstring const & description,
    FABRIC_SEQUENCE_NUMBER sequenceNumber,
    Common::DateTime const & sourceUtcTimestamp,
    bool removeWhenExpired)
    : sourceId_(sourceId)
    , property_(property)
    , timeToLive_(timeToLive)
    , state_(state)
    , description_(description)
    , sequenceNumber_(sequenceNumber)
    , removeWhenExpired_(removeWhenExpired)
    , sourceUtcTimestamp_(sourceUtcTimestamp)
{
}

HealthInformation::HealthInformation(
    std::wstring && sourceId,
    std::wstring && property,
    Common::TimeSpan const & timeToLive,
    FABRIC_HEALTH_STATE state,
    std::wstring const & description,
    FABRIC_SEQUENCE_NUMBER sequenceNumber,
    Common::DateTime const & sourceUtcTimestamp,
    bool removeWhenExpired)
    : sourceId_(move(sourceId))
    , property_(move(property))
    , timeToLive_(timeToLive)
    , state_(state)
    , description_(description)
    , sequenceNumber_(sequenceNumber)
    , removeWhenExpired_(removeWhenExpired)
    , sourceUtcTimestamp_(sourceUtcTimestamp)
{
}

HealthInformation::~HealthInformation()
{
}

ErrorCode HealthInformation::ToCommonPublicApi(
    __in ScopedHeap & heap, 
    __out FABRIC_HEALTH_INFORMATION & healthInformation) const
{
    if (sourceId_.empty())
    {
        Trace.WriteInfo(TraceSource, "HealthInformation: sourceId is empty");
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("{0} SourceId", HMResource::GetResources().HealthReportInvalidEmptyInput));
    }

    if (property_.empty())
    {
        Trace.WriteInfo(TraceSource, "HealthInformation: property is empty");
        return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("{0} Property", HMResource::GetResources().HealthReportInvalidEmptyInput));
    }

    DWORD timeToLiveSeconds = 0;
    if (timeToLive_ == TimeSpan::MaxValue)
    {
        timeToLiveSeconds = FABRIC_HEALTH_REPORT_INFINITE_TTL;
    }
    else
    {
        HRESULT hr = Int64ToDWord(timeToLive_.TotalSeconds(), &timeToLiveSeconds);
        if (!SUCCEEDED(hr))
        {
            return ErrorCode::FromHResult(hr);
        }
    }

    healthInformation.Description = heap.AddString(description_);
    healthInformation.Property = heap.AddString(property_);
    healthInformation.SourceId = heap.AddString(sourceId_);
    healthInformation.SequenceNumber = sequenceNumber_;
    healthInformation.State = state_;
    healthInformation.TimeToLiveSeconds = timeToLiveSeconds;
    healthInformation.RemoveWhenExpired = removeWhenExpired_ ? TRUE : FALSE;

    return ErrorCodeValue::Success;
}

ErrorCode HealthInformation::FromCommonPublicApi(
    FABRIC_HEALTH_INFORMATION const & healthInformation)
{
    HRESULT hr = StringUtility::LpcwstrToWstring(healthInformation.Property, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, property_);
    if (FAILED(hr)) 
    {
        auto error = ErrorCode::FromHResult(hr, wformatString("{0} {1}", HMResource::GetResources().HealthReportInvalidProperty, ParameterValidator::MaxStringSize));
        Trace.WriteInfo(TraceSource, "HealthInformation::FromCommonPublicApi {0}: {1}", error, error.Message);
        return error;
    }
    
    auto error = StringUtility::LpcwstrToTruncatedWstring(healthInformation.Description, true, 0, ParameterValidator::MaxStringSize, description_);
    if (!error.IsSuccess()) 
    { 
        auto innerError = ErrorCode::FromHResult(hr, wformatString("{0} {1}", HMResource::GetResources().HealthReportInvalidDescription, ParameterValidator::MaxStringSize));
        Trace.WriteInfo(TraceSource, "HealthInformation::FromCommonPublicApi {0}: {1}", innerError, innerError.Message);
        return innerError;
    }

    hr = StringUtility::LpcwstrToWstring(healthInformation.SourceId, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, sourceId_);
    if (FAILED(hr)) 
    { 
        error = ErrorCode::FromHResult(hr, wformatString("{0} {1}", HMResource::GetResources().HealthReportInvalidSourceId, ParameterValidator::MaxStringSize));
        Trace.WriteInfo(TraceSource, "HealthInformation::FromCommonPublicApi {0}: {1}", error, error.Message);
        return error;
    }
            
    state_ = healthInformation.State;
    
    sequenceNumber_ = healthInformation.SequenceNumber;
    
    if (healthInformation.TimeToLiveSeconds == FABRIC_HEALTH_REPORT_INFINITE_TTL)
    {
        timeToLive_ = TimeSpan::MaxValue;
    }
    else
    {
        timeToLive_ = TimeSpan::FromSeconds(healthInformation.TimeToLiveSeconds);
    }

    removeWhenExpired_ = (healthInformation.RemoveWhenExpired == TRUE) ? true : false;

    // TODO: 7953769: we should disallow system reports sent by public users.
    // CUrrently, this can break system components like RA that do not use the src\Api to avoid conversion
    // to COM, and also the system components that report through managed.
    error = TryAccept(false /*disallowSystemReport*/);
    if (!error.IsSuccess())
    {
        return error;
    }
        
    return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode HealthInformation::ValidateReport() const
{
    if (property_ == *DeleteProperty && 
        !StringUtility::StartsWithCaseInsensitive<wstring>(sourceId_, *Constants::EventSystemSourcePrefix))
    {
        // Used by HM only, no need to add resource string for message.
        auto error = ErrorCode(ErrorCodeValue::InvalidArgument);
        Trace.WriteInfo(TraceSource, "HealthInformation::ValidateReport only system source can delete reports: {0} - {1}", sourceId_, property_);
        return error;
    }

    return ErrorCode::Success();
}

Common::ErrorCode HealthInformation::ValidateNonSystemReport() const
{
    if (StringUtility::StartsWithCaseInsensitive<wstring>(sourceId_, *Constants::EventSystemSourcePrefix))
    {
        auto error = ErrorCode(
            ErrorCodeValue::InvalidArgument, 
            wformatString("{0} {1}", HMResource::GetResources().HealthReportReservedSourceId, sourceId_));
        Trace.WriteInfo(TraceSource, "HealthInformation::ValidateNonSystemReport {0}: {1}", error, error.Message);
        return error;
    }

    if (property_ == *DeleteProperty)
    {
        auto error = ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString("{0} {1}", HMResource::GetResources().HealthReportReservedProperty, property_));
        Trace.WriteInfo(TraceSource, "HealthInformation::ValidateNonSystemReport {0}: {1}", error, error.Message);
        return error;
    }

    return ErrorCode::Success();
}

Common::ErrorCode HealthInformation::TryAccept(bool disallowSystemReport)
{
    auto error = TryAcceptUserOrSystemReport();
    if (!error.IsSuccess())
    {
        return error;
    }

    if (disallowSystemReport)
    {
        error = ValidateNonSystemReport();
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    sourceUtcTimestamp_ = DateTime::Now();
    if (sequenceNumber_ <= FABRIC_AUTO_SEQUENCE_NUMBER)
    {
        sequenceNumber_ = SequenceNumber::GetNext();
    }

    return ErrorCode::Success();
}

Common::ErrorCode HealthInformation::TryAcceptUserOrSystemReport()
{
    if (sourceId_.empty())
    {
        auto error = ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("{0} SourceId", HMResource::GetResources().HealthReportInvalidEmptyInput));
        Trace.WriteInfo(TraceSource, "HealthInformation::TryAccept: {0}: {1}", error, error.Message);
        return error;
    }

    if (property_.empty())
    {
        auto error = ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("{0} Property", HMResource::GetResources().HealthReportInvalidEmptyInput));
        Trace.WriteInfo(TraceSource, "HealthInformation::TryAccept: {0}: {1}", error, error.Message);
        return error;
    }

    // FromPublicApi truncates the input string to desired length.
    // REST and internal reports are not going through public API conversion, so the truncation happens here.
    if (StringUtility::TruncateIfNeeded(description_, ParameterValidator::MaxStringSize))
    {
        // Truncate the description
        Trace.WriteInfo(TraceSource, "HealthInformation::TryAccept: Truncate description from {0} to {1}", description_.size(), ParameterValidator::MaxStringSize);
    }

    if ((state_ == FABRIC_HEALTH_STATE_INVALID) ||
        (state_ == FABRIC_HEALTH_STATE_UNKNOWN))
    {
        auto error = ErrorCode(
            ErrorCodeValue::InvalidArgument, 
            wformatString("{0} {1}", HMResource::GetResources().HealthReportInvalidHealthState, state_));
        Trace.WriteInfo(TraceSource, "HealthInformation::TryAccept: {0}: {1}", error, error.Message);
        return error;
    }

    if (sequenceNumber_ <= FABRIC_INVALID_SEQUENCE_NUMBER)
    {
        auto error = ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(HMResource::GetResources().HealthReportInvalidSequenceNumber, sequenceNumber_));
        Trace.WriteInfo(TraceSource, "HealthInformation::TryAccept: {0}: {1}", error, error.Message);
        return error;
    }

    return ErrorCode::Success();
}

void HealthInformation::WriteTo(__in TextWriter& writer, FormatOptions const &) const
{
    writer.Write(
        "HealthInfo(source={0} property={1} ttl={2} state={3} desc={4} seq={5} removeWhenExpired={6})",
        sourceId_,
        property_,
        timeToLive_,
        state_,
        description_,
        sequenceNumber_,
        removeWhenExpired_);
}
