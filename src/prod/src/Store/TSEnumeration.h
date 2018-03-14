// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    typedef Data::TStore::IStore<KString::SPtr, KBuffer::SPtr> IKvs; 
    typedef Data::KeyValuePair<LONG64, KBuffer::SPtr> IKvsGetEntry;
    typedef Data::IEnumerator<KString::SPtr> IKvsEnumerator; 

    class TSEnumeration 
        : public IStoreBase::EnumerationBase
        , public TSEnumerationBase
    {
        DENY_COPY(TSEnumeration)

    private:
        TSEnumeration(
            Store::PartitionedReplicaId const &,
            std::wstring const & type, 
            std::wstring const & keyPrefix,
            IKvs::SPtr const &,
            IKvsTransaction::SPtr &&,
            IKvsEnumerator::SPtr &&);

    public:

        static std::shared_ptr<TSEnumeration> Create(
            Store::PartitionedReplicaId const &,
            std::wstring const & type, 
            std::wstring const & keyPrefix,
            IKvs::SPtr const &,
            IKvsTransaction::SPtr &&,
            IKvsEnumerator::SPtr &&);

        virtual ~TSEnumeration();

        __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
        std::wstring const & get_TraceId() const { return traceId_; }

    public:
        
        //
        // IStoreBase::EnumerationBase
        //

        virtual Common::ErrorCode MoveNext() override;

        virtual Common::ErrorCode CurrentOperationLSN(__inout _int64 & operationNumber) override;
        virtual Common::ErrorCode CurrentLastModifiedFILETIME(__inout FILETIME & fileTime) override;
        virtual Common::ErrorCode CurrentLastModifiedOnPrimaryFILETIME(__inout FILETIME & fileTime) override;
        virtual Common::ErrorCode CurrentType(__inout std::wstring & buffer) override;
        virtual Common::ErrorCode CurrentKey(__inout std::wstring & buffer) override;
        virtual Common::ErrorCode CurrentValue(__inout std::vector<byte> & buffer) override;
        virtual Common::ErrorCode CurrentValueSize(__inout size_t & size) override;

    protected:

        //
        // TSEnumerationBase
        //

        bool OnInnerMoveNext() override;
        KString::SPtr OnGetCurrentKey() override;

    protected:

        //
        // TSComponent
        //

        Common::StringLiteral const & GetTraceComponent() const override;
        std::wstring const & GetTraceId() const override;

    private:

        Common::ErrorCode InnerMoveNext();
        Common::ErrorCode InnerGetCurrentEntry(__out IKvsGetEntry & result);

        IKvs::SPtr innerStore_;
        IKvsTransaction::SPtr storeTx_;
        IKvsEnumerator::SPtr innerEnum_;
        std::wstring traceId_;
    };
}
