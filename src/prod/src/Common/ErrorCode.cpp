// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

const DWORD UninitializedTid = 0;

StringLiteral const TraceComponent("ErrorCode");

//
// ErrorCode
//

ErrorCode::ErrorCode()
    : value_(ErrorCodeValue::Success),
      message_(),
      messageTid_(UninitializedTid),
      read_(false),
      enableOverwriteCheck_(false)
{
}

ErrorCode::ErrorCode(ErrorCode const& other)
    : value_(other.ReadValue()),
      message_(other.message_),
      messageTid_(other.messageTid_),
      read_(false),
      enableOverwriteCheck_(false)
{
}

ErrorCode::ErrorCode(ErrorCode && other)
    : value_(other.ReadValue()),
      message_(move(other.message_)),
      messageTid_(move(other.messageTid_)),
      read_(false),
      enableOverwriteCheck_(false)
{
}

ErrorCode::ErrorCode(ErrorCodeValue::Enum value)
    : value_(value),
      message_(),
      messageTid_(UninitializedTid),
      read_(false),
      enableOverwriteCheck_(false)
{
}

ErrorCode::ErrorCode(ErrorCodeValue::Enum value, wstring && msg)
    : value_(value),
      message_(move(msg)),
      messageTid_(UninitializedTid),
      read_(false),
      enableOverwriteCheck_(false)
{
}

ErrorCode::~ErrorCode()
{
    if (messageTid_ != UninitializedTid)
    {
        auto hr = ::FabricClearLastErrorMessage(messageTid_);
        if (FAILED(hr))
        {
            Trace.WriteWarning(TraceComponent, "~FabricClearLastErrorMessage({0}) failed: hr = {1}", messageTid_, hr);
        }
    }
}

ErrorCode & ErrorCode::operator=(ErrorCode const& other)
{
    if (this->TryAssignFrom(other, false))
    {
        message_ = other.message_;
    }

    return *this;
}

ErrorCode & ErrorCode::operator=(ErrorCode && other)
{
    if (this->TryAssignFrom(other, false))
    {
        message_ = move(other.message_);
    }

    return *this;
}

void ErrorCode::Overwrite(ErrorCode const& other)
{
    if (this->TryAssignFrom(other, true))
    {
        message_ = other.message_;
    }
}

void ErrorCode::Overwrite(ErrorCode && other)
{
    if (this->TryAssignFrom(other, true))
    {
        message_ = move(other.message_);
    }
}

ErrorCode & ErrorCode::Reset(ErrorCodeValue::Enum newValue)
{
    if (this->TryAssignFrom(newValue, false))
    {
        enableOverwriteCheck_ = false;
    }

    return *this;
}

ErrorCodeValue::Enum ErrorCode::ReadValue() const
{
    read_ = true;
    return value_;
}

bool ErrorCode::IsSuccess() const
{
    return ReadValue() == ErrorCodeValue::Success;
}

bool ErrorCode::IsError(ErrorCodeValue::Enum value) const
{
    return ReadValue() == value;
}

bool ErrorCode::IsWin32Error(DWORD win32Error) const
{
    return ReadValue() == static_cast<ErrorCodeValue::Enum>(HRESULT_FROM_WIN32(win32Error));
}

bool ErrorCode::IsNtStatus(NTSTATUS status) const
{
    return ReadValue() == static_cast<ErrorCodeValue::Enum>(HRESULT_FROM_NT(status));
}

void ErrorCode::SetThreadErrorMessage() const
{
    if (value_ != ErrorCodeValue::Success && !message_.empty())
    {
        if (messageTid_ != UninitializedTid)
        {
            auto hr = ::FabricClearLastErrorMessage(messageTid_);
            if (FAILED(hr))
            {
                Trace.WriteWarning(TraceComponent, "FabricClearLastErrorMessage({0}) failed: hr = {1}", messageTid_, hr);
            }
        }

        auto hr = ::FabricSetLastErrorMessage(message_.c_str(), &messageTid_);
        if (FAILED(hr))
        {
            Trace.WriteWarning(TraceComponent, "FabricSetLastErrorMessage failed: hr = {0}", hr);
        }
    }
}

