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
            class TestEntity : public Serialization::FabricSerializable, public Infrastructure::IEntityStateBase
            {
            public:
                TestEntity() : 
                    PersistedData(0),
                    InMemoryData(0)
                {
                }

                TestEntity(std::wstring const & key, int persistedData, int inMemoryData) :
                    key_(key),
                    PersistedData(persistedData),
                    InMemoryData(inMemoryData)
                {
                }

                __declspec(property(get = get_Key)) std::wstring const & Key;
                std::wstring const & get_Key() const
                {
                    return key_;
                }

                static std::string AddField(Common::TraceEvent & , std::string const & )
                {
                    return "";
                }

                void FillEventData(Common::TraceEventContext & ) const
                {
                }

                void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const
                {
                }

                std::wstring ToString() const
                {
                    return L"";
                }

                void WriteToEtw(uint16) const
                {
                }

                int PersistedData;
                int InMemoryData;

                FABRIC_FIELDS_02(key_, PersistedData);
            private:
                std::wstring key_;
            };

            template<>
            struct EntityTraits<TestEntity>
            {
                typedef TestEntity DataType;
                typedef std::wstring IdType;
                typedef HandlerParametersT<TestEntity> HandlerParametersType;
                typedef Infrastructure::EntityExecutionContext EntityExecutionContextType;
                typedef EntityMap<TestEntity> EntityMapType;

                static const Storage::Api::RowType::Enum RowType = Storage::Api::RowType::Test;

                static void AddEntityIdToMessage(
                    Transport::Message &,
                    IdType const &)
                {
                }

                static EntityMapType & GetEntityMap(ReconfigurationAgent & ra)
                {
                    // TODO: Make the ut context the root of the RA 
                    // Then this code can dynamic cast the root, get the ut context and add the other thing
                    auto & utContext = ReconfigurationAgentComponent::ReliabilityUnitTest::UnitTestContext::GetUnitTestContext(ra);
                    return utContext.ContextMap.Get<EntityMapType>(ReliabilityUnitTest::TestContextMap::TestEntityMapKey);
                }

                static HandlerParametersType CreateHandlerParameters(
                    std::string const & traceId,
                    LockedEntityPtr<DataType> & ft,
                    ReconfigurationAgent & ra,
                    StateMachineActionQueue & actionQueue,
                    MultipleEntityWork const * work,
                    std::wstring const & activityId)
                {
                    return HandlerParametersType(traceId, ft, ra, actionQueue, work, activityId);
                }

                static EntityExecutionContextType CreateEntityExecutionContext(
                    ReconfigurationAgent & ra,
                    StateMachineActionQueue & queue,
                    UpdateContext & updateContext,
                    IEntityStateBase const * state)
                {
                    return Infrastructure::EntityExecutionContext::Create(ra, queue, updateContext, state);
                }

                static bool PerformChecks(JobItemCheck::Enum checks, DataType const * e)
                {
                    if (checks == JobItemCheck::FTIsNotNull && e == nullptr)
                    {
                        return false;
                    }

                    return true;
                }

                static void AssertInvariants(
                    DataType const & ft,
                    Infrastructure::EntityExecutionContext& ) 
                {
                    UNREFERENCED_PARAMETER(ft);
                }

                static void Trace(
                    IdType const & ,
                    Federation::NodeInstance const & ,
                    DataType const & ,
                    std::vector<EntityJobItemTraceInformation> const &,
                    Common::ErrorCode const & ,
                    Diagnostics::ScheduleEntityPerformanceData const & ,
                    Diagnostics::ExecuteEntityJobItemListPerformanceData const & ,
                    Diagnostics::CommitEntityPerformanceData const & )
                {
                }

                static EntityEntryBaseSPtr Create(IdType const & id, ReconfigurationAgent & ra)
                {
                    return std::make_shared<EntityEntry<TestEntity>>(id, ra.LfumStore);
                }
            };

            struct TestJobItemContext
            {
                static const bool HasCorrelatedTrace = false;

                TestJobItemContext() :
                    InMemoryState(0),
                    PersistedState(0)
                {
                }

                int InMemoryState;
                int PersistedState;
            };

            typedef CommitDescription<TestEntity> TestEntityCommitDescription;
            typedef EntityMap<TestEntity> TestEntityMap;
            typedef std::shared_ptr<TestEntityMap> TestEntityMapSPtr;
            typedef std::shared_ptr<TestEntity> TestEntitySPtr;
            typedef EntityEntry<TestEntity> TestEntityEntry;
            typedef std::shared_ptr<TestEntityEntry> TestEntityEntrySPtr;
            typedef EntityJobItem<TestEntity, TestJobItemContext> TestJobItem;
            typedef std::shared_ptr<TestJobItem> TestJobItemSPtr;
            typedef LockedEntityPtr<TestEntity> LockedTestEntityPtr;
            typedef ReadOnlyLockedEntityPtr<TestEntity> ReadOnlyLockedTestEntityPtr;
            typedef HandlerParametersT<TestEntity> TestEntityHandlerParameters;

            inline TestEntityEntrySPtr CreateTestEntityEntry(std::wstring const & id, ReconfigurationAgent & ra)
            {
                return std::make_shared<TestEntityEntry>(id, ra.LfumStore);
            }

            inline TestEntityEntrySPtr CreateTestEntityEntry(std::wstring const & id, Storage::Api::IKeyValueStoreSPtr const & store)
            {
                return std::make_shared<TestEntityEntry>(id, store);
            }
        }
    }
}



