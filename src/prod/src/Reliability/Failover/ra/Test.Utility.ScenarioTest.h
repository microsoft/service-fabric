// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "RATestPointers.h"

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            class ScenarioTest
            {
                DENY_COPY(ScenarioTest);
            public:               
                __declspec(property(get = get_StateItemHelpers)) StateItemHelper::StateItemHelperCollection & StateItemHelpers;
                StateItemHelper::StateItemHelperCollection & get_StateItemHelpers() { return *stateItemHelper_; }

                __declspec(property(get = get_RA)) ReconfigurationAgent & RA;
                ReconfigurationAgent & get_RA() { return utContext_->RA; }

                __declspec(property(get = get_UTContext)) UnitTestContext & UTContext;
                UnitTestContext & get_UTContext() { return *utContext_; }

                __declspec(property(get = get_ReadContext)) StateManagement::MultipleReadContextContainer& ReadContext;
                StateManagement::MultipleReadContextContainer & get_ReadContext() { return readContext_; }

                __declspec(property(get = get_GenerationStateHelper)) StateItemHelper::GenerationStateHelper & GenerationStateHelper;
                StateItemHelper::GenerationStateHelper & get_GenerationStateHelper() { return stateItemHelper_->GenerationHelper; }

                __declspec(property(get = get_HostingHelper)) StateItemHelper::HostingStateHelper & HostingHelper;
                StateItemHelper::HostingStateHelper & get_HostingHelper() { return stateItemHelper_->HostingHelper; }

                __declspec(property(get = get_EntityExecutionContextHelperObj)) EntityExecutionContextTestUtility & EntityExecutionContextTestUtilityObj;
                EntityExecutionContextTestUtility & get_EntityExecutionContextHelperObj() { return entityExecutionContextTestUtility_; }

                // Add a FTContext
                // An FTContext is used for providing static information about an FT so that it does not need to be specified as strings
                void AddFTContext(std::wstring const & ftShortName, StateManagement::SingleFTReadContext const & ftContext);

                FailoverUnit & GetFT(std::wstring const & ftShortName);

                StateManagement::SingleFTReadContext const & GetSingleFTContext(std::wstring const & ftShortName);

                Infrastructure::EntityEntryBaseList GetFailoverUnitEntries(initializer_list<wstring> const & shortNames);

                // Validate a specific FT
                void ValidateFT(std::wstring const & ftShortName, std::wstring const & expectedFT);

                // Validate not present in LFUM
                void ValidateFTNotPresent(std::wstring const & ftShortName);

                // Validate FT is null
                void ValidateFTIsNull(std::wstring const & ftShortName);

                void ValidateFTRetryableErrorState(std::wstring const & shortName, RetryableErrorStateName::Enum stateName, int expectedFailureCount);

                void ValidateFTSTRRetryableErrorState(std::wstring const & shortName, RetryableErrorStateName::Enum stateName, int expectedFailureCount);

                void DumpFT(std::wstring const & ftShortName);

                void SetFTRetryableErrorState(std::wstring const & shortName, RetryableErrorStateName::Enum stateName);

                template<typename TContext>
                void ValidateJobItemHandlerReturnsFalse(
                    std::wstring const & ftShortName,
                    std::wstring const & initialState,
                    typename Infrastructure::EntityJobItem<FailoverUnit, TContext>::TryProcessFunctionPtr handler,
                    TContext & context)
                {
                    AddFT(ftShortName, initialState);

                    ValidateJobItemHandlerReturnsFalse<TContext>(ftShortName, handler, context);
                }

                template<typename TContext>
                void ExecuteJobItemHandlerWithInitialState(
                    std::wstring const & ftShortName,
                    std::wstring const & initialState,
                    typename Infrastructure::EntityJobItem<FailoverUnit, TContext>::TryProcessFunctionPtr handler,
                    TContext & context)
                {
                    AddFT(ftShortName, initialState);

                    ExecuteJobItemHandlerWithContext<TContext>(ftShortName, handler, context, true);
                }

                template<typename TContext>
                void ValidateJobItemHandlerReturnsFalse(
                    std::wstring const & ftShortName,
                    typename Infrastructure::EntityJobItem<FailoverUnit, TContext>::TryProcessFunctionPtr handler,
                    TContext & context)
                {
                    auto entry = GetLFUMEntry(ftShortName);
                    Verify::IsTrue(entry != nullptr, L"Entry cannot be null");

                    StateMachineActionQueue queue;
                    LockedFailoverUnitPtr ft(entry->Test_CreateLock());

                    HandlerParameters parameters("a", ft, RA, queue, nullptr, L"A");

                    bool rv = handler(parameters, context);
                    Verify::IsTrue(!rv, L"Expected handler to return false");

                    Verify::IsTrue(queue.IsEmpty, L"Expected queue to be empty");

                    queue.AbandonAllActions(RA);
                }

                template<typename TContext>
                void ExecuteJobItemHandler(
                    std::wstring const & ftShortName,
                    typename Infrastructure::EntityJobItem<FailoverUnit, TContext>::TryProcessFunctionPtr handler)
                {
                    ExecuteJobItemHandler<TContext>(ftShortName, handler, true);
                }

                template<typename TContext>
                void ExecuteJobItemHandler(
                    std::wstring const & ftShortName,
                    typename Infrastructure::EntityJobItem<FailoverUnit, TContext>::TryProcessFunctionPtr handler,
                    bool callEnableUpdate)
                {
                    TContext context;
                    ExecuteJobItemHandlerWithContext<TContext>(ftShortName, handler, context, callEnableUpdate);
                }

                template<typename TContext>
                void ExecuteJobItemHandlerWithContext(
                    std::wstring const & ftShortName,
                    typename Infrastructure::EntityJobItem<FailoverUnit, TContext>::TryProcessFunctionPtr handler,
                    TContext & context)
                {
                    ExecuteUnderLock(ftShortName, handler, context, true);
                }

                template<typename TContext>
                void ExecuteJobItemHandlerWithContext(
                    std::wstring const & ftShortName,
                    typename Infrastructure::EntityJobItem<FailoverUnit, TContext>::TryProcessFunctionPtr handler,
                    TContext & context,
                    bool callEnableUpdate)
                {
                    ExecuteUnderLock(
                        ftShortName,
                        [&context, &handler](Infrastructure::LockedFailoverUnitPtr & ft, Infrastructure::StateMachineActionQueue & queue, ReconfigurationAgent & ra)
                        {
                            HandlerParameters parameters("a", ft, ra, queue, nullptr, DefaultActivityId);
                            handler(parameters, context);
                        },
                        callEnableUpdate);
                }

                // Add an FT in a certain state with default properties
                Infrastructure::LocalFailoverUnitMapEntrySPtr AddFT(std::wstring const & ftShortName, std::wstring const & ft);

                Infrastructure::LocalFailoverUnitMapEntrySPtr AddDeletedFT(std::wstring const & ftShortName);
                Infrastructure::LocalFailoverUnitMapEntrySPtr AddInsertedFT(std::wstring const & ftShortName);

                // Add an FT in the closed state
                Infrastructure::LocalFailoverUnitMapEntrySPtr AddClosedFT(std::wstring const & ftShortName);

                // Lookup an FT from the LFUM
                Infrastructure::LocalFailoverUnitMapEntrySPtr GetLFUMEntry(std::wstring const & ftShortName);

                typedef std::function<void(Infrastructure::LockedFailoverUnitPtr &, Infrastructure::StateMachineActionQueue &, ReconfigurationAgent &)> ExecuteUnderLockTargetFunctionPtr;
                void ExecuteUnderLock(std::wstring const & ftShortName, ExecuteUnderLockTargetFunctionPtr func);
                void ExecuteUnderLock(std::wstring const & ftShortName, ExecuteUnderLockTargetFunctionPtr func, bool callEnableUpdate);

                template<typename T>
                void ExecuteUnderLockHelper(
                    Infrastructure::EntityEntryBaseSPtr const & entry,
                    std::function<void(Infrastructure::LockedEntityPtr<T> &, Infrastructure::StateMachineActionQueue &, ReconfigurationAgent &)> func,
                    bool callEnableUpdate)
                {
                    auto & casted = entry->As<Infrastructure::EntityEntry<T>>();
                    Infrastructure::StateMachineActionQueue queue;
                    Infrastructure::LockedEntityPtr<T> lockedEntity(casted.Test_CreateLock());

                    if (callEnableUpdate)
                    {
                        lockedEntity.EnableUpdate();
                    }

                    func(lockedEntity, queue, RA);

                    if (!lockedEntity.IsUpdating)
                    {
                        queue.ExecuteAllActions(DefaultActivityId, entry, RA);
                    }
                    else
                    {
                        auto error = casted.Test_Commit(entry, lockedEntity, RA);
                        if (error.IsSuccess())
                        {
                            queue.ExecuteAllActions(DefaultActivityId, entry, RA);
                        }
                        else
                        {
                            queue.AbandonAllActions(RA);
                        }
                    }
                }

                bool Cleanup();

                void ResetAll()
                {
                    StateItemHelpers.ResetAll();
                }

                shared_ptr<NodeUpOperationFactory> RestartRA();

#pragma region Hosting

                void SetFindServiceTypeRegistrationState(std::wstring const & ftShortName, FindServiceTypeRegistrationError::CategoryName::Enum errorCategory);
                void SetFindServiceTypeRegistrationSuccess(std::wstring const & ftShortName);

                void ProcessRuntimeClosed(std::wstring const & ftShortName);

                void ProcessAppHostClosed(std::wstring const & ftShortName, Common::ActivityDescription const & activityDescrition = Common::ActivityDescription::Empty);

                void ProcessServiceTypeRegistered(std::wstring const & ftShortName);

                void ProcessServiceTypeDisabled(
                    std::wstring const& ftShortName,
                    uint64 sequenceNumber,
                    ServiceModel::ServicePackageActivationMode::Enum activationMode = ServiceModel::ServicePackageActivationMode::SharedProcess);
                void ProcessServiceTypeEnabled(
                    std::wstring const& ftShortName,
                    uint64 sequenceNumber,
                    ServiceModel::ServicePackageActivationMode::Enum activationMode = ServiceModel::ServicePackageActivationMode::SharedProcess);

                void ProcessRuntimeClosedAndDrain(std::wstring const & ftShortName);

                void ProcessAppHostClosedAndDrain(std::wstring const & ftShortName, Common::ActivityDescription const & activityDescription = Common::ActivityDescription::Empty);

                void ProcessServiceTypeRegisteredAndDrain(std::wstring const & ftShortName);

                void ValidateNoTerminateCalls();

                void ValidateTerminateCall(std::wstring const & ftShortName);
                void ValidateTerminateCall(std::initializer_list<std::wstring> const & fts);

#pragma endregion

#pragma region Node Activation State

                // Validate for both FM and FMM
                void ValidateNodeIsActivated(uint64 expectedSequenceNumber);
                void ValidateNodeIsDeactivated(uint64 expectedSequenceNumber);
                void ValidateNodeActivationState(bool isActivated, uint64 expectedSequenceNumber);

                void ValidateNodeIsActivated(bool isFmm, uint64 expectedSequenceNumber);
                void ValidateNodeIsDeactivated(bool isFmm, uint64 expectedSequenceNumber);
                void ValidateNodeActivationState(bool isFmm, bool isActivated, uint64 expectedSequenceNumber);

                void ActivateNode(int64 sequenceNumber);
                void DeactivateNode(int64 sequenceNumber);
                void SetNodeActivationState(bool isActivated, int64 sequenceNumber);

                void SetNodeUpAckFromFMProcessed(bool value);
                void SetNodeUpAckFromFmmProcessed(bool value);
                void SetNodeUpAckProcessed(bool fm, bool fmm);

                void FireNodeDeactivationProgressCheckTimer(bool isFmm);
                void ValidateNodeDeactivationProgressCheckNotRunning(bool isFmm);

                void ValidateIsUpgrading(bool expected);
                void SetIsUpgrading(bool value);
                void SetLifeCycleState(StateItemHelper::RALifeCycleState::Enum e);


#pragma endregion

#pragma region Queues

                // Drain all queues
                void DrainJobQueues();

#pragma endregion

#pragma region Logging

                void LogStep(std::wstring const & step);

                template<StateManagement::MessageType::Enum T>
                void LogMessageAction()
                {
                    LogStep(std::wstring(StateManagement::MessageTypeTraits<T>::Action.begin()));
                }

#pragma endregion

#pragma region Health

                void ValidateReplicaHealthEvent(
                    Health::ReplicaHealthEvent::Enum type,
                    std::wstring const & shortName,
                    Common::ErrorCodeValue::Enum error)
                {
                    auto & ft = GetFT(shortName);
                    ValidateReplicaHealthEvent(type, shortName, ft.LocalReplicaId, ft.LocalReplicaInstanceId, error);
                }


                void ValidateReplicaHealthEvent(
                    Health::ReplicaHealthEvent::Enum type,
                    std::wstring const & shortName,
                    uint64 replicaId,
                    uint64 instanceId,
                    Common::ErrorCodeValue::Enum error)
                {
                    auto context = GetSingleFTContext(shortName);
                    stateItemHelper_->HealthHelper.ValidateReplicaHealthEvent(type, context.FUID, context.STInfo.SD.IsStateful, replicaId, instanceId, error);
                }


                void ValidateReplicaHealthEvent(
                    Health::ReplicaHealthEvent::Enum type,
                    std::wstring const & shortName,
                    uint64 replicaId,
                    uint64 instanceId)
                {
                    ValidateReplicaHealthEvent(type, shortName, replicaId, instanceId, Common::ErrorCodeValue::Success);
                }

                void ValidateNoReplicaHealthEvent()
                {
                    stateItemHelper_->HealthHelper.ValidateNoReplicaHealthEvent();
                }


#pragma endregion

#pragma region Upgrade
                Upgrade::UpgradeMessageProcessor::UpgradeElement const * GetUpgradeElementForApplicationUpgrade(uint64 instanceId);
                Upgrade::UpgradeMessageProcessor::UpgradeElement const * GetUpgradeElementForFabricUpgrade(uint64 instanceId);

                void ValidateFabricUpgradeIsCompleted(uint64 instanceId);
                void ValidateApplicationUpgradeIsCompleted(uint64 instanceId);

                void FireApplicationUpgradeCloseCompletionCheckTimer(uint64 instanceId);
                void FireApplicationUpgradeDropCompletionCheckTimer(uint64 instanceId);
                void FireApplicationUpgradeReplicaDownCompletionCheckTimer(uint64 instanceId);
                void FireFabricUpgradeCloseCompletionCheckTimer(uint64 instanceId);

#pragma endregion

#pragma region Messages

#pragma region Validation

                void ValidateNodeUpAckReceived(bool fromFMM, bool fromFM);

                void ValidateNoFMMessages()
                {
                    stateItemHelper_->FMMessageHelper.ValidateNoMessages();
                }

                void ValidateNoFmmMessages()
                {
                    stateItemHelper_->FMMMessageHelper.ValidateNoMessages();
                }

                void ValidateNoRAPMessage(std::wstring const & ftShortName)
                {
                    auto context = GetSingleFTContext(ftShortName);
                    stateItemHelper_->RAPMessageHelper.ValidateNoMessages(context.STInfo.HostId);
                }

                void ValidateNoRAPMessages()
                {
                    stateItemHelper_->RAPMessageHelper.ValidateNoMessages();
                }

                void ValidateNoRAMessage(uint64 nodeId)
                {
                    stateItemHelper_->RAMessageHelper.ValidateNoMessages(nodeId);
                }

                void ValidateNoRAMessage()
                {
                    stateItemHelper_->RAMessageHelper.ValidateNoMessages();
                }

                void ValidateNoMessages()
                {
                    ValidateNoFMMessages();
                    ValidateNoFmmMessages();
                    ValidateNoRAMessage();
                    ValidateNoRAPMessages();
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateNoFMMessagesOfType()
                {
                    stateItemHelper_->FMMessageHelper.ValidateNoMessagesOfType<T>();
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateNoFmmMessagesOfType()
                {
                    stateItemHelper_->FMMMessageHelper.ValidateNoMessagesOfType<T>();
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateFMMessage(std::wstring const & ftShortName, std::wstring const & messageBody)
                {
                    auto context = GetSingleFTContext(ftShortName);
                    stateItemHelper_->FMMessageHelper.ValidateMessage<T>(messageBody, context);
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateFMMessage()
                {
                    stateItemHelper_->FMMessageHelper.ValidateMessage<T>();
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateFMMessage(std::wstring const & messageBody)
                {
                    stateItemHelper_->FMMessageHelper.ValidateMessage<T>(messageBody, readContext_);
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateFmmMessage(std::wstring const & ftShortName, std::wstring const & messageBody)
                {
                    auto context = GetSingleFTContext(ftShortName);
                    stateItemHelper_->FMMMessageHelper.ValidateMessage<T>(messageBody, context);
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateFmmMessage(std::wstring const & messageBody)
                {
                    stateItemHelper_->FMMMessageHelper.ValidateMessage<T>(messageBody, readContext_);
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateFmmMessage()
                {
                    stateItemHelper_->FMMMessageHelper.ValidateMessage<T>();
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateRAPMessage(std::wstring const & ftShortName, std::wstring const & messageBody)
                {
                    auto context = GetSingleFTContext(ftShortName);
                    stateItemHelper_->RAPMessageHelper.ValidateMessage<T>(context.STInfo.HostId, messageBody, context);
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateRAPRequestReplyMessage(std::wstring const & ftShortName, std::wstring const & messageBody)
                {
                    auto context = GetSingleFTContext(ftShortName);
                    stateItemHelper_->RAPMessageHelper.ValidateRequestReplyMessage<T>(context.STInfo.HostId, messageBody, context);
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateRAPMessage(std::wstring const & ftShortName)
                {
                    auto context = GetSingleFTContext(ftShortName);
                    stateItemHelper_->RAPMessageHelper.ValidateMessage<T>(context.STInfo.HostId);
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateRAMessage(uint64 nodeId, std::wstring const & ftShortName, std::wstring const & messageBody)
                {
                    auto context = GetSingleFTContext(ftShortName);
                    stateItemHelper_->RAMessageHelper.ValidateMessage<T>(nodeId, messageBody, context);
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateRAMessage(uint64 nodeId)
                {                    
                    stateItemHelper_->RAMessageHelper.ValidateMessage<T>(nodeId);
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateRequestReceiverContextReply(std::wstring const & tag, std::wstring const & body)
                {
                    stateItemHelper_->RequestReceiverHelper.ValidateReply<T>(tag, body, readContext_);
                }

                void ValidateErrorReply(std::wstring const & tag, Common::ErrorCode const & errorCode)
                {
                    stateItemHelper_->RequestReceiverHelper.ValidateError(tag, errorCode);
                }

#pragma endregion

#pragma region Processing
                // Process a federation one way message
                void ProcessRequestHelper(Transport::MessageUPtr && message, Federation::NodeInstance const & from)
                {
                    auto metadata = RA.MessageMetadataTable.LookupMetadata(message);
                    auto context = Common::make_unique<Federation::OneWayReceiverContext>(nullptr, from, Transport::MessageId());

                    RA.ProcessRequest(metadata, std::move(message), std::move(context));
                }

                // Process a FM message about an entity (AddPrimary etc)
                template<StateManagement::MessageType::Enum T>
                void ProcessFMMessage(std::wstring const & shortName, std::wstring const & messageBody)
                {
                    auto message = ParseMessage<T>(shortName, messageBody);

                    ProcessRequestHelper(std::move(message), ReconfigurationAgent::InvalidNode);
                }

                // Process a FM message about an entity (AddPrimary etc)
                template<StateManagement::MessageType::Enum T>
                void ProcessFMMessageAndDrain(std::wstring const & shortName, std::wstring const & messageBody)
                {
                    LogMessageAction<T>();
                    ProcessFMMessage<T>(shortName, messageBody);
                    DrainJobQueues();
                }

                // Process a FM message for the node (DeactivateNode etc)
                template<StateManagement::MessageType::Enum T>
                void ProcessFMMessage(std::wstring const & messageBody)
                {
                    ProcessFMMessage<T>(messageBody, ReconfigurationAgent::InvalidNode);
                }

                // Process a FM message for the node (DeactivateNode etc)
                template<StateManagement::MessageType::Enum T>
                void ProcessFMMessageAndDrain(std::wstring const & messageBody)
                {
                    ProcessFMMessageAndDrain<T>(messageBody, ReconfigurationAgent::InvalidNode);
                }

                // Process a FM message for the node (DeactivateNode etc)
                template<StateManagement::MessageType::Enum T>
                void ProcessFMMessage(std::wstring const & messageBody, Federation::NodeInstance from)
                {
                    auto message = ParseMessage<T>(messageBody);
                    
                    ProcessRequestHelper(std::move(message), from);
                }

                // Process a FM message for the node (DeactivateNode etc)
                template<StateManagement::MessageType::Enum T>
                void ProcessFMMessageAndDrain(std::wstring const & messageBody, Federation::NodeInstance from)
                {
                    LogMessageAction<T>();
                    ProcessFMMessage<T>(messageBody, from);
                    DrainJobQueues();
                }

                // Process a message from remote ra about an entity (CreateReplica etc)
                template<StateManagement::MessageType::Enum T>
                void ProcessRemoteRAMessage(uint64 nodeId, std::wstring const & shortName, std::wstring const & messageBody)
                {
                    auto message = ParseMessage<T>(shortName, messageBody);
                    
                    ProcessRequestHelper(std::move(message), Federation::NodeInstance(Federation::NodeId(Common::LargeInteger(0, nodeId)), 1));
                }

                // Process a message from remote ra about an entity (CreateReplica etc)
                template<StateManagement::MessageType::Enum T>
                void ProcessRemoteRAMessageAndDrain(uint64 nodeId, std::wstring const & shortName, std::wstring const & messageBody)
                {
                    LogMessageAction<T>();
                    ProcessRemoteRAMessage<T>(nodeId, shortName, messageBody);
                    DrainJobQueues();
                }

                // Process RAP message about entity
                template<StateManagement::MessageType::Enum T>
                void ProcessRAPMessage(std::wstring const & shortName, std::wstring const & messageBody)
                {
                    auto message = ParseMessage<T>(shortName, messageBody);
                    auto metadata = RA.MessageMetadataTable.LookupMetadata(message);
                    
                    RA.ProcessIpcMessage(
                        metadata,
                        std::move(message), 
                        nullptr);
                }

                // Process RAP message about entity
                template<StateManagement::MessageType::Enum T>
                void ProcessRAPMessageAndDrain(std::wstring const & ftShortName, std::wstring const & messageBody)
                {
                    LogMessageAction<T>();
                    ProcessRAPMessage<T>(ftShortName, messageBody);
                    DrainJobQueues();
                }

                template<StateManagement::MessageType::Enum T>
                void ProcessRMMessageAndDrain(std::wstring const & messageBody)
                {
                    LogMessageAction<T>();
                    auto message = ParseMessage<T>(messageBody);
                    auto metadata = RA.MessageMetadataTable.LookupMetadata(message);
                    RA.ProcessIpcMessage(metadata, std::move(message), nullptr);
                    DrainJobQueues();
                }

                template<StateManagement::MessageType::Enum T>
                void ProcessRequestReceiverContext(std::wstring const & tag, std::wstring const & messageBody)
                {
                    auto message = ParseMessage<T>(messageBody);
                    auto sourceId = Transport::MessageId();

                    auto contextUPtr = utContext_->FederationWrapper.CreateRequestReceiverContext(sourceId, tag);

                    auto metadata = RA.MessageMetadataTable.LookupMetadata(message);
                    RA.ProcessRequestResponseRequest(metadata, std::move(message), std::move(contextUPtr));
                }

                template<StateManagement::MessageType::Enum T>
                void ProcessRequestReceiverContextAndDrain(std::wstring const & tag, std::wstring const & messageBody)
                {
                    LogMessageAction<T>();
                    ProcessRequestReceiverContext<T>(tag, messageBody);
                    DrainJobQueues();
                }

                void ProcessNodeUpAckAndDrain(std::wstring const & message, bool isFMM);
                void ProcessNodeUpAck(std::wstring const & message, bool isFMM);

#pragma endregion

                template<typename TBody>
                static Transport::MessageUPtr CreateMessage(TBody & body, std::wstring const & action, Reliability::GenerationHeader const & genHeader)
                {
                    auto message = Common::make_unique<Transport::Message>(body);
                    message->Headers.Add(Transport::ActionHeader(action));
                    message->Headers.Add(genHeader);

                    // TODO: Clean way to add query header
                    message->Headers.Add(::Query::QueryAddressHeader(::Query::QueryAddresses::RAAddressSegment));
                    message->Headers.Add(Transport::FabricActivityHeader(Common::Guid()));
                    return message;
                }

                template<StateManagement::MessageType::Enum T>
                Transport::MessageUPtr ParseMessage(std::wstring const & shortName, std::wstring const & messageBody)
                {
                    StateManagement::ErrorLogger errLogger(L"MessageParser", messageBody);

                    auto context = GetContext<T>(shortName);

                    return ReadMessage<T>(messageBody, *context);
                }

                template<StateManagement::MessageType::Enum T>
                Transport::MessageUPtr ParseMessage(std::wstring const & messageBody)
                {
                    return ReadMessage<T>(messageBody, readContext_);
                }

                template<typename T>
                T ReadObject(std::wstring const & ftShortName, std::wstring const & body)
                {
                    StateManagement::ErrorLogger errLogger(L"ObjectParser", body);

                    StateManagement::SingleFTReadContext const * ftContext = readContext_.GetSingleFTContext(ftShortName);
                    if (ftContext == nullptr)
                    {
                        errLogger.Log(L"FTShortName");
                        Verify::Fail(Common::wformatString("Unable to find ftShortName {0}", ftShortName));
                    }

                    Reader r(body, *ftContext);
                    T obj;
                    if (!r.Read(L'\0', obj))
                    {
                        Verify::Fail(Common::wformatString("Unable to find parse body"));
                    }

                    return obj;
                }


#pragma endregion

# pragma region Replica Up
                void ExecuteUpdateStateOnLfumLoad(std::wstring const & ftShortName);
                void ExecuteUpdateStateOnLfumLoad(std::wstring const & ftShortName, Federation::NodeInstance const & nodeInstance);
# pragma endregion

#pragma region Work Managers

                void RequestWorkAndDrain(StateItemHelper::MultipleFTBackgroundWorkManagerStateHelper & worker)
                {
                    LogStep(Common::wformatString("Request Work: {0}", worker.StateItem));
                    worker.RequestWork();
                    DrainJobQueues();
                }

                void RequestWorkAndDrain(StateItemHelper::BackgroundWorkManagerWithRetryStateHelper & worker)
                {
                    LogStep(Common::wformatString("Request Work: {0}", worker.StateItem));
                    worker.RequestWork();
                    DrainJobQueues();
                }

                void RequestFMMessageRetryAndDrain(Reliability::FailoverManagerId const & fm)
                {
                    LogStep(Common::wformatString("Request {0}", fm));
                    stateItemHelper_->FMMessageRetryHelperObj.Request(fm);
                    DrainJobQueues();
                }

#pragma endregion

#pragma region Reliability
                void ValidateLfumUploadData(StateItemHelper::GenerationNumberSource::Enum source, size_t index, std::wstring const & ftShortName, std::wstring const & ftInfo);
#pragma endregion

#pragma region Store
                void EnableLfumStoreFaultInjection(Common::ErrorCodeValue::Enum error = Storage::FaultInjectionAdapter::DefaultError);
                void DisableLfumStoreFaultInjection();
#pragma endregion

                static ScenarioTestUPtr Create();

                static ScenarioTestUPtr Create(UnitTestContext::Option options);

                static Reliability::GenerationHeader GetDefaultGenerationHeader()
                {
                    return Reliability::GenerationHeader(Reliability::GenerationNumber(0, ReconfigurationAgent::InvalidNode.Id), false);
                }

            private:

                template<StateManagement::MessageType::Enum T>
                StateManagement::ReadContext const * GetContext(std::wstring const & shortName)
                {
                    auto action = std::wstring(StateManagement::MessageTypeTraits<T>::Action.begin());
                    auto metadata = RA.MessageMetadataTable.LookupMetadata(action);

                    StateManagement::ReadContext const * context = nullptr;
                    switch (metadata->Target)
                    {
                    case Communication::MessageTarget::FT:
                        context = readContext_.GetSingleFTContext(shortName);
                        break;
                    default:
                        Verify::Fail(Common::wformatString("Unknown target {0} for {1}", static_cast<int>(metadata->Target), action));
                        break;
                    }

                    if (context == nullptr)
                    {
                        Verify::Fail(Common::wformatString("Unable to find ftShortName {0}", shortName));
                    }

                    return context;
                }


                template<StateManagement::MessageType::Enum T>
                static Transport::MessageUPtr ReadMessage(std::wstring const & messageBody, StateManagement::ReadContext const & context)
                {
                    Reader r(messageBody, context);

                    Reliability::GenerationHeader genHeader = GetDefaultGenerationHeader();

                    // Check for optional generation header
                    if (r.PeekChar() == L'g')
                    {
                        if (!r.Read(L' ', genHeader))
                        {
                            Verify::Fail(Common::wformatString("unable to parse gen header"));
                        }
                    }

                    typename StateManagement::MessageTypeTraits<T>::BodyType body;
                    if (!r.Read(L'\0', body))
                    {
                        Verify::Fail(Common::wformatString("Unable to find parse body"));
                    }

                    return CreateMessage(body, std::wstring(StateManagement::MessageTypeTraits<T>::Action.begin()), genHeader);
                }

                void FireCloseCompletionCheckTimerAndDrain(Common::AsyncOperationSPtr const & operation);
                ScenarioTest(UnitTestContextUPtr && utContext);
                UnitTestContextUPtr utContext_;
                StateManagement::MultipleReadContextContainer readContext_;
                StateItemHelper::StateItemHelperCollectionUPtr stateItemHelper_;
                EntityExecutionContextTestUtility entityExecutionContextTestUtility_;
            };

        }
    }
}
