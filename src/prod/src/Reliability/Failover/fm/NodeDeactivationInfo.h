// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class NodeDeactivationInfo : public Serialization::FabricSerializable
        {
            DEFAULT_COPY_ASSIGNMENT(NodeDeactivationInfo)

        public:
            NodeDeactivationInfo();

            NodeDeactivationInfo(NodeDeactivationInfo const& other);

            NodeDeactivationInfo(NodeDeactivationInfo && other);

            NodeDeactivationInfo & operator=(NodeDeactivationInfo && other);

            void Initialize();

            FABRIC_FIELDS_06(
                intent_,
                status_,
                batchIds_,
                sequenceNumber_,
                intents_,
                startTime_);

            __declspec (property(get = get_Intent)) NodeDeactivationIntent::Enum Intent;
            NodeDeactivationIntent::Enum get_Intent() const { return intent_; }

            __declspec (property(get = get_Status, put = set_Status)) NodeDeactivationStatus::Enum Status;
            NodeDeactivationStatus::Enum get_Status() const { return status_; }
            void set_Status(NodeDeactivationStatus::Enum value);
            __declspec (property(get = get_BatchIds)) std::vector<std::wstring> const& BatchIds;
            std::vector<std::wstring> const& get_BatchIds() const { return batchIds_; }

            __declspec (property(get = get_SequenceNumber, put = set_SequenceNumber)) int64 SequenceNumber;
            int64 get_SequenceNumber() const { return sequenceNumber_; }

            __declspec (property(get = get_StartTime, put = set_StartTime)) Common::DateTime StartTime;
            Common::DateTime get_StartTime() const { return startTime_; }

            __declspec (property(get = get_Duration)) Common::TimeSpan Duration;
            Common::TimeSpan get_Duration() const;

            __declspec (property(get = get_IsActivated)) bool IsActivated;
            bool get_IsActivated() const { return intent_ == NodeDeactivationIntent::None; }

            __declspec (property(get = get_IsDeactivated)) bool IsDeactivated;
            bool get_IsDeactivated() const { return intent_ != NodeDeactivationIntent::None; }

            __declspec (property(get = get_IsPause)) bool IsPause;
            bool get_IsPause() const { return intent_ == NodeDeactivationIntent::Pause; }

            __declspec (property(get = get_IsRestart)) bool IsRestart;
            bool get_IsRestart() const { return intent_ == NodeDeactivationIntent::Restart; }

            __declspec (property(get = get_IsRemoveData)) bool IsRemoveData;
            bool get_IsRemoveData() const { return intent_ == NodeDeactivationIntent::RemoveData; }

            __declspec (property(get = get_IsRemoveNode)) bool IsRemoveNode;
            bool get_IsRemoveNode() const { return intent_ == NodeDeactivationIntent::RemoveNode; }

            __declspec (property(get = get_IsRemove)) bool IsRemove;
            bool get_IsRemove() const { return (IsRemoveData || IsRemoveNode); }

            __declspec (property(get = get_IsSafetyCheckInProgress)) bool IsSafetyCheckInProgress;
            bool get_IsSafetyCheckInProgress() const { return status_ == NodeDeactivationStatus::DeactivationSafetyCheckInProgress; }

            __declspec (property(get = get_IsSafetyCheckComplete)) bool IsSafetyCheckComplete;
            bool get_IsSafetyCheckComplete() const { return status_ == NodeDeactivationStatus::DeactivationSafetyCheckComplete; }

            __declspec (property(get = get_IsDeactivationComplete)) bool IsDeactivationComplete;
            bool get_IsDeactivationComplete() const { return status_ == NodeDeactivationStatus::DeactivationComplete; }

            __declspec (property(get = get_IsActivationInProgress)) bool IsActivationInProgress;
            bool get_IsActivationInProgress() const { return status_ == NodeDeactivationStatus::ActivationInProgress; }

            __declspec (property(get = get_PendingSafetyChecks)) std::vector<SafetyCheckWrapper> const& PendingSafetyChecks;
            std::vector<SafetyCheckWrapper> const& get_PendingSafetyChecks() const { return pendingSafetyChecks_; }

            void AddBatch(std::wstring const& batchId, NodeDeactivationIntent::Enum intent, int64 sequenceNumber);
            void RemoveBatch(std::wstring const& batchId, int64 sequenceNumber, bool isNodeUp);
            bool ContainsBatch(std::wstring const& batchId) const;

            void UpdateProgressDetails(
                Federation::NodeId nodeId,
                std::map<Federation::NodeId, NodeUpgradeProgress> && pending,
                std::map<Federation::NodeId, std::pair<NodeUpgradeProgress, NodeInfoSPtr>> && ready,
                std::map<Federation::NodeId, NodeUpgradeProgress> && waiting,
                bool isComplete);

            ServiceModel::NodeDeactivationQueryResult GetQueryResult() const;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            std::wstring GetBatchIdsWithIntent();

        private:

            void UpdateSafetyChecks(Federation::NodeId nodeId, std::map<Federation::NodeId, NodeUpgradeProgress> && progressDetails);
            void UpdateSafetyChecks(Federation::NodeId nodeId, std::map<Federation::NodeId, std::pair<NodeUpgradeProgress, NodeInfoSPtr>> && progressDetails);

            void UpdateSafetyCheck(UpgradeSafetyCheckWrapper const& upgradeSafetyCheckWrapper);

            // The effective intent, which is defined as the greater of all the intents.
            NodeDeactivationIntent::Enum intent_;

            // The status corresponding to the effective intent.
            NodeDeactivationStatus::Enum status_;

            // Ideally, we should have a map of batchIds and intents.
            // However, due to backward compatibility, these are kept as separate vectors.
            std::vector<std::wstring> batchIds_;
            std::vector<NodeDeactivationIntent::Enum> intents_;

            int64 sequenceNumber_;

            std::vector<SafetyCheckWrapper> pendingSafetyChecks_;

            Common::DateTime startTime_;
        };
    }
}
