// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        class RepairTaskHistory
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
            , public Common::TextTraceComponent<Common::TraceTaskCodes::RepairManager>
        {
            DEFAULT_COPY_CONSTRUCTOR(RepairTaskHistory)
            DEFAULT_COPY_ASSIGNMENT(RepairTaskHistory)
        public:
            RepairTaskHistory();

            RepairTaskHistory(RepairTaskHistory &&);

            // TODO add for all if needed
            __declspec(property(get = get_PreparingHealthCheckStartTimestamp)) Common::DateTime const & PreparingHealthCheckStartTimestamp;
            Common::DateTime const & get_PreparingHealthCheckStartTimestamp() const { return preparingHealthCheckStartUtcTimestamp_; }

            __declspec(property(get = get_PreparingHealthCheckEndTimestamp)) Common::DateTime const & PreparingHealthCheckEndTimestamp;
            Common::DateTime const & get_PreparingHealthCheckEndTimestamp() const { return preparingHealthCheckEndUtcTimestamp_; }

            __declspec(property(get = get_RestoringHealthCheckStartTimestamp)) Common::DateTime const & RestoringHealthCheckStartTimestamp;
            Common::DateTime const & get_RestoringHealthCheckStartTimestamp() const { return restoringHealthCheckStartUtcTimestamp_; }

            __declspec(property(get = get_RestoringHealthCheckEndTimestamp)) Common::DateTime const & RestoringHealthCheckEndTimestamp;
            Common::DateTime const & get_RestoringHealthCheckEndTimestamp() const { return restoringHealthCheckEndUtcTimestamp_; }

            __declspec(property(get=get_CompletedUtcTimestamp)) Common::DateTime const & CompletedUtcTimestamp;
            Common::DateTime const & get_CompletedUtcTimestamp() const { return completedUtcTimestamp_; }

            Common::ErrorCode FromPublicApi(FABRIC_REPAIR_TASK_HISTORY const &);
            void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_REPAIR_TASK_HISTORY &) const;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            void Initialize(RepairTaskState::Enum const & initialState);
            void SetTimestamps(RepairTaskState::Enum const & stateMask);

            void SetPreparingHealthCheckStartTimestamp(Common::DateTime const & value);
            void SetPreparingHealthCheckEndTimestamp(Common::DateTime const & value);
            void SetRestoringHealthCheckStartTimestamp(Common::DateTime const & value);
            void SetRestoringHealthCheckEndTimestamp(Common::DateTime const & value);

            FABRIC_FIELDS_11(createdUtcTimestamp_, claimedUtcTimestamp_, preparingUtcTimestamp_, approvedUtcTimestamp_, executingUtcTimestamp_, restoringUtcTimestamp_, completedUtcTimestamp_, preparingHealthCheckStartUtcTimestamp_, preparingHealthCheckEndUtcTimestamp_, restoringHealthCheckStartUtcTimestamp_, restoringHealthCheckEndUtcTimestamp_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::CreatedUtcTimestamp, createdUtcTimestamp_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ClaimedUtcTimestamp, claimedUtcTimestamp_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::PreparingUtcTimestamp, preparingUtcTimestamp_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApprovedUtcTimestamp, approvedUtcTimestamp_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ExecutingUtcTimestamp, executingUtcTimestamp_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::RestoringUtcTimestamp, restoringUtcTimestamp_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::CompletedUtcTimestamp, completedUtcTimestamp_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::PreparingHealthCheckStartUtcTimestamp, preparingHealthCheckStartUtcTimestamp_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::PreparingHealthCheckEndUtcTimestamp, preparingHealthCheckEndUtcTimestamp_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::RestoringHealthCheckStartUtcTimestamp, restoringHealthCheckStartUtcTimestamp_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::RestoringHealthCheckEndUtcTimestamp, restoringHealthCheckEndUtcTimestamp_)                
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:

            static bool TrySetTimestamp(
                RepairTaskState::Enum const & stateMask,
                RepairTaskState::Enum const & matchState,
                Common::DateTime & target,
                Common::DateTime const & value);
            static bool TrySetHealthCheckTimestamp(Common::DateTime & target, Common::DateTime const & value);

            Common::DateTime createdUtcTimestamp_;
            Common::DateTime claimedUtcTimestamp_;
            Common::DateTime preparingUtcTimestamp_;
            Common::DateTime approvedUtcTimestamp_;
            Common::DateTime executingUtcTimestamp_;
            Common::DateTime restoringUtcTimestamp_;
            Common::DateTime completedUtcTimestamp_;
            Common::DateTime preparingHealthCheckStartUtcTimestamp_;
            Common::DateTime preparingHealthCheckEndUtcTimestamp_;
            Common::DateTime restoringHealthCheckStartUtcTimestamp_;
            Common::DateTime restoringHealthCheckEndUtcTimestamp_;
            
        };
    }
}
