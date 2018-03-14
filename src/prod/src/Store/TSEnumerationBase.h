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

    class TSEnumerationBase
        : public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
        , public TSComponent
        , public ReplicatedStoreEnumeratorBase
    {
        DENY_COPY(TSEnumerationBase)

    protected:

        using PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>::TraceId;
        using PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>::get_TraceId;

        TSEnumerationBase(
            Store::PartitionedReplicaId const &,
            std::wstring const & type, 
            std::wstring const & keyPrefix,
            bool strictPrefix);

        virtual ~TSEnumerationBase();
            
        std::wstring const & GetTargetType() { return targetType_; }
        std::wstring const & GetTargetKeyPrefix() { return targetKeyPrefix_; }
        std::wstring const & GetCurrentType() { return currentType_; }
        std::wstring const & GetCurrentKey() { return currentKey_; }

    protected:

        //
        // ReplicatedStoreEnumeratorBase
        //

        Common::ErrorCode OnMoveNextBase() override;

    protected:

        virtual bool OnInnerMoveNext() = 0;
        virtual KString::SPtr OnGetCurrentKey() = 0;

    private:
        Common::ErrorCode InnerMoveNext();

        std::wstring targetType_;
        std::wstring targetKeyPrefix_;
        bool strictPrefix_;

        bool isInnerEnumInitialized_;
        std::wstring currentType_;
        std::wstring currentKey_;
    };
}

