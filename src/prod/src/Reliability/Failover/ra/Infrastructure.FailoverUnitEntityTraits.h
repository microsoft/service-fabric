// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            template<>
            struct EntityTraits<FailoverUnit>
            {                
                typedef FailoverUnit DataType;
                typedef Reliability::FailoverUnitId IdType;
                typedef FailoverUnitEntityExecutionContext EntityExecutionContextType;
                typedef HandlerParameters HandlerParametersType;

                static const Storage::Api::RowType::Enum RowType = Storage::Api::RowType::FailoverUnit;

                static void AddEntityIdToMessage(
                    Transport::Message & message, 
                    IdType const & ftId);

                static Infrastructure::EntityMap<FailoverUnit> & GetEntityMap(ReconfigurationAgent & ra);

                static HandlerParametersType CreateHandlerParameters(
                    std::string const & traceId,
                    LockedEntityPtr<DataType> & ft,
                    ReconfigurationAgent & ra,
                    StateMachineActionQueue & actionQueue,
                    MultipleEntityWork const * work,
                    std::wstring const & activityId);

                static EntityExecutionContextType CreateEntityExecutionContext(
                    ReconfigurationAgent & ra,
                    StateMachineActionQueue & queue,
                    UpdateContext & updateContext,
                    IEntityStateBase const * state);

                static bool PerformChecks(JobItemCheck::Enum checks, DataType const * ft);

                static void AssertInvariants(
                    FailoverUnit const & ft,
                    Infrastructure::EntityExecutionContext & context);

                static EntityEntryBaseSPtr Create(
                    IdType const & id,
                    ReconfigurationAgent & ra);

                static EntityEntryBaseSPtr Create(
                    IdType const & id,
                    std::shared_ptr<DataType> && data,
                    ReconfigurationAgent & ra);

                static void Trace(
                    IdType const & id,
                    Federation::NodeInstance const & nodeInstance, 
                    DataType const & data, 
                    std::vector<EntityJobItemTraceInformation> const & traceInfo,
                    Common::ErrorCode const & commitResult, 
                    Diagnostics::ScheduleEntityPerformanceData const & schedulePerfData,
                    Diagnostics::ExecuteEntityJobItemListPerformanceData const & executePerfData,
                    Diagnostics::CommitEntityPerformanceData const & commitPerfData);

                template<typename TBody>
                static bool CheckStaleness(
                    Communication::StalenessCheckType::Enum type,
                    std::wstring const & action,
                    TBody const & body,
                    FailoverUnit const * ft)
                {
                    return CheckStaleness(
                        type,
                        action,
                        BodyTypeTraits<TBody>::GetFailoverUnitDescription(body),
                        BodyTypeTraits<TBody>::GetLocalReplicaDescription(body),
                        ft);
                }

                static bool CheckStaleness(
                    Communication::StalenessCheckType::Enum type,
                    std::wstring const & action,
                    Reliability::FailoverUnitDescription const * incomingFTDesc,
                    Reliability::ReplicaDescription const * incomingLocalReplica,
                    FailoverUnit const * ft);

                template<typename TBody>
                static IdType GetEntityIdFromMessage(TBody const & body)
                {
                    return body.FailoverUnitDescription.FailoverUnitId;
                }

            private:
                class StalenessChecker
                {
                public:
                    static bool CanProcessTcpMessage(
                        std::wstring const & action,
                        Reliability::FailoverUnitDescription const * incomingFTDesc,
                        Reliability::ReplicaDescription const * incomingLocalReplica,
                        FailoverUnit const * ft);

                    static bool CanProcessIpcMessage(
                        std::wstring const & action,
                        Reliability::FailoverUnitDescription const * incomingFTDesc,
                        Reliability::ReplicaDescription const * incomingLocalReplica,
                        FailoverUnit const * ft);

                private:
                    static bool IsTcpMessageStale(
                        std::wstring const & action,
                        Reliability::FailoverUnitDescription const * incomingFTDesc,
                        Reliability::ReplicaDescription const * incomingLocalReplica,
                        FailoverUnit const * ft,
                        bool compareEpochEqualityOnly,
                        bool comparePrimaryEpoch,
                        bool compareReplicaInstance,
                        bool compareCurrentEpoch);

                    static bool IsIpcMessageStale(
                        std::wstring const & action,
                        Reliability::FailoverUnitDescription const * incomingFTDesc,
                        Reliability::ReplicaDescription const * incomingLocalReplica,
                        FailoverUnit const * ft,
                        bool compareEpochEqualityOnly,
                        bool comparePrimaryEpoch);
                    
                    static bool IsIpcMessageStaleBasedOnEpoch(
                        Reliability::Epoch const & ftEpoch,
                        Reliability::Epoch const & incomingEpoch,
                        bool comparePrimaryEpochOnly);                    
                };
            };

            typedef EntityEntry<FailoverUnit> LocalFailoverUnitMapEntry;
            typedef std::shared_ptr<LocalFailoverUnitMapEntry> LocalFailoverUnitMapEntrySPtr;
            typedef LockedEntityPtr<FailoverUnit> LockedFailoverUnitPtr;
            typedef ReadOnlyLockedEntityPtr<FailoverUnit> ReadOnlyLockedFailoverUnitPtr;
        }
    }
}
