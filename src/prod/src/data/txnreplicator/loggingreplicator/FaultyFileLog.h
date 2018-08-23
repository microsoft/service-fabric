// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        // Test wrapper around FileLog
        // Used to override ILogicalLog APIs during testing (ex. PhysicalLogWriter.Test.cpp)
        // Included in the product solution file for use in FabricTest
        //
        // On Windows, this uses the Win32 based file APIs
        // On linux, we use the ktl based sparse log
        class FaultyFileLog
#ifdef PLATFORM_UNIX
            : public Log::FileLogicalLog
#else
            : public FileLog
#endif
        {
            K_FORCE_SHARED(FaultyFileLog);

        public:
            static NTSTATUS Create(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in KAllocator& allocator,
                __out FaultyFileLog::SPtr& fileLogicalLog);

            //
            // Milliseconds to delay when appending to appending to the logical log
            //
            __declspec(property(get = get_AppendAsyncDelay, put = set_AppendAsyncDelay)) ULONG AppendAsyncDelayInMs;
            ULONG get_AppendAsyncDelay() const
            {
                return appendAsyncDelayInMs_;
            };
            void set_AppendAsyncDelay(ULONG value)
            {
                appendAsyncDelayInMs_ = value;
            }

            //
            // Milliseconds to delay when flushing with a marker
            //
            __declspec(property(get = get_FlushWithMarkerAsyncDelay, put = set_FlushWithMarkerAsyncDelay)) ULONG FlushWithMarkerAsyncDelayInMs;
            ULONG get_FlushWithMarkerAsyncDelay() const
            {
                return flushWithMarkerAsyncDelayInMs_;
            };
            void set_FlushWithMarkerAsyncDelay(ULONG value)
            {
                flushWithMarkerAsyncDelayInMs_ = value;
            }

            //
            // Determines if an exception status code should be returned in AppendAsync.
            // The exception type can be set through the ReturnedExceptionStatusCode property
            //
            __declspec(property(get = get_ThrowExceptionInAppendAsync, put = set_ThrowExceptionInAppendAsync)) bool ThrowExceptionInAppendAsync;
            bool get_ThrowExceptionInAppendAsync() const
            {
                return throwExceptionInAppendAsync_;
            };
            void set_ThrowExceptionInAppendAsync(bool value)
            {
                throwExceptionInAppendAsync_ = value;
            }

            //
            // Determines if an exception status code should be returned in FlushWithMarkerAsync.
            // The exception type can be set through the ReturnedExceptionStatusCode property
            //
            __declspec(property(get = get_InvalidFlushWithMarkerAsync, put = set_InvalidFlushWithMarkerAsync)) bool ThrowExceptionInFlushWithMarkerAsync;
            bool get_InvalidFlushWithMarkerAsync() const
            {
                return throwExceptionInFlushWithMarkerAsync_;
            };
            void set_InvalidFlushWithMarkerAsync(bool value)
            {
                throwExceptionInFlushWithMarkerAsync_ = value;
            }

            //
            // The exception status code to return from ILogicalLog APIs
            //
            __declspec(property(get = get_ExceptionStatusCode, put = set_ExceptionStatusCode)) NTSTATUS ReturnedExceptionStatusCode;
            NTSTATUS get_ExceptionStatusCode() const
            {
                return testExceptionStatusCode_;
            };
            void set_ExceptionStatusCode(NTSTATUS value)
            {
                testExceptionStatusCode_ = value;
            }

            ktl::Awaitable<NTSTATUS> AppendAsync(
                __in KBuffer const& buffer,
                __in LONG offsetIntoBuffer,
                __in ULONG count,
                __in ktl::CancellationToken const& cancellationToken
            ) override;

            ktl::Awaitable<NTSTATUS> FlushWithMarkerAsync(__in ktl::CancellationToken const& cancellationToken) override;

            ktl::Awaitable<NTSTATUS> CloseAsync(__in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override
            {
                co_return co_await __super::CloseAsync(cancellationToken);
            }
        protected:

            FaultyFileLog(
                __in Data::Utilities::PartitionedReplicaId const & traceId);

            ULONG appendAsyncDelayInMs_;
            ULONG flushWithMarkerAsyncDelayInMs_;
            bool throwExceptionInAppendAsync_;
            bool throwExceptionInFlushWithMarkerAsync_;
            NTSTATUS testExceptionStatusCode_;
        };
    }
};
