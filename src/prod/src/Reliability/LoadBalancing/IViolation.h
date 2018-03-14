// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "LoadEntry.h"
#include "PlacementReplicaSet.h"
#include "LoadBalancingDomainEntry.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class PlacementReplica;
        class PartitionEntry;
        class NodeEntry;
        class ApplicationEntry;
        class IViolation;
        typedef std::unique_ptr<IViolation> IViolationUPtr;

        struct ViolationRelation
        {
        public:
            enum Enum
            {
                Smaller = -1,
                Equal = 0,
                Greater = 1,
            };
        };

        class IViolation
        {
        public:
            virtual size_t GetCount() const = 0;
            virtual bool IsEmpty() const = 0;
            virtual bool operator <= (IViolation const& other) const = 0;
            virtual std::vector<std::wstring> GetViolationListDetail() = 0;
            virtual ViolationRelation::Enum CompareViolation(IViolation const& other) const = 0;
            virtual ~IViolation() = 0 {}
        };

        class ReplicaSetViolation: public IViolation
        {
        public:
            ReplicaSetViolation(PlacementReplicaSet && invalidReplicas)
                : invalidReplicas_(move(invalidReplicas))
            {
            }

            virtual size_t GetCount() const;
            virtual bool IsEmpty() const;
            virtual bool operator<=(IViolation const& other) const { return (*this <= dynamic_cast<ReplicaSetViolation const&>(other));}
            bool operator<=(ReplicaSetViolation const& other) const;
            virtual ViolationRelation::Enum CompareViolation(IViolation const& other) const { return CompareViolation(dynamic_cast<ReplicaSetViolation const&>(other)); }
            ViolationRelation::Enum CompareViolation(ReplicaSetViolation const& other) const;
            virtual std::vector<std::wstring> GetViolationListDetail();
            virtual ~ReplicaSetViolation() {}

        private:
            PlacementReplicaSet invalidReplicas_;
        };

        class PartitionSetViolation: public IViolation
        {
        public:
            PartitionSetViolation(std::set<PartitionEntry const*> && invalidPartitions)
                : invalidPartitions_(move(invalidPartitions))
            {
            }

            virtual size_t GetCount() const;
            virtual bool IsEmpty() const;
            virtual bool operator<=(IViolation const& other) const { return (*this <= dynamic_cast<PartitionSetViolation const&>(other));}
            bool operator<=(PartitionSetViolation const& other) const;
            virtual ViolationRelation::Enum CompareViolation(IViolation const& other) const { return CompareViolation(dynamic_cast<PartitionSetViolation const&>(other)); }
            ViolationRelation::Enum CompareViolation(PartitionSetViolation const& other) const;
            virtual std::vector<std::wstring> GetViolationListDetail();
            virtual ~PartitionSetViolation() {}

        private:
            std::set<PartitionEntry const*> invalidPartitions_;
        };

        class NodeLoadViolation: public IViolation
        {
        public:    
            NodeLoadViolation(int64 totalLoadOverCapacity, 
                std::map<NodeEntry const*, LoadEntry> && nodeLoadOverCapacity,
                LoadBalancingDomainEntry const* globalDomainEntryPointer)
                : totalLoadOverCapacity_(totalLoadOverCapacity), 
                nodeLoadOverCapacity_(move(nodeLoadOverCapacity)),
                globalDomainEntryPointer_(globalDomainEntryPointer)
            {
            }

            virtual size_t GetCount() const;
            virtual bool IsEmpty() const;
            virtual bool operator<=(IViolation const& other) const { return (*this <= dynamic_cast<NodeLoadViolation const&>(other));}
            bool operator<=(NodeLoadViolation const& other) const;
            virtual ViolationRelation::Enum CompareViolation(IViolation const& other) const { return CompareViolation(dynamic_cast<NodeLoadViolation const&>(other)); }
            ViolationRelation::Enum CompareViolation(NodeLoadViolation const& other) const;
            virtual std::vector<std::wstring> GetViolationListDetail();
            virtual ~NodeLoadViolation() {}

        private:
            LoadBalancingDomainEntry const* globalDomainEntryPointer_;
            int64 totalLoadOverCapacity_;
            std::map<NodeEntry const*, LoadEntry> nodeLoadOverCapacity_;
        };

        class ScaleoutCountViolation : public IViolation
        {
        public:
            ScaleoutCountViolation(std::map<ApplicationEntry const*, std::set<NodeEntry const *>> && appInvalidNodes,
                PlacementReplicaSet invalidReplicas)
                : appInvalidNodes_(move(appInvalidNodes)),
                invalidReplicas_(move(invalidReplicas))
            {
            }

            virtual size_t GetCount() const;
            virtual bool IsEmpty() const;
            virtual bool operator<=(IViolation const& other) const { return (*this <= dynamic_cast<ScaleoutCountViolation const&>(other)); }
            bool operator<=(ScaleoutCountViolation const& other) const;
            virtual ViolationRelation::Enum CompareViolation(IViolation const& other) const { return CompareViolation(dynamic_cast<ScaleoutCountViolation const&>(other)); }
            ViolationRelation::Enum CompareViolation(ScaleoutCountViolation const& other) const;
            virtual std::vector<std::wstring> GetViolationListDetail();
            virtual ~ScaleoutCountViolation() {}

        private:
            std::map<ApplicationEntry const*, std::set<NodeEntry const *>> appInvalidNodes_;
            PlacementReplicaSet invalidReplicas_;
        };

    }
}
