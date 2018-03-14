// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "RATestPointers.h"
#include "Reliability/Failover/ra/Diagnostics.IEventWriter.h"
#include "Test.Utility.EventWriterStub.h"

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            namespace StateItemHelper
            {
				static Common::StringLiteral const TestStateItemHelperSource("StateItemHelperSourceTest");

                class GenerationInfo
                {
                public:
                    GenerationInfo(int64 number, int nodeId)
                        : number_(number, Federation::NodeId(Common::LargeInteger(0, nodeId)))
                    {
                    }

                    __declspec(property(get = get_Number)) Reliability::GenerationNumber const & Number;
                    Reliability::GenerationNumber const & get_Number() const { return number_; }

                private:
                    Reliability::GenerationNumber number_;
                };

                namespace GenerationNumberSource
                {
                    enum Enum
                    {
                        FM = 0,
                        FMM = 1,
                    };
                };

                namespace RAStateItem
                {
                    enum Enum
                    {
                        None = 0x00,
                        FMMessages = 0x01,
                        FMMMessages = 0x02,
                        RAMessages = 0x04,
                        RAPMessages = 0x08,

                        // BGMR states
                        MessageRetryTimer = 0x10,
                        FMMessageRetry = 0x40,

                        // Multiple FT Work Manager States
                        StateCleanupWorkManager = 0x80,
                        ReconfigurationMessageRetryWorkManager = 0x100,
                        ReplicaCloseRetryWorkManager = 0x400,
                        ReplicaOpenRetryWorkManager = 0x1000,
                        UpdateServiceDescriptionRetryWorkManager = 0x2000,

                        Hosting = 0x4000,

                        Health = 0x8000,
                        RequestReceiverContexts = 0x10000,
                        Reliability = 0x20000,

                        Generation = 0x40000,

                        PerfCounters = 0x80000,

                        LastReplicaUpState = 0x100000,

                        RALifeCycleState = 0x200000, 

                        Diagnostics = 0x400000
                    };

                    void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
                }

                class StateItemHelperBase
                {
                    DENY_COPY(StateItemHelperBase);

                public:

                    __declspec(property(get = get_StateItem)) RAStateItem::Enum StateItem;
                    RAStateItem::Enum get_StateItem() const { return stateItem_; }

                    virtual ~StateItemHelperBase() {}

                    // Called to reset the contents of this state item to default values
                    virtual void Reset() = 0;

                protected:
                    StateItemHelperBase(RAStateItem::Enum stateItem)
                        : stateItem_(stateItem)
                    {
                    }

                    RAStateItem::Enum stateItem_;
                };

                class FMMessageRetryHelper : public StateItemHelperBase
                {
                    DENY_COPY(FMMessageRetryHelper);

                public:
                    FMMessageRetryHelper(ReconfigurationAgent & ra);

                    void Reset();

                    void Request(Reliability::FailoverManagerId const & fm);

                private:
                    ReconfigurationAgent & ra_;
                };

                class LastReplicaUpStateHelper : public StateItemHelperBase
                {
                    DENY_COPY(LastReplicaUpStateHelper);

                public:
                    LastReplicaUpStateHelper(ReconfigurationAgent & ra);

                    void Reset();

                    void ValidateLastReplicaUpReceivedForFM();

                    void ValidateLastReplicaUpReplyState(Reliability::FailoverManagerId const & fm, bool isReceived);

                    void ValidateToBeUploadedReplicaSetIsEmpty(Reliability::FailoverManagerId const & fm);

                    void Request(Reliability::FailoverManagerId const & fm);

                private:
                    Node::PendingReplicaUploadState & GetPendingReplicaUploadState(Reliability::FailoverManagerId const & fm);

                    Node::PendingReplicaUploadStateProcessor & GetPendingReplicaUploadStateProcessor(Reliability::FailoverManagerId const & fm);

                    ReconfigurationAgent & ra_;
                };

                class RetryTimerStateHelper
                {
                    DENY_COPY(RetryTimerStateHelper);

                public:
                    RetryTimerStateHelper(Infrastructure::RetryTimer & timer, RAStateItem::Enum stateItem);

                    void CancelTimer();

                    void ValidateTimerIsSet();

                    void ValidateTimerIsNotSet();

                    void FireTimer();

                    void Reset();

                    void Set();

                private:
                    Infrastructure::RetryTimer & timer_;
                    RAStateItem::Enum stateItem_;
                };

                class MultipleFTBackgroundWorkManagerStateHelper : public StateItemHelperBase
                {
                    DENY_COPY(MultipleFTBackgroundWorkManagerStateHelper);

                public:
                    MultipleFTBackgroundWorkManagerStateHelper(RAStateItem::Enum item, Infrastructure::MultipleEntityBackgroundWorkManager & multipleFTBGM);

                    __declspec(property(get = get_RetryTimerHelper)) RetryTimerStateHelper & RetryTimerHelper;
                    RetryTimerStateHelper & get_RetryTimerHelper() { return timerStateHelper_; }

                    void RequestWork();

                    void ValidateIsSetButEmpty();

                    void ValidateTimerIsSet();

                    void ValidateTimerIsNotSet();

                    void Reset();

                private:
                    Infrastructure::MultipleEntityBackgroundWorkManager & workManager_;
                    RetryTimerStateHelper timerStateHelper_;
                };

                class BackgroundWorkManagerWithRetryStateHelper : public StateItemHelperBase
                {
                    DENY_COPY(BackgroundWorkManagerWithRetryStateHelper);

                public:
                    BackgroundWorkManagerWithRetryStateHelper(RAStateItem::Enum item, Infrastructure::BackgroundWorkManagerWithRetry & bgmr);

                    __declspec(property(get = get_RetryTimerHelper)) RetryTimerStateHelper & RetryTimerHelper;
                    RetryTimerStateHelper & get_RetryTimerHelper() { return timerStateHelper_; }

                    void RequestWork();

                    void ValidateTimerIsSet();

                    void ValidateTimerIsNotSet();
                    
                    void Reset();

                private:
                    Infrastructure::BackgroundWorkManagerWithRetry & workManager_;
                    RetryTimerStateHelper timerStateHelper_;
                };

                class HostingStateHelper : public StateItemHelperBase
                {
                    DENY_COPY(HostingStateHelper);

                public:
                    HostingStateHelper(HostingStub & hosting);

                    void Reset();

                    __declspec(property(get = get_HostingObj)) HostingStub & HostingObj;
                    HostingStub & get_HostingObj() { return hosting_; }

                    void ValidateDownloadCount(size_t expected);
                    void ValidateDownloadSpecification(size_t downloadIndex, size_t packageCountExpected);
                    void ValidateDownloadPackageSpecification(size_t downloadIndex, size_t packageIndex, ULONG rolloutMajorVersion, ULONG rolloutMinorVersion, std::wstring const & packageName);

                    void ValidateUpgradeCount(size_t expected);
                    void ValidateUpgradeSpecification(size_t upgradeIndex, size_t packageCountExpected);
                    void ValidateUpgradePackageSpecification(size_t upgradeIndex, size_t packageIndex, ULONG rolloutMajorVersion, ULONG rolloutMinorVersion, std::wstring const & packageName);

                    void ValidateAnalyzeCount(size_t expected);
                    void ValidateAnalyzeSpecification(size_t analyzeIndex, size_t packageCountExpected);
                    void ValidateAnalyzePackageSpecification(size_t analyzeIndex, size_t packageIndex, ULONG rolloutMajorVersion, ULONG rolloutMinorVersion, std::wstring const & packageName);

                    void SetAllAsyncApisToCompleteSynchronouslyWithSuccess();

                    void ValidateNoTerminateCalls();
                    void ValidateTerminateCall(std::wstring const & hostId);
                    void ValidateTerminateCall(std::vector<std::wstring> const & hostIds);

                private:
                    void ValidateServiceModelUpgradeSpecification(
                        ServiceModel::ApplicationUpgradeSpecification const & actual,
                        size_t packageCountExpected,
                        std::wstring const & tag);

                    void ValidateServiceModelUpgradePackageSpecification(
                        std::vector<ServiceModel::ApplicationUpgradeSpecification> const & actual,
                        size_t index,
                        size_t packageIndex,
                        ULONG rolloutMajorVersion,
                        ULONG rolloutMinorVersion,
                        std::wstring const & packageName,
                        std::wstring const & tag);

                    HostingStub & hosting_;
                };

                class ReliabilityStateHelper : public StateItemHelperBase
                {
                    DENY_COPY(ReliabilityStateHelper);

                public:

                    ReliabilityStateHelper(ReliabilitySubsystemStub & reliability);

                    void Reset();

                    void ValidateNodeUpToFM() const;

                    void ValidateNodeUpToFMM() const;

                    void ValidateLfumUploadCount(GenerationNumberSource::Enum source);
                    void ValidateLfumUpload(GenerationNumberSource::Enum source, GenerationInfo info, int replicaCountExpected, bool anyReplicaFoundExpected);
                    void ValidateLfumUploadData(GenerationNumberSource::Enum source, size_t index, Reliability::FailoverUnitInfo const & ftInfoExpected);
                    void ValidateNoLfumUpload(GenerationNumberSource::Enum source);

                private:

                    std::vector<ReliabilitySubsystemStub::UploadLfumInfo> const & GetVector(GenerationNumberSource::Enum source);
                    ReliabilitySubsystemStub & reliability_;
                };

                template<StateManagement::MessageType::Enum T>
                Transport::Message* GetFirstMessageMatchingAction(
                    RAStateItem::Enum e,
                    std::vector<Transport::MessageUPtr> const & messageVector)
                {
                    std::vector<Transport::Message*> casted;
                    for (auto const & it : messageVector)
                    {
                        casted.push_back(it.get());
                    }

                    return GetFirstMessageMatchingAction<T>(e, casted);
                }

                template<StateManagement::MessageType::Enum T>
                Transport::Message* GetFirstMessageMatchingAction(
                    RAStateItem::Enum e,
                    std::vector<Transport::Message*> const & messageVector)
                {
                    auto expectedAction = std::wstring(StateManagement::MessageTypeTraits<T>::Action.begin());
                    std::vector<Transport::Message*> messagesMatchingAction;
                    for (auto const & it : messageVector)
                    {
                        if (it->Action == expectedAction)
                        {
                            messagesMatchingAction.push_back(it);
                        }
                    }

                    if (messagesMatchingAction.size() != 1)
                    {
                        auto msg = Common::wformatString("{0}: Message {1} mismatch in list. List size {2}. Contents: ", e, expectedAction, messagesMatchingAction.size());
                        for (auto const & it : messagesMatchingAction)
                        {
                            msg += it->Action;
                            msg += L", ";
                        }

                        if (messagesMatchingAction.empty())
                        {
                            Verify::Fail(msg);
                            return nullptr;
                        }
                        else
                        {
                            TestLog::WriteInfo(Common::wformatString("{0}", msg));
                        }
                    }

                    return messagesMatchingAction[0];
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateMessageHelper(
                    RAStateItem::Enum e,
                    std::vector<Transport::Message*> const & messageVector,
                    std::wstring const & body,
                    StateManagement::ReadContext const & context,
                    StateManagement::ReadOption::Enum readOption)
                {
                    auto actualMessage = GetFirstMessageMatchingAction<T>(e, messageVector);
                    if (actualMessage == nullptr)
                    {
                        return;
                    }

                    ValidateMessageHelper<T>(e, *actualMessage, body, context, readOption);
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateMessageHelper(
                    RAStateItem::Enum e,
                    std::vector<Transport::MessageUPtr> const & messageVector,
                    std::wstring const & body,
                    StateManagement::ReadContext const & context,
                    StateManagement::ReadOption::Enum readOption)
                {
                    auto actualMessage = GetFirstMessageMatchingAction<T>(e, messageVector);
                    if (actualMessage == nullptr)
                    {
                        return;
                    }

                    ValidateMessageHelper<T>(e, *actualMessage, body, context, readOption);
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateMessageHelper(
                    RAStateItem::Enum e,
                    std::vector<Transport::MessageUPtr> const & messageVector)
                {
                    GetFirstMessageMatchingAction<T>(e, messageVector);
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateMessageHelper(
                    RAStateItem::Enum e,
                    Transport::Message & actualMessage,
                    std::wstring const & body,
                    StateManagement::ReadContext const & context,
                    StateManagement::ReadOption::Enum z)
                {
                    Trace.WriteInfo(TestStateItemHelperSource, "Verifying message for {0} with action {1}", e, StateManagement::MessageTypeTraits<T>::Action);

                    ValidateMessageHelper<T>(actualMessage, body, context, z);
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateMessageHelper(
                    Transport::Message & actualMessage,
                    std::wstring const & body,
                    StateManagement::ReadContext const & context,
                    StateManagement::ReadOption::Enum)
                {
                    ValidateMessageAction<T>(actualMessage);

                    typename StateManagement::MessageTypeTraits<T>::BodyType actualBody;
                    bool isBodyValid = actualMessage.GetBody<typename StateManagement::MessageTypeTraits<T>::BodyType>(actualBody);
                    VERIFY_IS_TRUE(isBodyValid);

                    Verify::Compare(body, actualBody, context);
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateMessageAction(
                    Transport::Message const & actualMessage)
                {
                    Verify::AreEqualLogOnError(std::wstring(StateManagement::MessageTypeTraits<T>::Action.begin()), actualMessage.Action);
                }

                template<StateManagement::MessageType::Enum T>
                void ValidateNoMessageOfType(
                    RAStateItem::Enum,
                    std::vector<Transport::MessageUPtr> const & messageVector)
                {
                    auto expectedAction = std::wstring(StateManagement::MessageTypeTraits<T>::Action.begin());
                    for (auto const & it : messageVector)
                    {
                        if ((*it).Action == expectedAction)
                        {
                            Verify::Fail(wformatString("Found a message of type {0}", expectedAction));
                        }
                    }
                }

                class RAPMessageStateItemHelper : public StateItemHelperBase
                {
                    DENY_COPY(RAPMessageStateItemHelper);

                public:
                    RAPMessageStateItemHelper(RaToRapTransportStub & rapTransport);

                    template<StateManagement::MessageType::Enum T>
                    void ValidateMessage(std::wstring const & target, std::wstring const & body, StateManagement::ReadContext & context)
                    {
                        ValidateMessageHelper<T>(stateItem_, rapTransport_.Messages[target], body, context, StateManagement::ReadOption::None);
                    }

                    template<StateManagement::MessageType::Enum T>
                    void ValidateMessage(std::wstring const & target)
                    {
                        ValidateMessageHelper<T>(stateItem_, rapTransport_.Messages[target]);
                    }

                    void ValidateNoMessages(std::wstring const & target)
                    {
                        Verify::AreEqualLogOnError(0, rapTransport_.Messages[target].size(), L"RAP Message count not zero");
                    }

                    void ValidateNoMessages()
                    {
                        Verify::AreEqualLogOnError(0, rapTransport_.Messages.size(), L"RAP Message count not zero");
                    }

                    void ValidateTotalMessageCount(size_t expected)
                    {
                        size_t actual = 0;
                        for (auto const & it : rapTransport_.Messages)
                        {
                            actual += it.second.size();
                        }

                        Verify::AreEqualLogOnError(expected, actual, L"Message count");
                    }

                    template<StateManagement::MessageType::Enum T>
                    void ValidateRequestReplyMessage(std::wstring const & target, std::wstring const & body, StateManagement::ReadContext & context)
                    {
                        // Clone the parameters
                        std::vector<Transport::Message*> messages;
                        for (auto const & it : rapTransport_.RequestReplyApi.CallList)
                        {
                            if (std::get<1>(it->Parameters) == target)
                            {
                                messages.push_back(std::get<0>(it->Parameters).get());
                            }
                        }

                        ValidateMessageHelper<T>(stateItem_, messages, body, context, StateManagement::ReadOption::None);
                    }

                    void Reset();

                private:
                    RaToRapTransportStub & rapTransport_;
                };

                class FMMessageStateItemHelper : public StateItemHelperBase
                {
                    DENY_COPY(FMMessageStateItemHelper);

                public:
                    FMMessageStateItemHelper(FederationWrapperStub & federation, bool isFMM);

                    template<StateManagement::MessageType::Enum T>
                    void ValidateMessage(std::wstring const & body, StateManagement::ReadContext & context)
                    {
                        ValidateMessageHelper<T>(stateItem_, isFMM_ ? federation_.FmmMessages : federation_.FmMessages, body, context, StateManagement::ReadOption::None);
                    }

                    template<StateManagement::MessageType::Enum T>
                    void ValidateMessage()
                    {
                        ValidateMessageHelper<T>(stateItem_, GetMessageVector());
                    }

                    template<StateManagement::MessageType::Enum T>
                    void ValidateNoMessagesOfType()
                    {
                        ValidateNoMessageOfType<T>(stateItem_, GetMessageVector());
                    }

                    void Reset();

                    void ValidateNoMessages()
                    {
                        ValidateCount(0, L"FM/FMM Message count not zero");
                    }

                    void ValidateCount(size_t expected)
                    {
                        ValidateCount(expected, L"FM/FMM Message size mismatch");
                    }

                private:
                    std::vector<Transport::MessageUPtr> const & GetMessageVector()
                    {
                        return isFMM_ ? federation_.FmmMessages : federation_.FmMessages;
                    }

                    void ValidateCount(size_t expected, std::wstring const & message)
                    {
                        auto & messageList = GetMessageVector();

                        std::wstring finalMessage = message;
                        for (auto const & it : messageList)
                        {
                            finalMessage += Common::wformatString("\r\nMessage {0}", it->Action);
                        }

                        Verify::AreEqualLogOnError(expected, messageList.size(), finalMessage);
                    }

                    bool const isFMM_;
                    FederationWrapperStub & federation_;
                };

                class RAMessageStateItemHelper : public StateItemHelperBase
                {
                    DENY_COPY(RAMessageStateItemHelper);

                public:
                    RAMessageStateItemHelper(FederationWrapperStub & federation);

                    template<StateManagement::MessageType::Enum T>
                    void ValidateMessage(uint64 nodeId, std::wstring const & body, StateManagement::ReadContext const & context)
                    {
                        ValidateMessageHelper<T>(stateItem_, federation_.OtherNodeMessages[Federation::NodeId(Common::LargeInteger(0, nodeId))], body, context, StateManagement::ReadOption::None);
                    }

                    template<StateManagement::MessageType::Enum T>
                    void ValidateMessage(uint64 nodeId)
                    {
                        ValidateMessageHelper<T>(stateItem_, federation_.OtherNodeMessages[Federation::NodeId(Common::LargeInteger(0, nodeId))]);
                    }

                    void ValidateNoMessages(uint64 nodeId)
                    {
                        Verify::AreEqualLogOnError(0, federation_.OtherNodeMessages[Federation::NodeId(Common::LargeInteger(0, nodeId))].size(), L"Remote RA Message count not zero");
                    }

                    void ValidateNoMessages()
                    {
                        Verify::AreEqualLogOnError(0, federation_.OtherNodeMessages.size(), L"RA Message Count");
                    }

                    void Reset();

                private:

                    FederationWrapperStub & federation_;
                };

                class EventWriterStateItemHelper : public StateItemHelperBase
                {
                    DENY_COPY(EventWriterStateItemHelper);

                public:
                    EventWriterStateItemHelper(Diagnostics::IEventWriter & writer);

                    void Reset();

                    bool IsTracedEventsEmpty();

                    template<typename T> 
                    std::shared_ptr<T> GetEvent(int index = 0)
                    {
                        if (writer_.TracedEvents.empty())
                        {
                            Verify::Fail(L"TracedEvents is empty!");
                            return nullptr;
                        }

                        auto lastEvent = writer_.TracedEvents[index];
                        try
                        {
                            std::shared_ptr<T> eventData = std::dynamic_pointer_cast<T>(lastEvent);
                            return eventData;
                        }
                        catch (const std::bad_cast &)
                        {
                            Verify::Fail(L"EventData has an unexpected type.");
                        }
                    }

                private:
                    Diagnostics::EventWriterStub & writer_;
                };

                class HealthStateItemHelper : public StateItemHelperBase
                {
                    DENY_COPY(HealthStateItemHelper);

                public:
                    HealthStateItemHelper(Health::IHealthSubsystemWrapper & health);

                    HealthSubsystemWrapperStub::ReplicaHealthEventInfo const & GetLastReplicaHealthEvent();
                    
                    void Reset();


                    void ValidateReplicaHealthEvent(
                        Health::ReplicaHealthEvent::Enum type,
                        Reliability::FailoverUnitId const & ftId,
                        bool isStateful,
                        uint64 replicaId,
                        uint64 instanceId);

                    void ValidateReplicaHealthEvent(
                        Health::ReplicaHealthEvent::Enum type,
                        Reliability::FailoverUnitId const & ftId,
                        bool isStateful,
                        uint64 replicaId,
                        uint64 instanceId,
                        Common::ErrorCodeValue::Enum error);

                    void ValidateNoReplicaHealthEvent();

                private:
                    HealthSubsystemWrapperStub & health_;
                };

                class RequestReceiverContextStateHelper : public StateItemHelperBase
                {
                    DENY_COPY(RequestReceiverContextStateHelper);

                public:
                    RequestReceiverContextStateHelper(FederationWrapperStub & federation);

                    void Reset();

                    template<StateManagement::MessageType::Enum T>
                    void ValidateReply(std::wstring const & tag, std::wstring const & body, StateManagement::ReadContext const & context)
                    {
                        RequestReceiverContextState * state = FindTag(tag);

                        if (state == nullptr)
                        {
                            return;
                        }

                        if (state->State != RequestReceiverContextState::Completed)
                        {
                            Verify::Fail(Common::wformatString("Request Receiver context for {0} is not completed", tag));
                            return;
                        }

                        ValidateMessageHelper<T>(stateItem_, state->ReplyMessage, body, context, StateManagement::ReadOption::None);
                    }

                    void ValidateError(std::wstring const & tag, Common::ErrorCode const & expected);

                private:
                    RequestReceiverContextState * FindTag(std::wstring const & tag);

                    FederationWrapperStub & federation_;
                };

                class GenerationStateHelper : public StateItemHelperBase
                {
                    DENY_COPY(GenerationStateHelper);

                public:
                    GenerationStateHelper(ReconfigurationAgent & ra);

                    void VerifyGeneration(GenerationNumberSource::Enum source, int nodeId, int sendGeneratio, int receiveGeneration, int proposedGeneration);
                    void VerifyGeneration(GenerationNumberSource::Enum source, GenerationInfo sendGeneration, GenerationInfo receiveGeneration, GenerationInfo proposedGeneration);
                    void VerifySendGeneration(GenerationNumberSource::Enum source, GenerationInfo sendGeneration);
                    void VerifyReceiveGeneration(GenerationNumberSource::Enum source, GenerationInfo receiveGeneration);
                    void VerifyProposedGeneration(GenerationNumberSource::Enum source, GenerationInfo proposedGeneration);

                    void SetGeneration(GenerationNumberSource::Enum source, int nodeId, int sendGeneratio, int receiveGeneration, int proposedGeneration);
                    void SetGeneration(GenerationNumberSource::Enum source, GenerationInfo sendGeneration, GenerationInfo receiveGeneration, GenerationInfo proposedGeneration);
                    void SetSendGeneration(GenerationNumberSource::Enum source, GenerationInfo sendGeneration);
                    void SetReceiveGeneration(GenerationNumberSource::Enum source, GenerationInfo receiveGeneration);
                    void SetProposedGeneration(GenerationNumberSource::Enum source, GenerationInfo proposedGeneration);

                    void Reset();

                    GenerationNumberSource::Enum GetOther(GenerationNumberSource::Enum source) const;
                private:

                    static void VerifyGeneration(GenerationInfo const & expected, Reliability::GenerationNumber const & actual);
                    static void SetGeneration(GenerationInfo const & expected, Reliability::GenerationNumber & actual);

                    GenerationState & GetGeneration(GenerationNumberSource::Enum source);

                    ReconfigurationAgent & ra_;
                };

                class PerformanceCounterHelper : public StateItemHelperBase
                {
                    DENY_COPY(PerformanceCounterHelper);

                public:
                    PerformanceCounterHelper(ReconfigurationAgent & ra);
                    PerformanceCounterHelper(Diagnostics::PerformanceCountersSPtr perfCounters);

                    void VerifyAveragePerformanceCounter(
                        Common::PerformanceCounterData & base,
                        Common::PerformanceCounterData & value,
                        int expected,
                        std::wstring const & name);

                    void VerifyAveragePerformanceCounterUnchanged(
                        Common::PerformanceCounterData & base,
                        Common::PerformanceCounterData & value,
                        std::wstring const & name);

                    void Reset();

                private:
                    Diagnostics::PerformanceCountersSPtr perfCounters_;
                };

                namespace RALifeCycleState
                {
                    enum Enum
                    {
                        Closed,
                        Open,
                        Closing,
                    };
                }

                class RALifeCycleStateHelper : public StateItemHelperBase
                {
                    DENY_COPY(RALifeCycleStateHelper);
                public:
                    RALifeCycleStateHelper(ReconfigurationAgent & ra);

                    void SetState(RALifeCycleState::Enum e);

                    void Reset();
                private:
                    ReconfigurationAgent & ra_;
                };

                class StateItemHelperCollection
                {
                    DENY_COPY(StateItemHelperCollection);

                public:
                    StateItemHelperCollection(UnitTestContext & utContext) :
                        fmMessageHelper_(utContext.FederationWrapper, false),
                        fmmMessageHelper_(utContext.FederationWrapper, true),
                        raMessageHelper_(utContext.FederationWrapper),
                        rapMessageHelper_(utContext.RapTransport),
                        messageRetryHelper_(RAStateItem::MessageRetryTimer, utContext.RA.ServiceTypeUpdateProcessorObj.MessageRetryWorkManager),
                        stateCleanupHelper_(RAStateItem::StateCleanupWorkManager, utContext.RA.StateCleanupWorkManager),
                        reconfigMessageRetryHelper_(RAStateItem::ReconfigurationMessageRetryWorkManager, utContext.RA.ReconfigurationMessageRetryWorkManager),
                        replicaCloseRetryHelper_(RAStateItem::ReplicaCloseRetryWorkManager, utContext.RA.ReplicaCloseMessageRetryWorkManager),
                        replicaOpenRetryHelper_(RAStateItem::ReplicaOpenRetryWorkManager, utContext.RA.ReplicaOpenRetryWorkManager),
                        updateServiceDescriptionRetryHelper_(RAStateItem::UpdateServiceDescriptionRetryWorkManager, utContext.RA.UpdateServiceDescriptionMessageRetryWorkManager),
                        requestReceiverHelper_(utContext.FederationWrapper),
                        reliabilityHelper_(utContext.Reliability),
                        generationHelper_(utContext.RA),
                        lastReplicaUpStateHelper_(utContext.RA),
                        lifeCycleStateHelper_(utContext.RA),
                        fmMessageRetryHelper_(utContext.RA), 
                        eventWriterStateItemHelper_(utContext.RA.EventWriter)
                    {
                        if (!utContext.Options.UseHostingPerfTestStub)
                        {
                            hostingHelper_ = Common::make_unique<HostingStateHelper>(utContext.Hosting);
                        }

                        if (!utContext.Options.UseHealthPerfTestStub)
                        {
                            healthHelper_ = Common::make_unique<HealthStateItemHelper>(utContext.RA.HealthSubsystem);
                        }
                    }

                    __declspec(property(get = get_FMMessageHelper)) FMMessageStateItemHelper & FMMessageHelper;
                    FMMessageStateItemHelper & get_FMMessageHelper() { return fmMessageHelper_; }

                    __declspec(property(get = get_FMMMessageHelper)) FMMessageStateItemHelper & FMMMessageHelper;
                    FMMessageStateItemHelper & get_FMMMessageHelper() { return fmmMessageHelper_; }

                    __declspec(property(get = get_RAMessageHelper)) RAMessageStateItemHelper & RAMessageHelper;
                    RAMessageStateItemHelper & get_RAMessageHelper() { return raMessageHelper_; }

                    __declspec(property(get = get_RAPMessageHelper)) RAPMessageStateItemHelper & RAPMessageHelper;
                    RAPMessageStateItemHelper & get_RAPMessageHelper() { return rapMessageHelper_; }

                    __declspec(property(get = get_MessageRetryHelper)) BackgroundWorkManagerWithRetryStateHelper & MessageRetryHelper;
                    BackgroundWorkManagerWithRetryStateHelper & get_MessageRetryHelper()  { return messageRetryHelper_; }

                    __declspec(property(get = get_StateCleanupHelper)) MultipleFTBackgroundWorkManagerStateHelper & StateCleanupHelper;
                    MultipleFTBackgroundWorkManagerStateHelper & get_StateCleanupHelper() { return stateCleanupHelper_; }

                    __declspec(property(get = get_ReconfigurationMessageRetryHelper)) MultipleFTBackgroundWorkManagerStateHelper & ReconfigurationMessageRetryHelper;
                    MultipleFTBackgroundWorkManagerStateHelper & get_ReconfigurationMessageRetryHelper() { return reconfigMessageRetryHelper_; }

                    __declspec(property(get = get_ReplicaCloseRetryHelper)) MultipleFTBackgroundWorkManagerStateHelper & ReplicaCloseRetryHelper;
                    MultipleFTBackgroundWorkManagerStateHelper & get_ReplicaCloseRetryHelper() { return replicaCloseRetryHelper_; }

                    __declspec(property(get = get_ReplicaOpenRetryHelper)) MultipleFTBackgroundWorkManagerStateHelper & ReplicaOpenRetryHelper;
                    MultipleFTBackgroundWorkManagerStateHelper & get_ReplicaOpenRetryHelper() { return replicaOpenRetryHelper_; }

                    __declspec(property(get = get_UpdateServiceDescriptionRetryHelper)) MultipleFTBackgroundWorkManagerStateHelper & UpdateServiceDescriptionRetryHelper;
                    MultipleFTBackgroundWorkManagerStateHelper & get_UpdateServiceDescriptionRetryHelper() { return updateServiceDescriptionRetryHelper_; }

                    __declspec(property(get = get_HostingHelper)) HostingStateHelper & HostingHelper;
                    HostingStateHelper & get_HostingHelper()
                    {
                        ASSERT_IF(hostingHelper_ == nullptr, "Default Hosting Stub not available. Cannot return hosting helper");
                        return *hostingHelper_;
                    }

                    __declspec(property(get = get_HealthHelper)) HealthStateItemHelper & HealthHelper;
                    HealthStateItemHelper & get_HealthHelper()
                    {
                        ASSERT_IF(healthHelper_ == nullptr, "Default health stub not available");
                        return *healthHelper_;
                    }

                    __declspec(property(get = get_RequestReceiverHelper)) RequestReceiverContextStateHelper & RequestReceiverHelper;
                    RequestReceiverContextStateHelper & get_RequestReceiverHelper() { return requestReceiverHelper_; }

                    __declspec(property(get = get_ReliabilityHelper)) ReliabilityStateHelper & ReliabilityHelper;
                    ReliabilityStateHelper & get_ReliabilityHelper() { return reliabilityHelper_; }

                    __declspec(property(get = get_GenerationHelper)) GenerationStateHelper & GenerationHelper;
                    GenerationStateHelper & get_GenerationHelper() { return generationHelper_; }

                    __declspec(property(get = get_LastReplicaUpHelper)) LastReplicaUpStateHelper & LastReplicaUpHelper;
                    LastReplicaUpStateHelper & get_LastReplicaUpHelper() { return lastReplicaUpStateHelper_; }

                    __declspec(property(get = get_LifeCycleStateHelper)) RALifeCycleStateHelper & LifeCycleStateHelper;
                    RALifeCycleStateHelper & get_LifeCycleStateHelper() { return lifeCycleStateHelper_; }
                    
                    __declspec(property(get = get_FMMessageRetryHelperObj)) FMMessageRetryHelper & FMMessageRetryHelperObj;
                    FMMessageRetryHelper & get_FMMessageRetryHelperObj() { return fmMessageRetryHelper_; }

                    __declspec(property(get = get_EventWriterStateItemHelper)) EventWriterStateItemHelper & EventWriterHelper;
                    EventWriterStateItemHelper & get_EventWriterStateItemHelper() { return eventWriterStateItemHelper_; }

                    void ResetAll()
                    {
                        fmMessageHelper_.Reset();
                        fmmMessageHelper_.Reset();
                        raMessageHelper_.Reset();
                        rapMessageHelper_.Reset();

                        messageRetryHelper_.Reset();

                        stateCleanupHelper_.Reset();
                        reconfigMessageRetryHelper_.Reset();
                        replicaCloseRetryHelper_.Reset();
                        replicaOpenRetryHelper_.Reset();
                        updateServiceDescriptionRetryHelper_.Reset();

                        if (hostingHelper_ != nullptr)
                        {
                            hostingHelper_->Reset();
                        }

                        if (healthHelper_ != nullptr)
                        {
                            healthHelper_->Reset();
                        }

                        requestReceiverHelper_.Reset();

                        reliabilityHelper_.Reset();
                        generationHelper_.Reset();

                        lastReplicaUpStateHelper_.Reset();

                        fmMessageRetryHelper_.Reset();
                        lifeCycleStateHelper_.Reset();

                        eventWriterStateItemHelper_.Reset();
                    }

                private:
                    FMMessageStateItemHelper fmMessageHelper_;
                    FMMessageStateItemHelper fmmMessageHelper_;
                    RAMessageStateItemHelper raMessageHelper_;
                    RAPMessageStateItemHelper rapMessageHelper_;

                    BackgroundWorkManagerWithRetryStateHelper messageRetryHelper_;

                    MultipleFTBackgroundWorkManagerStateHelper replicaOpenRetryHelper_;
                    MultipleFTBackgroundWorkManagerStateHelper stateCleanupHelper_;
                    MultipleFTBackgroundWorkManagerStateHelper reconfigMessageRetryHelper_;
                    MultipleFTBackgroundWorkManagerStateHelper replicaCloseRetryHelper_;
                    MultipleFTBackgroundWorkManagerStateHelper updateServiceDescriptionRetryHelper_;

                    std::unique_ptr<HostingStateHelper> hostingHelper_;
                    std::unique_ptr<HealthStateItemHelper> healthHelper_;

                    RequestReceiverContextStateHelper requestReceiverHelper_;
                    LastReplicaUpStateHelper lastReplicaUpStateHelper_;
                    ReliabilityStateHelper reliabilityHelper_;
                    GenerationStateHelper generationHelper_;

                    RALifeCycleStateHelper lifeCycleStateHelper_;
                    FMMessageRetryHelper fmMessageRetryHelper_;

                    EventWriterStateItemHelper eventWriterStateItemHelper_;
                };
            }
        }
    }
}
