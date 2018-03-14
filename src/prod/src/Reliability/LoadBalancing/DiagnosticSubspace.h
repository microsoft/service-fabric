// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "IConstraint.h"
#include "Common/StringUtility.h"
#include "PlacementReplica.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class DiagnosticSubspace
        {
            DENY_COPY(DiagnosticSubspace);
        public:
            DiagnosticSubspace(ISubspaceUPtr && subspace, size_t count, std::wstring && nodeDetail)
            : subspace_(move(subspace)),
            count_(count),
            nodeDetail_(move(nodeDetail))
            {}

            DiagnosticSubspace(DiagnosticSubspace && other)
            : subspace_(move(other.subspace_)),
            count_(other.count_),
            nodeDetail_(move(other.nodeDetail_))
            {}

            static void RefreshSubspacesDiagnostics(std::vector<DiagnosticSubspace> & subspaces);

            __declspec (property(get=get_Subspace)) ISubspaceUPtr const& Subspace;
            ISubspaceUPtr const& get_Subspace() const { return subspace_; }

            __declspec (property(get=get_Count, put=set_Count)) size_t Count;
            size_t get_Count() const { return count_; }
            void set_Count(size_t count) { count_ = count; }

            __declspec (property(get=get_NodeDetail, put=set_NodeDetail)) std::wstring & NodeDetail;
            std::wstring const& get_NodeDetail() const { return nodeDetail_; }
            void set_NodeDetail(std::wstring && nodeDetail) { nodeDetail_ = move(nodeDetail); }

        private:
            ISubspaceUPtr subspace_;
            size_t count_;
            std::wstring nodeDetail_;
        };

        class ConstraintViolation
        {

        public:
            ConstraintViolation(PlacementReplica const* replica, IConstraint::Enum constraint, int priority)
            : replica_(replica), constraint_(constraint), priority_(priority)
            {}

            __declspec (property(get=get_Replica)) PlacementReplica const* Replica;
            PlacementReplica const* get_Replica() const { return replica_; }

            __declspec (property(get=get_Constraint)) IConstraint::Enum Constraint;
            IConstraint::Enum get_Constraint() const { return constraint_; }

            __declspec (property(get=get_Priority)) int Priority;
            int get_Priority() const { return priority_; }

            __declspec (property(get=get_ViolationHash)) std::wstring ViolationHash;
            std::wstring get_ViolationHash() const { return replica_->ReplicaHash(true) + Common::StringUtility::ToWString(constraint_); }

        private:
            PlacementReplica const* replica_;
            IConstraint::Enum constraint_;
            int priority_;
        };
    }
}
