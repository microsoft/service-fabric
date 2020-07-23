// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    struct ConsistencyUnitId 
        : public ServiceModel::ClientServerMessageBody
        , public Common::ISizeEstimator
    {
    public:
        class ReservedIdRange
        {
            //
            // We reserve some GUID values for system Consistency units.
            // Reserved GUIDs are of the form 00000000-0000-0000-0000-0000nnnnnnnn,
            // where nnnnnnnn is a 32-bit unsigned id from one of the ranges
            // reserved below.
            //
            // A non-reserved GUID is any GUID that has any of its first 96 bits
            // non-zero.
            //
        public:
            ReservedIdRange(ULONG low, ULONG high)
                : low_(low), 
                high_(high)
            {
                ASSERT_IF(low > high, "Low cannot be greater than high {0} {1}", low, high);
            }

            __declspec(property(get = get_Count)) ULONG Count;
            ULONG get_Count() const { return high_ - low_ + 1; }             

            bool IsInRange(ULONG id) const
            {
                return id >= low_ && id <= high_;
            }

            Common::Guid CreateReservedGuid(ULONG index) const;
    
        private:
            ULONG low_;
            ULONG high_;
        };
        
        static Common::Global<ReservedIdRange> FMIdRange;
        static Common::Global<ReservedIdRange> NamingIdRange;
        static Common::Global<ReservedIdRange> CMIdRange;
        static Common::Global<ReservedIdRange> RMIdRange;
        static Common::Global<ReservedIdRange> FileStoreIdRange;
        static Common::Global<ReservedIdRange> FaultAnalysisServiceIdRange;
        static Common::Global<ReservedIdRange> BackupRestoreServiceIdRange;
        static Common::Global<ReservedIdRange> UpgradeOrchestrationServiceIdRange;
        static Common::Global<ReservedIdRange> CentralSecretServiceIdRange;
        static Common::Global<ReservedIdRange> EventStoreServiceIdRange;

        static const ConsistencyUnitId Zero;

        struct Hasher
        {
            size_t operator() (ConsistencyUnitId const & key) const 
            { 
                return key.Guid.GetHashCode();
            }
            bool operator() (ConsistencyUnitId const & left, ConsistencyUnitId const & right) const 
            { 
                return (left == right);
            }
        };

    public:

        // Constructors
        ConsistencyUnitId() :
          guid_(CreateNonreservedGuid())
        {
        }

        explicit ConsistencyUnitId(Common::Guid guid) :
          guid_(guid)
        {
        }

        ConsistencyUnitId(ConsistencyUnitId const& other) :
          guid_(other.guid_)
        {
        }

        __declspec (property(get=get_IsInvalid)) bool IsInvalid;
        bool get_IsInvalid() const { return guid_ == Common::Guid::Empty(); }

        _declspec (property(get=get_Guid)) Common::Guid const & Guid;
        Common::Guid const & get_Guid() const { return guid_; }

        static ConsistencyUnitId Invalid() { return ConsistencyUnitId(Common::Guid::Empty()); }

        static bool IsReserved(Common::Guid guid);
        static Common::Guid CreateNonreservedGuid();
        static ConsistencyUnitId CreateReserved(ReservedIdRange const & range, ULONG index)
        {
            return ConsistencyUnitId(range.CreateReservedGuid(index));
        }

        static ConsistencyUnitId CreateFirstReservedId(ReservedIdRange const & range)
        {
            return CreateReserved(range, 0);
        }
        
        ULONG GetReservedId() const;

        bool IsRepairManager() const
        {
            return IsInReservedIdRange(*RMIdRange);
        }

        bool IsNaming() const   
        {
            return IsInReservedIdRange(*NamingIdRange);
        }

        bool IsFailoverManager() const
        {
            return IsInReservedIdRange(*FMIdRange);
        }

        bool IsFileStoreService() const
        {
            return IsInReservedIdRange(*FileStoreIdRange);
        }

        bool IsFaultAnalysisService() const
        {
            return IsInReservedIdRange(*FaultAnalysisServiceIdRange);
        }

        bool IsBackupRestoreService() const
        {
            return IsInReservedIdRange(*BackupRestoreServiceIdRange);
        }

        bool IsClusterManager() const
        {
            return IsInReservedIdRange(*CMIdRange);
        }

        bool IsUpgradeOrchestrationService() const
        {
            return IsInReservedIdRange(*UpgradeOrchestrationServiceIdRange);
        }

        bool IsCentralSecretService() const
        {
            return IsInReservedIdRange(*CentralSecretServiceIdRange);
        }

        bool IsEventStoreService() const
        {
            return IsInReservedIdRange(*EventStoreServiceIdRange);
        }

        // Comparison operators
        bool operator == (ConsistencyUnitId const& other) const
        {
            return (guid_ == other.guid_);
        }

        bool operator != (ConsistencyUnitId const& other) const
        {
            return !(*this == other);
        }

        bool operator < (ConsistencyUnitId const& other) const
        {
            return (guid_ < other.guid_);
        }

        bool operator >= (ConsistencyUnitId const& other) const
        {
            return !(*this < other);
        }

        bool operator > (ConsistencyUnitId const& other) const
        {
            return (other < *this);
        }

        bool operator <= (ConsistencyUnitId const& other) const
        {
            return !(*this > other);
        }

        void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const
        {
            writer.Write("{0}", guid_);
        }

        std::wstring ToString() const;

        FABRIC_FIELDS_01(guid_);

        // guid is initialized with non-zero value, so dynamic size is 0
        BEGIN_DYNAMIC_SIZE_ESTIMATION()
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        bool IsInReservedIdRange(ReservedIdRange const & range) const
        {
            if (!IsReserved(guid_))
            {
                return false;
            }

            ULONG id = GetReservedId();
            return range.IsInRange(id);
        }

        Common::Guid guid_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Reliability::ConsistencyUnitId);