wstring ErrorCode::GetAndClearThreadErrorMessage() 
{
    return GetThreadErrorMessage(true);
}

wstring ErrorCode::GetThreadErrorMessage()
{
    return GetThreadErrorMessage(false);
}

wstring ErrorCode::GetThreadErrorMessage(bool clear)
{
    wstring result;
    ComPointer<IFabricStringResult> stringCPtr;
    auto hr = ::FabricGetLastErrorMessage(stringCPtr.InitializationAddress());

    if (SUCCEEDED(hr))
    {
        if (clear)
        {
            hr = ::FabricClearLastErrorMessage(::GetCurrentThreadId());
            if (FAILED(hr))
            {
                Trace.WriteWarning(TraceComponent, "FabricClearLastErrorMessage, failed: hr = {0}", hr);
            }
        }

        hr = StringUtility::LpcwstrToWstring(stringCPtr->get_String(), true, result);
        if (FAILED(hr))
        {
            Trace.WriteWarning(TraceComponent, "FabricGetLastErrorMessage, LPCWSTR->wstring failed: hr = {0}", hr);
        }
    }
    else
    {
        Trace.WriteWarning(TraceComponent, "FabricGetLastErrorMessage failed: hr = {0}", hr);
    }

    return result;
}

void ErrorCode::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << value_;
}

void ErrorCode::WriteToEtw(uint16 contextSequenceId) const
{
    wstring temp;
    StringWriter(temp).Write("{0}", value_);

    CommonEventSource::Events->ErrorCode(contextSequenceId, temp);
}

HRESULT ErrorCode::ToHResult() const
{
    ErrorCodeValue::Enum value = ReadValue();

    // Add Conversion from Internal ErrorCodes to Public ErrorCodes here
    // WARNING: This conversion will loose information, if these values round trip 
    // to and from public API surface. If you need to preserve the different 
    // error code values, that survies round trips, add a new public ErrorCode
    switch (value)
    {
        case ErrorCodeValue::HostingServiceTypeAlreadyRegisteredToSameRuntime:
            value = ErrorCodeValue::HostingServiceTypeAlreadyRegistered;
            break;


        case ErrorCodeValue::HostingServiceTypeNotOwned:
            value = ErrorCodeValue::HostingServiceTypeNotRegistered;
            break;
    }

    return static_cast<HRESULT>(value);
}

NTSTATUS ErrorCode::ToNTStatus() const
{
    return ToHResult() & ~FACILITY_NT_BIT;
}

ErrorCode ErrorCode::FromHResult(HRESULT hr)
{
    return FromHResult(hr, false);
}

ErrorCode ErrorCode::FromHResult(HRESULT hr, bool captureThreadErrorMessage)
{
    if (SUCCEEDED(hr))
    {
        return ErrorCode(ErrorCodeValue::Success);
    }

    if (!captureThreadErrorMessage)
    {
        return ErrorCode(static_cast<ErrorCodeValue::Enum>(hr));
    }
    else
    {
        return FromHResult(hr, GetAndClearThreadErrorMessage());
    }
}

ErrorCode ErrorCode::FromHResult(HRESULT hr, std::wstring && message)
{
    if (SUCCEEDED(hr))
    {
        return ErrorCode(ErrorCodeValue::Success, move(message));
    }
    
    return ErrorCode(static_cast<ErrorCodeValue::Enum>(hr), move(message));
}

ErrorCode ErrorCode::FromWin32Error()
{
#ifdef PLATFORM_UNIX
    return FromErrno();
#else
    return FromWin32Error(::GetLastError());
#endif
}

ErrorCode ErrorCode::FromWin32Error(DWORD win32Error)
{
#ifdef PLATFORM_UNIX
    return FromErrno(win32Error);
#else
    return FromHResult(HRESULT_FROM_WIN32(win32Error));
#endif
}

