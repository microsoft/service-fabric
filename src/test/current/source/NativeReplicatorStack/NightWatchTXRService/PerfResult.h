// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace NightWatchTXRService
{
    class PerfResult final
        : public KObject<PerfResult>
        , public KShared<PerfResult>
        , public Common::IFabricJsonSerializable
    {
        K_FORCE_SHARED(PerfResult)

    public:

        static PerfResult::SPtr Create(
            __in ULONG numberOfOperations,
            __in __int64 durationSeconds,
            __in double operationsPerSecond,
            __in double operationsPerMillisecond,
            __in double throughput,
            __in double averageLatency,
            __in ULONG numberOfWriters,
            __in int64 startTicks_,
            __in int64 endTicks_,
            __in std::wstring codePackageDirectory_,
            __in TestStatus::Enum const & status,
            __in_opt Data::Utilities::SharedException const * exception,
            __in KAllocator & allocator);

        static PerfResult::SPtr Create(
            __in_opt Data::Utilities::SharedException const * exception,
            __in KAllocator & allocator);

        PerfResult(
            __in ULONG numberOfOperations,
            __in __int64 durationSeconds,
            __in double operationsPerSecond,
            __in double operationsPerMillisecond,
            __in double throughput,
            __in double averagelatency,
            __in ULONG numberOfWriters,
            __in int64 startTicks_,
            __in int64 endTicks_,
            __in std::wstring codePackageDirectory_,
            __in TestStatus::Enum const & status,
            __in_opt Data::Utilities::SharedException const * exception);

        __declspec(property(get = get_numberOfOperations)) ULONG NumberfOfOperations;
        ULONG get_numberOfOperations() const
        {
            return numberOfOperations_;
        }

        __declspec(property(get = get_durationSeconds)) __int64 DurationSeconds;
        __int64 get_durationSeconds() const
        {
            return durationSeconds_;
        }

        __declspec(property(get = get_operationsPerSecond)) double OperationsPerSecond;
        double get_operationsPerSecond() const
        {
            return operationsPerSecond_;
        }

        __declspec(property(get = get_operationsPerMillisecond)) double OperationsPerMillisecond;
        double get_operationsPerMilliseconds() const
        {
            return operationsPerMillisecond_;
        }

        __declspec(property(get = get_throughput)) double Throughput;
        double get_throughput() const
        {
            return throughput_;
        }

        __declspec(property(get = get_averageLatency)) double AverageLatency;
        double get_averageLatency() const
        {
            return averageLatency_;
        }

        __declspec(property(get = get_numberOfWriters)) ULONG NumberOfWriters;
        ULONG get_numberOfWriters() const
        {
            return numberOfWriters_;
        }

        __declspec(property(get = get_startTicks)) int64 StartTicks;
        int64 get_startTicks() const
        {
            return startTicks_;
        }

        __declspec(property(get = get_endTicks)) int64 EndTicks;
        int64 get_endTicks() const
        {
            return endTicks_;
        }

        __declspec(property(get = get_codePackageDirectory)) std::wstring CodePackageDirectory;
        std::wstring get_codePackageDirectory() const
        {
            return codePackageDirectory_;
        }

        _declspec(property(get = get_status)) TestStatus::Enum Status;
        TestStatus::Enum get_status() const
        {
            return status_;
        }

        __declspec(property(get = get_exception)) Data::Utilities::SharedException::CSPtr Exception;
        Data::Utilities::SharedException::CSPtr get_exception() const
        {
            return exception_;
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
        {
            w.Write(ToString());
        }

        std::wstring ToString() const
        {
            return Common::wformatString(
                "NumberOfOperations = '{0}', DurationSeconds = '{1}', OperationsPerSecond = '{2}', OperationsPerMilliseconds = '{3}', Throughput = '{4}', AverageLatency = '{5}', NumberOfWriters = '{6}', StartTicks = '{7}', EndTicks = '{8}', CodePackageDirectory = '{9}', Status = '{10}'",
                numberOfOperations_,
                durationSeconds_,
                operationsPerSecond_,
                operationsPerMillisecond_,
                throughput_,
                averageLatency_,
                numberOfWriters_,
                startTicks_,
                endTicks_,
                codePackageDirectory_,
                TestStatus::ToString(status_));
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"NumberOfOperations", numberOfOperations_)
            SERIALIZABLE_PROPERTY(L"DurationSeconds", durationSeconds_)
            SERIALIZABLE_PROPERTY(L"OperationsPerSecond", operationsPerSecond_)
            SERIALIZABLE_PROPERTY(L"OperationsPerMillisecond", operationsPerMillisecond_)
            SERIALIZABLE_PROPERTY(L"Throughput", throughput_)
            SERIALIZABLE_PROPERTY(L"AverageLatency", averageLatency_)
            SERIALIZABLE_PROPERTY(L"NumberOfWriters", numberOfWriters_)
            SERIALIZABLE_PROPERTY(L"StartTicks", startTicks_)
            SERIALIZABLE_PROPERTY(L"EndTicks", endTicks_)
            SERIALIZABLE_PROPERTY(L"CodePackageDirectory", codePackageDirectory_)
            SERIALIZABLE_PROPERTY_ENUM(L"Status", status_)
            END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        ULONG numberOfOperations_;
        __int64 durationSeconds_;
        double operationsPerSecond_;
        double operationsPerMillisecond_;
        double throughput_;
        double averageLatency_;
        ULONG numberOfWriters_;
        int64 startTicks_;
        int64 endTicks_;
        std::wstring codePackageDirectory_;
        TestStatus::Enum status_;

        const Data::Utilities::SharedException::CSPtr exception_;
    };
}
