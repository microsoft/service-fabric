// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
#define RETURN_IF_ERROR( error ) \
    if (!error.IsSuccess()) { return error; } \

#define CO_RETURN_IF_ERROR( error ) \
    if (!error.IsSuccess()) { co_return error; } \

#define TRY_CATCH_VOID( expr ) \
    { \
        auto exceptionError = this->TryCatchVoid(#expr, [&]() -> void { expr; }); \
        RETURN_IF_ERROR( exceptionError )\
    } \

#define TRY_CATCH_ERROR( expr ) \
    { \
        auto exceptionError = this->TryCatchError(#expr, [&]() -> Common::ErrorCode { return expr; }); \
        RETURN_IF_ERROR( exceptionError )\
    } \

#define TRY_CATCH_NOOP( expr ) \
    this->TryCatchVoid(#expr, [&]() -> void { expr; }).ReadValue(); \

#define TRY_CATCH_VOID_CO_RETURN( expr ) \
    { \
        auto exceptionError = this->TryCatchVoid(#expr, [&]() -> void { expr; }); \
        CO_RETURN_IF_ERROR( exceptionError ) \
    } \

#define TRY_CATCH_ASYNC( expr ) \
    { \
        auto exceptionError = co_await this->TryCatchAsync(#expr, expr); \
        CO_RETURN_IF_ERROR( exceptionError ) \
    } \

    class TSComponent : private Common::TextTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
    private:
        static Common::GlobalWString TypeKeyDelimiter;

    protected:
        TSComponent();
        virtual ~TSComponent();

    protected:

        //
        // Wrapper functions for calling V2 store stack APIs, which throw exceptions
        //

        Common::ErrorCode TryCatchVoid(StringLiteral const & tag, std::function<void(void)> const &) const;
        Common::ErrorCode TryCatchError(StringLiteral const & tag, std::function<Common::ErrorCode(void)> const &) const;

        ktl::Awaitable<Common::ErrorCode> TryCatchAsync(StringLiteral const & tag, __in ktl::Awaitable<void> &) const;
        ktl::Awaitable<Common::ErrorCode> TryCatchAsync(StringLiteral const & tag, __in ktl::Awaitable<void> &&) const;
        ktl::Awaitable<Common::ErrorCode> TryCatchAsync(StringLiteral const & tag, __in ktl::Awaitable<Common::ErrorCode> &) const;

        Common::ErrorCode FromNtStatus(StringLiteral const & tag, NTSTATUS) const;

    protected:

        virtual Common::StringLiteral const & GetTraceComponent() const = 0;
        virtual std::wstring const & GetTraceId() const = 0;

    protected:

        //
        // Key/Value entry conversion
        //

        void SplitKey(KString::SPtr const &, __out std::wstring & type, __out std::wstring & key);
        Common::ErrorCode CreateKey(wstring const & type, wstring const & key, __in KAllocator &, __out KString::SPtr &);

        std::wstring ToWString(KString::SPtr const &);
        std::wstring ToWString(KUri const &);
        std::vector<byte> ToByteVector(KBuffer::SPtr const &);

    private:
        Common::ErrorCode FromException(StringLiteral const & tag, ktl::Exception const &) const;
        void OnTraceException(std::wstring const & toTrace) const;
    };
}