ErrorCode ErrorCode::FromNtStatus(NTSTATUS status)
{
    return FromHResult(HRESULT_FROM_NT(status));
}

ErrorCode ErrorCode::FromErrno()
{
    return ErrorCode::FromErrno(errno);
}

ErrorCode ErrorCode::FromErrno(int error)
{
    //LINUXTODO consider removing HRESULT_FROM_WIN32 below (revert back to commit fab0ac79d978d646454ca13561154f031b5008c8),
    //so that ErrorCode generated from PAL win32 error will not accidentally collide with ErrorCode generated from errno.
    //This will require all code call FromWin32Error instead of directly calling FromHResult(HRESULT_FROM_WIN32(error)).
    return FromHResult(HRESULT_FROM_WIN32(error));
}

bool ErrorCode::IsErrno(int error) const
{
    return ReadValue() == static_cast<ErrorCodeValue::Enum>(HRESULT_FROM_WIN32(error));
}

ErrorCode ErrorCode::TraceReturn(
    ErrorCode const error, 
    TraceTaskCodes::Enum const & taskCode, 
    StringLiteral const & traceType,
    wstring const & traceId,
    StringLiteral const & operation)
{
    
    if (error.IsSuccess())
    {
        TraceNoise(taskCode, traceType, traceId,  "Operation:{0}, Result={1}", operation, error);
    }
    else
    {
        TraceWarning(taskCode, traceType, traceId,  "Operation:{0}, Result={1}", operation, error);
    }

    return error;
}

ErrorCode ErrorCode::TraceReturn(
    ErrorCode const error, 
    TraceTaskCodes::Enum const & taskCode, 
    StringLiteral const & traceType,
    StringLiteral const & operation)
{
    
    if (error.IsSuccess())
    {
        TraceNoise(taskCode, traceType, "Operation:{0}, Result={1}", operation, error);
    }
    else
    {
        TraceWarning(taskCode, traceType, "Operation:{0}, Result={1}", operation, error);
    }

    return error;
}

wstring ErrorCode::ErrorCodeValueToString() const
{
    wstring valueString;
    StringWriter writer(valueString);
    writer.Write(static_cast<ErrorCodeValue::Enum>(value_));
    return valueString;
}

ErrorCode ErrorCode::FirstError(ErrorCode const & left, ErrorCode const & right)
{
    if (!left.IsSuccess())
    {
        // discard right value
        right.ReadValue();
        return left;
    }
    else
    {
        return right;
    }
}

LogLevel::Enum ErrorCode::ToLogLevel(
    LogLevel::Enum errorLevel,
    LogLevel::Enum successLevel)
{
    if (this->IsSuccess())
    {
        return successLevel;
    }
    else
    {
        return errorLevel;
    }
}

bool ErrorCode::TryAssignFrom(ErrorCode const& other, bool allowOverwrite)
{
    if (this != &other)
    {
        // Read other value before asserting.  This prevents other from asserting in cases
        // where we assert.
        //
        auto otherValue = other.ReadValue();

        if (allowOverwrite)
        {
            if ((value_ != ErrorCodeValue::Success) && (otherValue == ErrorCodeValue::Success))
            {
                Assert::CodingError("Cannot overwrite error value {0} with Success.", value_);
            }
        }
        else
        {
            ASSERT_IF(enableOverwriteCheck_ && !read_, "ErrorCode {0} was not read", value_);
        }

        value_ = otherValue;
        messageTid_ = other.messageTid_;
        read_ = false;
        enableOverwriteCheck_ = true;

        return true;
    }
    else
    {
        return false;
    }
}

std::string ErrorCode::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    std::string format = "{0}";
    size_t index = 0;

    traceEvent.AddEventField<Common::ErrorCodeValue::Trace>(format, name + ".val", index);

    return format;
}

void ErrorCode::FillEventData(Common::TraceEventContext & context) const
{
    context.WriteCopy<uint>(static_cast<uint>(value_));
}

//
// FirstErrorTracker
//

FirstErrorTracker::FirstErrorTracker()
    : value_(ErrorCodeValue::Success)
{
}
