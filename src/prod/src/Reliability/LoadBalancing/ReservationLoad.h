// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "stdafx.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ReservationLoad
        {

        public:
            explicit ReservationLoad();

            ReservationLoad(ReservationLoad && other);

            static std::wstring const FormatHeader;

            __declspec(property(get = get_NodeReservedLoad)) std::map<Federation::NodeId, int64> const& NodeReservedLoad;
            std::map<Federation::NodeId, int64> const& get_NodeReservedLoad() const { return nodeReservedLoad_; }

            __declspec(property(get = get_TotalReservedLoadUsed)) int64 TotalReservedLoadUsed;
             int64 get_TotalReservedLoadUsed() const { return totalReservedLoadUsed_; }

            // add load for metrics -> affects totalReservedLoadUsed_
            void UpdateTotalReservedLoadUsed(int64 load);

            // update on specific node load -> affects nodeReservedLoadUsed_
            void AddNodeReservedLoadForAllApps(Federation::NodeId node, int64 load);
            void DeleteNodeReservedLoadForAllApps(Federation::NodeId node, int64 load);
            
            int64 GetNodeReservedLoadUsed(Federation::NodeId nodeId) const;

            void MergeReservedLoad(ReservationLoad& other);

        private:
            // NodeID to reservation load for all applications on that node
            std::map<Federation::NodeId, int64> nodeReservedLoad_;

            // metric to total reserved load map for all applications, for all nodes
            // Total equal to the sum of all application reserved load (actual load if less than capacity)
            int64 totalReservedLoadUsed_;
        };
    }
}
