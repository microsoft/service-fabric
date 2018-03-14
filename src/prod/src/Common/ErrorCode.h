// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ErrorCode
    {
    public:
        ErrorCode();
        ErrorCode(ErrorCode const&);
        ErrorCode(ErrorCode &&);
        ErrorCode(ErrorCodeValue::Enum);
        ErrorCode(ErrorCodeValue::Enum, std::wstring && msg);
        ~ErrorCode();

        ErrorCode & operator=(ErrorCode const&);
        ErrorCode & operator=(ErrorCode &&);
        void Overwrite(ErrorCode const&);
        void Overwrite(ErrorCode &&);
        ErrorCode & Reset(ErrorCodeValue::Enum newValue = ErrorCodeValue::Success);

        ErrorCodeValue::Enum ReadValue() const;

        __declspec(property(get=get_Message)) std::wstring const & Message;
        inline std::wstring const& get_Message() const { return message_; }
        std::wstring TakeMessage() { return move(message_); }

        static inline ErrorCode Success() { return ErrorCodeValue::Success; }

        bool IsSuccess() const;
        bool IsError(ErrorCodeValue::Enum value) const;
        bool IsWin32Error(DWORD win32Error) const;
        bool IsNtStatus(NTSTATUS status) const;
        bool IsErrno(int error) const;

        void SetThreadErrorMessage() const;
        static std::wstring GetThreadErrorMessage();        
        static std::wstring GetAndClearThreadErrorMessage();

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        void WriteToEtw(uint16 contextSequenceId) const;
        std::wstring ErrorCodeValueToString() const;

        HRESULT ToHResult() const;
        NTSTATUS ToNTStatus() const;
        static ErrorCode FromHResult(HRESULT);
        static ErrorCode FromHResult(HRESULT, std::wstring && message);
        static ErrorCode FromHResult(HRESULT hr, bool captureThreadErrorMessage);
        static ErrorCode FromWin32Error();
        static ErrorCode FromWin32Error(DWORD win32Error);
        static ErrorCode FromNtStatus(NTSTATUS status);
        static ErrorCode FromErrno();
        static ErrorCode FromErrno(int error);

        static ErrorCode TraceReturn(
            ErrorCode const error, 
            TraceTaskCodes::Enum const & taskCode, 
            StringLiteral const & traceType,
            std::wstring const & traceId,
            StringLiteral const & operation);
        static ErrorCode TraceReturn(
            ErrorCode const error, 
            TraceTaskCodes::Enum const & taskCode, 
            StringLiteral const & traceType,
            StringLiteral const & operation);

        static ErrorCode FirstError(ErrorCode const & left, ErrorCode const & right);

        LogLevel::Enum ToLogLevel(
            LogLevel::Enum errorLevel = LogLevel::Warning,
            LogLevel::Enum successLevel = LogLevel::Noise);

        FABRIC_PRIMITIVE_FIELDS_01(value_);

    private:
        static std::wstring GetThreadErrorMessage(bool clear);
        bool TryAssignFrom(ErrorCode const&, bool allowOverwrite);

        ErrorCodeValue::Enum value_;

        std::wstring message_;
        mutable DWORD messageTid_;
        mutable bool read_;

        // This is set to true whenever we initialize or assign this from another ErrorCode.
        // If we initialize from an explicit ErrorCodeValue we do not need to check since
        // the code already knows the value.
        //
        mutable bool enableOverwriteCheck_;
    };

    // An atomic FirstError
    class FirstErrorTracker
    {
    public:
        FirstErrorTracker();

        ErrorCodeValue::Enum ReadValue() const { return value_; };

        void Update(ErrorCode newValue) { InterlockedCompareExchange(reinterpret_cast<LONG volatile *>(&value_), newValue.ReadValue(), ErrorCodeValue::Success); }

    private:
        ErrorCodeValue::Enum value_;
    };
}
