// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStore::Enumeration 
        : public ILocalStore::EnumerationBase
        , public Common::TextTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
        DENY_COPY_CONSTRUCTOR(Enumeration);
    public:
        Enumeration(
            std::shared_ptr<ReplicatedStore::TransactionBase> const &,
            EnumerationSPtr && innerEnumeration,
            Common::ErrorCodeValue::Enum const,
            int enumerationPerfTraceThreshold);
        virtual ~Enumeration();

        virtual Common::ErrorCode MoveNext();
        virtual Common::ErrorCode CurrentOperationLSN(__out _int64 & lsn);
        virtual Common::ErrorCode CurrentLastModifiedFILETIME(__out FILETIME & fileTime);
        virtual Common::ErrorCode CurrentLastModifiedOnPrimaryFILETIME(__out FILETIME & fileTime);
        virtual Common::ErrorCode CurrentType(__out std::wstring & buffer);
        virtual Common::ErrorCode CurrentKey(__out std::wstring & buffer);
        virtual Common::ErrorCode CurrentValue(__out std::vector<byte> & buffer);
        virtual Common::ErrorCode CurrentValueSize(__out size_t & size);

    private:
        typedef std::function<Common::ErrorCode(void)> EnumerationOperationFunc;
        typedef std::pair<std::wstring, Common::TimeSpan> PerfDetailEntry;

        bool IsPerfDetailsEnabled();
        void InitializePerfDetails();
        void InitializePerfDetails(std::wstring const & name);
        void TrackElapsedTime(std::shared_ptr<ReplicatedStore::TransactionBase> const & txSPtr, size_t opIndex);
        void TracePerfDetails(std::shared_ptr<ReplicatedStore::TransactionBase> const & txSPtr);

        Common::ErrorCode CheckAndPerform(EnumerationOperationFunc const &, size_t opIndex);
        Common::ErrorCode CheckReadAccess(__out std::shared_ptr<ReplicatedStore::TransactionBase> &);
        void AbortOnError(std::shared_ptr<ReplicatedStore::TransactionBase> const &, Common::ErrorCode const &);

        std::weak_ptr<ReplicatedStore::TransactionBase> txWPtr_;
        EnumerationSPtr innerEnumeration_;
        Common::ErrorCodeValue::Enum creationError_;

        int enumerationPerfTraceThreshold_;
        std::vector<PerfDetailEntry> perfDetails_;
        Common::Stopwatch stopwatch_;
        size_t itemCount_;
        size_t totalBytes_;
        Common::TimeSpan totalElapsed_;
    };
}
