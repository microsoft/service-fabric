// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "FailoverUnitMovementType.h"
#include "PLBSchedulerActionType.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class FailoverUnitMovement;
        typedef std::map<Common::Guid, FailoverUnitMovement> FailoverUnitMovementTable;

        class FailoverUnitMovement
        {
        public:

            struct PLBAction
            {
                PLBAction(
                    Federation::NodeId const & sourceNode,
                    Federation::NodeId const & targetNode,
                    FailoverUnitMovementType::Enum action,
                    PLBSchedulerActionType::Enum schedulerAction)
                    : SourceNode(sourceNode),
                    TargetNode(targetNode),
                    Action(action),
                    SchedulerActionType(schedulerAction)
                {
                }

                __declspec (property(get=get_IsAdd)) bool IsAdd;
                bool get_IsAdd() const { return Action == FailoverUnitMovementType::AddPrimary || Action == FailoverUnitMovementType::AddSecondary || Action == FailoverUnitMovementType::AddInstance; }

                __declspec (property(get = get_HasSourceNode)) bool HasSourceNode;
                bool get_HasSourceNode() const
                {
                    return Action == FailoverUnitMovementType::SwapPrimarySecondary ||
                        Action == FailoverUnitMovementType::MoveSecondary ||
                        Action == FailoverUnitMovementType::MoveInstance ||
                        Action == FailoverUnitMovementType::MovePrimary ||
                        Action == FailoverUnitMovementType::RequestedPlacementNotPossible ||
                        Action == FailoverUnitMovementType::DropPrimary ||
                        Action == FailoverUnitMovementType::DropSecondary ||
                        Action == FailoverUnitMovementType::DropInstance;
                }

                __declspec (property(get = get_HasTargetNode)) bool HasTargetNode;
                bool get_HasTargetNode() const
                {
                    return Action == FailoverUnitMovementType::SwapPrimarySecondary ||
                        Action == FailoverUnitMovementType::MoveSecondary ||
                        Action == FailoverUnitMovementType::MoveInstance ||
                        Action == FailoverUnitMovementType::MovePrimary ||
                        Action == FailoverUnitMovementType::AddPrimary ||
                        Action == FailoverUnitMovementType::AddSecondary ||
                        Action == FailoverUnitMovementType::AddInstance ||
                        Action == FailoverUnitMovementType::PromoteSecondary;
                }

                bool operator == (PLBAction const & other ) const
                {
                    return (SourceNode == other.SourceNode && TargetNode == other.TargetNode && Action == other.Action);
                }

                bool operator != (PLBAction const & other ) const
                {
                    return !(*this == other);
                }

                void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
                {
                    writer.Write("{0}:{1}=>{2}", Action, SourceNode, TargetNode);
                }

                Federation::NodeId SourceNode;
                Federation::NodeId TargetNode;
                FailoverUnitMovementType::Enum Action;
                PLBSchedulerActionType::Enum SchedulerActionType;
            };

            FailoverUnitMovement(
                Common::Guid failoverUnitId,
                std::wstring && serviceName,
                bool isStateful,
                int64 version,
                bool isFuInTransition,
                std::vector<PLBAction> && actions);

            FailoverUnitMovement(FailoverUnitMovement && other);

            FailoverUnitMovement & operator = (FailoverUnitMovement && other);

            FailoverUnitMovement(FailoverUnitMovement const& other);

            bool operator == (FailoverUnitMovement const& other) const;

            bool operator != (FailoverUnitMovement const& other) const;

            __declspec (property(get=get_FailoverUnitId)) Common::Guid FailoverUnitId;
            Common::Guid get_FailoverUnitId() const { return failoverUnitId_; }

            __declspec (property(get=get_IsStateful)) bool IsStateful;
            bool get_IsStateful() const { return isStateful_; }

            __declspec (property(get=get_ServiceName)) std::wstring const& ServiceName;
            std::wstring const& get_ServiceName() const { return serviceName_; }

            __declspec (property(get=get_Version)) int64 Version;
            int64 get_Version() const { return version_; }

            __declspec (property(get = get_IsFuInTransition)) bool IsFuInTransition;
            bool get_IsFuInTransition() const { return isFuInTransition_; }

            __declspec (property(get=get_Actions)) std::vector<PLBAction> const& Actions;
            std::vector<PLBAction> const& get_Actions() const { return actions_; }
            std::vector<PLBAction> & get_Actions() { return actions_; }

            void UpdateActions(std::vector<PLBAction> && actions);

            bool ContainCreation();

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        private:

            Common::Guid failoverUnitId_;
            std::wstring serviceName_;
            bool isStateful_;
            int64 version_;
            bool isFuInTransition_;
            std::vector<PLBAction> actions_;
        };
    }
}
