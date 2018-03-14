// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {

        // Helper methods to convert KTL based strings to StringLiteral, in order to do tracing
        __inline static Common::StringLiteral ToStringLiteral(KStringA const & string)
        {
             return Common::StringLiteral(
                (char *)string,
                (char *)string + string.Length());
        }

        __inline static Common::WStringLiteral ToStringLiteral(KString const & string)
        {
             return Common::WStringLiteral(
                (wchar_t *)string,
                (wchar_t *)string + string.Length());
        }

        __inline static Common::StringLiteral ToStringLiteral(KStringViewA const & string)
        {
             return Common::StringLiteral(
                (char *)string,
                (char *)string + string.Length());
        }

        __inline static Common::WStringLiteral ToStringLiteral(KStringView const & string)
        {
             return Common::WStringLiteral(
                (wchar_t *)string,
                (wchar_t *)string + string.Length());
        }

        __inline static Common::WStringLiteral ToStringLiteral(KUriView const & string)
        {
            return ToStringLiteral(string.Get(KUriView::eRaw));
        }

        __inline static Common::WStringLiteral ToStringLiteral(std::wstring const & string)
        {
            return Common::WStringLiteral(
                string.c_str(),
                string.c_str() + string.length());
        }

        __inline static Common::StringLiteral ToStringLiteral(std::string const & string)
        {
            return Common::StringLiteral(
                string.c_str(),
                string.c_str() + string.length());
        }

        //
        // Class that encapsulates a partitionid:replicaid pair.
        // Any class that needs to trace can do so by having a handle to this object and using the tuple to identify the replica and partition
        //
        class PartitionedReplicaId 
            : public KObject<PartitionedReplicaId>
            , public KShared<PartitionedReplicaId>
        {
            K_FORCE_SHARED(PartitionedReplicaId)

        public:

            static PartitionedReplicaId::SPtr Create(
                __in KGuid const & partitionId,
                __in ::FABRIC_REPLICA_ID replicaId,
                __in KAllocator & allocator);

            __declspec(property(get = get_TraceId)) Common::StringLiteral TraceId;
            Common::StringLiteral get_TraceId() const 
            { 
                return ToStringLiteral(*traceId_);
            }

            __declspec(property(get = get_PartitionId)) KGuid const & PartitionId;
            KGuid const & get_PartitionId() const 
            { 
                return partitionId_; 
            }

            __declspec(property(get = get_TracePartitionId)) Common::Guid const & TracePartitionId;
            Common::Guid const & get_TracePartitionId() const 
            { 
                return tracePartitionId_; 
            }

            __declspec(property(get = get_ReplicaId)) ::FABRIC_REPLICA_ID const & ReplicaId;
            ::FABRIC_REPLICA_ID const & get_ReplicaId() const 
            { 
                return replicaId_; 
            }

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
            void FillEventData(Common::TraceEventContext & context) const;

        private:

            PartitionedReplicaId(
                __in KGuid const & partitionId,
                __in::FABRIC_REPLICA_ID replicaId);

            KStringA::SPtr traceId_;
            KGuid partitionId_;
            ::FABRIC_REPLICA_ID replicaId_;
            Common::Guid tracePartitionId_;
        };
    }
}
