// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ComTestStateProvider.h"
#include "ComTestOperation.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace ReplicationUnitTest
{
    using namespace Common;
    using namespace std;
    using namespace Reliability::ReplicationComponent;

    static Common::StringLiteral const ReliableOperationSenderTestSource("ReliableOpSenderTest");
    static Common::StringLiteral const OperationLatencyListTest("OperationLatencyListTest");

    typedef std::shared_ptr<REConfig> REConfigSPtr;
    class TestReliableOperationSender
    {
    protected:
        static REConfigSPtr CreateGenericConfig();
        static ReliableOperationSender::OperationLatencyList CreateOperationLatencyList(int itemCount);
        static void VerifyOperationListStopWatchesState(
            ReliableOperationSender::OperationLatencyList list, 
            FABRIC_SEQUENCE_NUMBER receiveWatchMustBeStoppedAfter, 
            FABRIC_SEQUENCE_NUMBER applyWatchMustBeStoppedAfter);
    };

    class ReliableOperationSenderWrapper;
    typedef shared_ptr<ReliableOperationSenderWrapper> ReliableOperationSenderWrapperSPtr;

    enum TimerState { TimerEnabled, TimerDisabled, UnknownTimerState };
    enum SendWindowSizeState { SwsConstant, SwsReduced, SwsIncreased };
    
    class ReliableOperationSenderWrapper : public ComponentRoot
    {
    public:

        static wstring const PartitionId;
        static wstring const Purpose;
        static DWORD const WaitMsec;
        static int const CheckRequestACKRetryCount;
        
        ReliableOperationSenderWrapper(
            std::wstring const & description,
            ReplicationEndpointId const & endpoint,
            FABRIC_REPLICA_ID const & id, 
            REConfigSPtr && config);
        ~ReliableOperationSenderWrapper();
        
        void AddOp(FABRIC_SEQUENCE_NUMBER lsn);
        
        // Add operations [fromLSN, toLSN] one by one, shuffled if so specified
        void AddOps(
            FABRIC_SEQUENCE_NUMBER fromLSN,
            FABRIC_SEQUENCE_NUMBER toLSN,
            bool shuffle);

        void Ack(
            FABRIC_SEQUENCE_NUMBER ackedReceivedLSN, 
            FABRIC_SEQUENCE_NUMBER ackedQuorumLSN, 
            bool expectProgress = true);
        void CheckSender(
            FABRIC_SEQUENCE_NUMBER receivedLSN, 
            FABRIC_SEQUENCE_NUMBER quorumLSN, 
            wstring const & expectedOps,
            TimerState expectedTimerState,
            SendWindowSizeState expectedSwsState = SwsConstant);

        void CheckSws(size_t sws, SendWindowSizeState expectedSwsState);
        
        void CheckRequestAck(uint64 minExpectedValue);
        void WaitUntilSWSReduced(size_t sws);

        void Open();

    private:
        REConfigSPtr config_;
        OperationQueue queue_;
        ReliableOperationSender sender_;
        vector<FABRIC_SEQUENCE_NUMBER> operationsSent_;
        atomic_uint64 requestAckCount_;
        size_t sendWindowSize_;
    };

    wstring const ReliableOperationSenderWrapper::PartitionId(Common::Guid::NewGuid().ToString());
    wstring const ReliableOperationSenderWrapper::Purpose(L"REPL");
    DWORD const ReliableOperationSenderWrapper::WaitMsec = 500;
    int const ReliableOperationSenderWrapper::CheckRequestACKRetryCount = 20;

    /***********************************
    * TestReliableOperationSender methods
    **********************************/

    BOOST_FIXTURE_TEST_SUITE(TestReliableOperationSenderSuite,TestReliableOperationSender)

    BOOST_AUTO_TEST_CASE(TestReceiveACKProgress)
    {
        ComTestOperation::WriteInfo(
            ReliableOperationSenderTestSource,
            "Start TestReceiveACKProgress");

        auto config = CreateGenericConfig();
        config->InitialCopyQueueSize = 16;
        config->MaxCopyQueueSize = 16;
        auto wrapperSPtr = std::make_shared<ReliableOperationSenderWrapper>(L"TestReceiveACKProgress",ReplicationEndpointId(),1, move(config));

        ReliableOperationSenderWrapper & wrapper = *wrapperSPtr;
        wrapper.Open();
        wrapper.AddOp(1);
        wrapper.CheckSender(-1, -1, L"1;", TimerEnabled);
        wrapper.AddOp(2);
        wrapper.CheckSender(-1, -1, L"1;2;", TimerEnabled);
        wrapper.AddOp(3);
        wrapper.CheckSender(-1, -1, L"1;2;3;", TimerEnabled);
        
        // Expect progress when received ACK is moved forward
        wrapper.Ack(1, 0, true);
        wrapper.CheckSender(1, 0, L"2;3;", TimerEnabled, SwsIncreased);

        // Expect progress when received ACK is moved forward
        wrapper.Ack(2, 0, true);
        wrapper.CheckSender(2, 0, L"3;", TimerEnabled, SwsIncreased);

        wrapper.AddOp(4);
        wrapper.CheckSender(2, 0, L"3;4;", TimerEnabled);

        // Expect progress when quorum ACK is moved forward 
        wrapper.Ack(2, 1, true);
        wrapper.CheckSender(2, 1, L"3;4;", TimerEnabled, SwsIncreased);

        // Expect progress when quorum ACK AND receive ACK is moved forward
        wrapper.Ack(3, 2, true);
        wrapper.CheckSender(3, 2, L"4;", TimerEnabled, SwsIncreased);

        wrapper.Ack(4, 4, true);
        wrapper.CheckSender(4, 4, L"", TimerDisabled, SwsIncreased);
    }

    BOOST_AUTO_TEST_CASE(TestAddRemove)
    {
        ComTestOperation::WriteInfo(
            ReliableOperationSenderTestSource,
            "Start TestAddRemove");

        auto config = CreateGenericConfig();
        config->InitialCopyQueueSize = 16;
        config->MaxCopyQueueSize = 16;
        auto wrapperSPtr = std::make_shared<ReliableOperationSenderWrapper>(L"TestAddRemove",ReplicationEndpointId(), 1, move(config));

        ReliableOperationSenderWrapper & wrapper = *wrapperSPtr;
        wrapper.Open();
        wrapper.AddOp(1);
        wrapper.CheckSender(-1, -1, L"1;", TimerEnabled);
        wrapper.AddOp(2);
        wrapper.CheckSender(-1, -1, L"1;2;", TimerEnabled);
        wrapper.AddOp(3);
        wrapper.CheckSender(-1, -1, L"1;2;3;", TimerEnabled);
        
        wrapper.Ack(3, 0);
        wrapper.CheckSender(3, 0, L"", TimerEnabled);

        wrapper.Ack(3, 1);
        wrapper.CheckSender(3, 1, L"", TimerEnabled);
     
        wrapper.CheckRequestAck(0);
        
        wrapper.AddOp(4);
        wrapper.WaitUntilSWSReduced(16);
        // Since more than one callback was invoked,
        // sws was reduced
        wrapper.CheckSender(3, 1, L"4;", TimerEnabled, SwsReduced);

        wrapper.Ack(4, 2);
        wrapper.CheckSender(4, 2, L"", TimerEnabled, SwsIncreased);
                
        wrapper.AddOp(5);
        wrapper.CheckSender(4, 2, L"5;", TimerEnabled);

        wrapper.AddOp(6);
        wrapper.CheckSender(4, 2, L"5;6;", TimerEnabled);

        wrapper.Ack(4, 4);
        wrapper.CheckSender(4, 4, L"5;6;", TimerEnabled, SwsIncreased);
        
        wrapper.Ack(5, 5);
        wrapper.CheckSender(5, 5, L"6;", TimerEnabled, SwsIncreased);

        wrapper.Ack(6, 5);
        wrapper.CheckSender(6, 5, L"", TimerEnabled, SwsIncreased);
        
        wrapper.Ack(6, 6);
        wrapper.CheckSender(6, 6, L"", TimerDisabled, SwsIncreased);
    }

    BOOST_AUTO_TEST_CASE(TestAddMoreThanContinuousSend)
    {
        ComTestOperation::WriteInfo(
            ReliableOperationSenderTestSource,
            "Start TestAddMoreThanContinuousSend");

        // Send window size is 4, so we can only send max 4 messages at one time
        auto config = CreateGenericConfig();
        config->InitialCopyQueueSize = 4;
        config->MaxCopyQueueSize = 4;
        auto wrapperSPtr = std::make_shared<ReliableOperationSenderWrapper>(L"TestAddMoreThanContinuousSend", ReplicationEndpointId(), 1, move(config));

        ReliableOperationSenderWrapper & wrapper = *wrapperSPtr;
        wrapper.Open();
        wrapper.AddOps(1, 5, false /*shuffle*/);
        // 4 items should be sent, the 5th will be sent after an ACK
        wrapper.CheckSender(-1, -1, L"1;2;3;4;5.0;", TimerEnabled);
        
        // Ack the first operations, which makes room for the last item to be sent
        wrapper.Ack(4, 4);
        wrapper.CheckSender(4, 4, L"5;", TimerEnabled);
        
        // Add 4 more operations shuffled
        wrapper.AddOps(6, 8, true /*shuffle*/);

        // Allow callback to be fired, so 2 or all new operations are sent 
        wrapper.WaitUntilSWSReduced(4);
        // Depending on the timing of the callback, we may keep sending 4, 5, 6 
        // and 7 may never be sent. 
        // Make sure all operations are sent by ACK-ing 5
        wrapper.Ack(5, 5);
        wrapper.CheckSender(5, 5, L"6;7;8;", TimerEnabled);
                
        wrapper.Ack(8, 8);
        wrapper.CheckSender(8, 8, L"", TimerDisabled);
    }

    BOOST_AUTO_TEST_CASE(TestMaxSendingWindowSize1)
    {
        ComTestOperation::WriteInfo(
            ReliableOperationSenderTestSource,
            "Start TestMaxSendingWindowSize1");

        auto config = CreateGenericConfig();
        config->RetryInterval = TimeSpan::FromMilliseconds(1000);
        config->InitialCopyQueueSize = 8;
        config->MaxCopyQueueSize = 0;

        auto wrapperSPtr = std::make_shared<ReliableOperationSenderWrapper>(L"TestMaxSendingWindowSize1", ReplicationEndpointId(), 1, move(config));
        ReliableOperationSenderWrapper & wrapper = *wrapperSPtr;
        wrapper.Open();
        wrapper.AddOps(1, 1, false /*shuffle*/);
        wrapper.Ack(1, 1);

        wrapper.CheckSws(16, SendWindowSizeState::SwsIncreased);
    }
 
    BOOST_AUTO_TEST_CASE(TestMaxSendingWindowSize2)
    {
        ComTestOperation::WriteInfo(
            ReliableOperationSenderTestSource,
            "Start TestMaxSendingWindowSize2");

        auto config = CreateGenericConfig();
        config->RetryInterval = TimeSpan::FromMilliseconds(1000);
        config->InitialCopyQueueSize = 2048;
        config->MaxCopyQueueSize = 0;

        auto wrapperSPtr = std::make_shared<ReliableOperationSenderWrapper>(L"TestMaxSendingWindowSize2", ReplicationEndpointId(), 1, move(config));
        ReliableOperationSenderWrapper & wrapper = *wrapperSPtr;
        wrapper.Open();
        wrapper.AddOps(1, 1, false /*shuffle*/);
        wrapper.Ack(1, 1);
        wrapper.AddOps(2, 2, false /*shuffle*/);
        wrapper.Ack(2, 2);
        wrapper.AddOps(3, 3, false /*shuffle*/);
        wrapper.Ack(3, 3);
        wrapper.AddOps(4, 4, false /*shuffle*/);
        wrapper.Ack(4, 4);
        wrapper.AddOps(5, 5, false /*shuffle*/);
        wrapper.Ack(5, 5);
        wrapper.AddOps(6, 6, false /*shuffle*/);
        wrapper.Ack(6, 6);
        
        // MAX must be N times the initial size. N = Constant
        wrapper.CheckSws(ReliableOperationSender::DEFAULT_MAX_SWS_FACTOR_WHEN_0 * 2048, SendWindowSizeState::SwsIncreased);
    }

    BOOST_AUTO_TEST_CASE(TestMaxSendingWindowSize3)
    {
        //Repeatitive acks with no progress should not increase the send window size.
        ComTestOperation::WriteInfo(
            ReliableOperationSenderTestSource,
            "Start TestMaxSendingWindowSize3");

        auto config = CreateGenericConfig();
        config->RetryInterval = TimeSpan::FromMilliseconds(1000);
        config->InitialCopyQueueSize = 8;
        config->MaxCopyQueueSize = 0;

        auto wrapperSPtr = std::make_shared<ReliableOperationSenderWrapper>(L"TestMaxSendingWindowSize3", ReplicationEndpointId(), 1, move(config));
        ReliableOperationSenderWrapper & wrapper = *wrapperSPtr;
        wrapper.Open();
        wrapper.AddOps(1, 1, false /*shuffle*/);
        wrapper.Ack(1, 1);
        wrapper.Ack(1, 1, false);
        wrapper.Ack(1, 1, false);

        wrapper.CheckSws(16, SendWindowSizeState::SwsIncreased);
    }

    BOOST_AUTO_TEST_CASE(TestOperationLatencyList)
    {
        auto temp1 = TimeSpan::Zero;
        auto temp2 = TimeSpan::Zero;
        {
            ComTestOperation::WriteInfo(
                OperationLatencyListTest,
                "Start TestOperationLatencyList1");

            auto testList = CreateOperationLatencyList(100);

            testList.OnAck(2, 0);
            testList.OnAck(3, 0);
            testList.OnAck(4, 0);

            VerifyOperationListStopWatchesState(testList, 4, 0);

            testList.OnAck(4, 2);
            VerifyOperationListStopWatchesState(testList, 4, 2);
            testList.ComputeAverageAckDuration(temp1, temp2);
            VERIFY_IS_TRUE(testList.Count == 98);

            testList.OnAck(100, 99);
            VerifyOperationListStopWatchesState(testList, 100, 99);
            VERIFY_IS_TRUE(testList.Count == 98);
            testList.ComputeAverageAckDuration(temp1, temp2);
            VERIFY_IS_TRUE(testList.Count == 1);

            // Duplicate ack is okay
            testList.OnAck(100, 99);
            VerifyOperationListStopWatchesState(testList, 100, 99);
            VERIFY_IS_TRUE(testList.Count == 1);

            testList.OnAck(100, 100);
            VERIFY_IS_TRUE(testList.Count == 1);
            VerifyOperationListStopWatchesState(testList, 100, 100);
            testList.ComputeAverageAckDuration(temp1, temp2);
            VERIFY_IS_TRUE(testList.Count == 0);
        }

        { 
            ComTestOperation::WriteInfo(
                OperationLatencyListTest,
                "Start TestOperationLatencyList2");

            auto testList = CreateOperationLatencyList(100);

            testList.OnAck(100, 0);
            VerifyOperationListStopWatchesState(testList, 100, 0);
            testList.ComputeAverageAckDuration(temp1, temp2);
            VERIFY_IS_TRUE(testList.Count == 100);

            testList.OnAck(100, 99);
            VerifyOperationListStopWatchesState(testList, 100, 99);
            VERIFY_IS_TRUE(testList.Count == 100);
            testList.ComputeAverageAckDuration(temp1, temp2);
            VERIFY_IS_TRUE(testList.Count == 1);
        }
 
        {
            ComTestOperation::WriteInfo(
                OperationLatencyListTest,
                "Start TestOperationLatencyList3");

            auto testList = CreateOperationLatencyList(100);

            testList.OnAck(100, 100);
            VerifyOperationListStopWatchesState(testList, 100, 100);
            testList.ComputeAverageAckDuration(temp1, temp2);
            VERIFY_IS_TRUE(testList.Count == 0);
        }
    }
    BOOST_AUTO_TEST_SUITE_END()

    ReliableOperationSender::OperationLatencyList TestReliableOperationSender::CreateOperationLatencyList(int itemCount)
    {
        ReliableOperationSender::OperationLatencyList opList;
        std::vector<FABRIC_SEQUENCE_NUMBER> lsns;
        for (int i = 1; i <= itemCount; i++)
            lsns.push_back(i);

        random_shuffle(lsns.begin(), lsns.end());

        for (int i = 0; i < itemCount; i++)
        {
            ComTestOperation::WriteInfo(
                OperationLatencyListTest,
                "Adding LSN {0}",
                lsns[i]);

            opList.Add(lsns[i]);
        }

        return opList;
    }
        
    void TestReliableOperationSender::VerifyOperationListStopWatchesState(
        ReliableOperationSender::OperationLatencyList list, 
        FABRIC_SEQUENCE_NUMBER receiveWatchMustBeStoppedAfter,
        FABRIC_SEQUENCE_NUMBER applyWatchMustBeStoppedAfter)
    {
        auto items = list.Test_GetItems();

        auto it = items.begin();

        while (it != end(items))
        {
            if (it->first <= applyWatchMustBeStoppedAfter)
            {
                VERIFY_ARE_EQUAL2(it->second.first.IsRunning, false);
                VERIFY_ARE_EQUAL2(it->second.second.IsRunning, false);
            }
            else if (it->first <= receiveWatchMustBeStoppedAfter)
            {
                VERIFY_ARE_EQUAL2(it->second.first.IsRunning, false);
                VERIFY_ARE_EQUAL2(it->second.second.IsRunning, true);
            }
            else
            {
                VERIFY_ARE_EQUAL2(it->second.first.IsRunning, true);
                VERIFY_ARE_EQUAL2(it->second.second.IsRunning, true);
            }

            ++it;
        }

    }

    REConfigSPtr TestReliableOperationSender::CreateGenericConfig()
    {
        REConfigSPtr config = std::make_shared<REConfig>();

        config->RetryInterval = TimeSpan::FromMilliseconds(1000);
        config->QueueHealthMonitoringInterval = TimeSpan::Zero;
        config->SecondaryClearAcknowledgedOperations = (Random().NextDouble() > 0.5)? true : false; 
        return config;
    }

    /***********************************
    * ReliableOperationSenderWrapper methods
    **********************************/
    ReliableOperationSenderWrapper::ReliableOperationSenderWrapper(
        std::wstring const & description,
        ReplicationEndpointId const & endpoint,
        FABRIC_REPLICA_ID const & id, 
        REConfigSPtr && config) 
        :   config_(move(config)),
            queue_(
                PartitionId, 
                description, 
                config_->InitialCopyQueueSize,
                0,
                config_->MaxReplicationMessageSize,
                0,
                0,
                false /*hasPersistedState*/, 
                true /*cleanOnComplete*/, 
                true /*ignoreCommit*/, 
                1 /*startSeq*/,
                nullptr),
            sender_(
                REInternalSettings::Create(nullptr, config_),
                config_->InitialCopyQueueSize, 
                config_->MaxCopyQueueSize, 
                PartitionId,
                Purpose,
                endpoint,
                id,
                -1),
            operationsSent_(),
            requestAckCount_(0),
            sendWindowSize_(static_cast<size_t>(config_->InitialCopyQueueSize))
    {
    }

    void ReliableOperationSenderWrapper::Open()
    {
        sender_.Open<ComponentRootSPtr>(
            this->CreateComponentRoot(),
            [this](ComOperationCPtr const & op, bool requestAck, FABRIC_SEQUENCE_NUMBER) 
            { 
                if (requestAck)
                {
                    ++requestAckCount_;
                    ComTestOperation::WriteInfo(
                        ReliableOperationSenderTestSource,
                        "Request ACK {0}",
                        requestAckCount_.load());

                    return true;
                }

                ComTestOperation::WriteInfo(
                    ReliableOperationSenderTestSource,
                    "Send {0}",
                    op->SequenceNumber);
                this->operationsSent_.push_back(op->SequenceNumber);
                return true;
            });
    }

    ReliableOperationSenderWrapper::~ReliableOperationSenderWrapper() 
    {
        sender_.Close();
    }

    void ReliableOperationSenderWrapper::AddOps(
        FABRIC_SEQUENCE_NUMBER fromLSN,
        FABRIC_SEQUENCE_NUMBER toLSN,
        bool shuffle)
    {
        // Enqueue the operations in order, 
        // but add them in sender shuffled if so specified
        vector<ComOperationRawPtr> ops;
        for (auto i = fromLSN; i <= toLSN; ++i)
        {
            ComPointer<IFabricOperationData> op = make_com<ComTestOperation,IFabricOperationData>(L"Dummy op generated by ReliableOperationSender test");   
            FABRIC_OPERATION_METADATA metadata;
            metadata.Type = FABRIC_OPERATION_TYPE_NORMAL;
            metadata.SequenceNumber = i;
            metadata.Reserved = NULL;
            ComPointer<ComOperation> opPointer = make_com<ComUserDataOperation,ComOperation>(
                move(op), 
                metadata);

            ComOperationRawPtr opRawPtr = opPointer.GetRawPointer();
            ErrorCode error = queue_.TryEnqueue(opPointer);
            VERIFY_IS_TRUE_FMT(error.IsSuccess(), "Enqueued {0} returned error {1}", i, error);
            ops.push_back(opRawPtr);
        }

        if (shuffle)
        {
            if (ops.size() == 2)
            {
                swap(ops[0], ops[1]);
            }
            else
            {
                random_shuffle(ops.begin(), ops.end());
            }

            wstring shuffledOps;
            StringWriter writer(shuffledOps);
            for(size_t i = 0; i < ops.size(); ++i)
            {
                writer.Write(ops[i]->SequenceNumber);
                writer.Write(L';');
            }

            Trace.WriteInfo(ReliableOperationSenderTestSource, "Shuffled ops: \"{0}\"", shuffledOps);
        }

        for (auto opRawPtr : ops)
        {
            sender_.Add(opRawPtr, -1);
        }
    }
        
    void ReliableOperationSenderWrapper::AddOp(
        FABRIC_SEQUENCE_NUMBER lsn)
    {
        ComPointer<IFabricOperationData> op = make_com<ComTestOperation,IFabricOperationData>(L"Dummy op generated by ReliableOperationSender test");   
        FABRIC_OPERATION_METADATA metadata;
        metadata.Type = FABRIC_OPERATION_TYPE_NORMAL;
        metadata.SequenceNumber = lsn;
        metadata.Reserved = NULL;
        ComPointer<ComOperation> opPointer = make_com<ComUserDataOperation,ComOperation>(
            move(op), 
            metadata);

        ComOperationRawPtr opRawPtr = opPointer.GetRawPointer();
        ErrorCode error = queue_.TryEnqueue(opPointer);
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "Enqueued {0} returned error {1}", lsn, error);

        sender_.Add(opRawPtr, -1);
    }

    void ReliableOperationSenderWrapper::Ack(
        FABRIC_SEQUENCE_NUMBER ackedReceivedLSN, 
        FABRIC_SEQUENCE_NUMBER ackedQuorumLSN,
        bool expectProgress)
    {
        bool progress = sender_.ProcessOnAck(ackedReceivedLSN, ackedQuorumLSN);
        VERIFY_ARE_EQUAL_FMT(
            progress,
            expectProgress,
            "Process ACK {0}.{1} returned {2}",
                ackedReceivedLSN, ackedQuorumLSN, progress);
    }

    void ReliableOperationSenderWrapper::CheckSender(
        FABRIC_SEQUENCE_NUMBER receivedLSN,
        FABRIC_SEQUENCE_NUMBER quorumLSN,
        wstring const & expectedOps,
        TimerState expectedTimerState,
        SendWindowSizeState expectedSwsState)
    {
        Trace.WriteInfo(ReliableOperationSenderTestSource, "----------------Check sender");
        FABRIC_SEQUENCE_NUMBER actualReceivedLSN;
        FABRIC_SEQUENCE_NUMBER actualQuorumLSN;
        Common::DateTime time;
        sender_.GetProgress(actualReceivedLSN, actualQuorumLSN, time);

        VERIFY_ARE_EQUAL_FMT(
            actualQuorumLSN,
            quorumLSN,
            "Quorum LSN: expected {0}, actual {1}", quorumLSN, actualQuorumLSN);
        VERIFY_ARE_EQUAL_FMT(
            actualReceivedLSN,
            receivedLSN,
            "Received LSN: expected {0}, actual {1}", receivedLSN, actualReceivedLSN);

        list<std::pair<ComOperationRawPtr, Common::DateTime>> ops;
        bool timerActive;
        size_t sws;
        sender_.Test_GetPendingOperationsCopy(ops, timerActive, sws);
        if (expectedTimerState == TimerEnabled)
        {
            VERIFY_IS_TRUE(timerActive);
        }
        else if (expectedTimerState == TimerDisabled)
        {
            VERIFY_IS_FALSE(timerActive);
        }

        CheckSws(sws, expectedSwsState);
        
        wstring actualOps;
        StringWriter writer(actualOps);

        auto it = ops.begin();
        for (; it != ops.end(); ++it)
        {
            writer.Write(it->first->SequenceNumber);
            if (it->second == DateTime::Zero)
            {
                writer.Write(".0");
            }
        
            writer << L';';
        }

        VERIFY_ARE_EQUAL_FMT(
            actualOps,
            expectedOps,
            "Pending ops: expected \"{0}\", actual \"{1}\"", expectedOps, actualOps);

        wstring actual;
        StringWriter writer2(actual);
        for(size_t i = 0; i < operationsSent_.size(); ++i)
        {
            writer2.Write(operationsSent_[i]);
            writer2.Write(L';');
        }

        Trace.WriteInfo(ReliableOperationSenderTestSource, "Operations sent: \"{0}\"", actual);
    }

    static wstring PrintSwsState(SendWindowSizeState state)
    {
        switch (state)
        {
            case SwsConstant: return L"Constant";
            case SwsIncreased: return L"Increase";
            case SwsReduced: return L"Reduce";
            default: return L"Unknown";
        }
    }

    void ReliableOperationSenderWrapper::CheckSws(
        size_t sws,
        SendWindowSizeState expectedSwsState)
    {
        Trace.WriteInfo(
            ReliableOperationSenderTestSource,
            "Previous sws {0}, new sws {1}. Expected to {2}",
            sendWindowSize_, 
            sws, 
            PrintSwsState(expectedSwsState).c_str());
        if (expectedSwsState == SwsConstant)
        {
            if (sws != sendWindowSize_ && 
                sws != static_cast<size_t>(config_->InitialCopyQueueSize) &&
                sws != sendWindowSize_ / 2 /*accept one extra callback*/)
            {
                VERIFY_FAIL(L"sws is not constant or defaulted to start capacity");
            }
        }
        else if (expectedSwsState == SwsReduced)
        {
            if (sws >= sendWindowSize_ && sws != 1)
            {
                VERIFY_FAIL(L"sws wasn't reduced");
            }
        }
        else // increased
        {
            // A callback can be executed at any time after we decide to check
            // so accept == as valid here
            if (sws < sendWindowSize_)
            {
                VERIFY_FAIL(L"sws wasn't increased or at least kept constant");
            }
        }

        sendWindowSize_ = sws;
    }

    void ReliableOperationSenderWrapper::CheckRequestAck(uint64 minExpectedValue)
    {
        uint64 requestAckCount = 0;
        for (int retry = 0; retry < CheckRequestACKRetryCount; retry++)
        {
            // Sleep more than retry interval and 
            // the timer callback should request ack
            Sleep(ReliableOperationSenderWrapper::WaitMsec);
    
            requestAckCount = requestAckCount_.load();
            if (requestAckCount > minExpectedValue)
            {
                break;
            }
            Trace.WriteInfo(ReliableOperationSenderTestSource, "Sleep {0} times in CheckRequestAck", retry + 1);
        }

        VERIFY_IS_TRUE_FMT(
            requestAckCount > minExpectedValue,
            "requestAckCount is {0}", requestAckCount);
    }

    void ReliableOperationSenderWrapper::WaitUntilSWSReduced(size_t sws)
    {
        size_t reducedSws;
        for (int retry = 0; retry < CheckRequestACKRetryCount; retry++)
        {
            // Sleep more than retry interval and 
            // the timer callback should request ack
            Sleep(ReliableOperationSenderWrapper::WaitMsec);
    
            list<std::pair<ComOperationRawPtr, Common::DateTime>> ops;
            bool timerActive;
            sender_.Test_GetPendingOperationsCopy(ops, timerActive, reducedSws);

            if (sws > reducedSws)
            {
                break;
            }
            Trace.WriteInfo(ReliableOperationSenderTestSource, "Sleep {0} times in WaitUntilSWSReduced", retry + 1);
        }

        VERIFY_IS_TRUE_FMT(
            sws > reducedSws,
            "ReducedSWS is {0}", reducedSws);
    }
}
