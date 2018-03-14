// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TestHealthClient.h"
#include "ComTestStatefulServicePartition.h"
#include "ComTestOperation.h"
#include "ComTestStateProvider.h"
#include "ComProxyTestReplicator.h"
#include "ComReplicator.h"
#include "TestChangeRole.h"
#include "testFabricAsyncCallbackHelper.h"

namespace ReplicationUnitTest
{
    using namespace Common;
    using namespace std;
    using namespace Reliability::ReplicationComponent;

    static Common::StringLiteral const PublicTestSource("TESTPublic");

    class TestPublicApi
    {
    protected:
        TestPublicApi() { BOOST_REQUIRE(Setup()); }
        TEST_CLASS_SETUP(Setup)
        void TestStateReplicator(bool hasPersistedState);
        void TestDataLoss(bool changeStateOnDataLoss);

        class DummyRoot : public Common::ComponentRoot
        {
        };
    public:
        static Common::ComponentRoot const & GetRoot();
    };

    namespace {
    class ReplicatorTestWrapper
    {
    public:
        ReplicatorTestWrapper(
            bool hasPersistedState,
            int64 numberOfCopyOps,
            int64 numberOfCopyContextOps,
            bool changeStateOnDataLoss) 
            :   partitionId_(Common::Guid::NewGuid()),
                failoverUnitId_(partitionId_.ToString()),
                factory_(TestPublicApi::GetRoot()),
                primaryId_(0LL),
                secondaryIds_(),
                epoch_(),
                hasPersistedState_(hasPersistedState),
                numberOfCopyOps_(numberOfCopyOps),
                numberOfCopyContextOps_(numberOfCopyContextOps),
                changeStateOnDataLoss_(changeStateOnDataLoss),
                primary_(),
                secondaries_(),
                stateProvider_(Common::make_com<ComTestStateProvider,::IFabricStateProvider>(
                    numberOfCopyOps, 
                    numberOfCopyContextOps, 
                    TestPublicApi::GetRoot(),
                    changeStateOnDataLoss))
        {
            VERIFY_IS_TRUE(factory_.Open(L"0").IsSuccess(), L"ComReplicatorFactory opened");
            epoch_.DataLossNumber = 1;
            epoch_.ConfigurationNumber = 100;
        }

        ~ReplicatorTestWrapper()
        {
            VERIFY_IS_TRUE(factory_.Close().IsSuccess(), L"ComReplicatorFactory closed");
        }

        ComProxyTestReplicatorSPtr const & GetSecondary(size_t index)
        {
            ASSERT_IF(index >= secondaries_.size(), "Incorrect index {0}: {1} secondaries", index, secondaries_.size());
            return secondaries_[index];
        }

        ComProxyTestReplicatorSPtr const & GetPrimary()
        {
            return primary_;
        }


        void CreatePrimary(
            ComTestStatefulServicePartition ** partition,
            Replicator::ReplicatorInternalVersion version = Replicator::V1,
            bool batchEnabled = false)
        {
            ComProxyTestReplicator::CreateComProxyTestReplicator(
                partitionId_,
                factory_,
                primaryId_,
                stateProvider_,
                hasPersistedState_,
                false /*supportsParallelStreams*/,
                version, /*Replicator::Version*/
                batchEnabled,
                primary_,
                partition);
            primary_->Open();
            primary_->ChangeRole(epoch_, ::FABRIC_REPLICA_ROLE_PRIMARY);
        }

        void CreateIdle(
            ::FABRIC_REPLICA_ID secondaryId,
            __out ComProxyTestReplicatorSPtr & secondary,
            Replicator::ReplicatorInternalVersion version = Replicator::V1,
            bool batchEnabled = false)
        {
            ComTestStatefulServicePartition * partition = nullptr;
            CreateIdle(secondaryId, stateProvider_, secondary, &partition, version, batchEnabled);
        }

        void CreateIdle(
            ::FABRIC_REPLICA_ID secondaryId,
            ComPointer<IFabricStateProvider> const & stateProvider,
            __out ComProxyTestReplicatorSPtr & secondary,
            __out ComTestStatefulServicePartition ** partition,
            Replicator::ReplicatorInternalVersion version = Replicator::V1,
            bool batchEnabled = false)
        {
            bool supportsParallelStreams = false;
            ComProxyTestReplicator::CreateComProxyTestReplicator(
                partitionId_,
                factory_,
                secondaryId,
                stateProvider,
                hasPersistedState_,
                supportsParallelStreams,
                version,
                batchEnabled,
                secondary,
                partition);
            secondary->Open();
            secondary->ChangeRole(epoch_, ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
            // Start operation pumps
            secondary->StartCopyOperationPump(secondary);
            if (supportsParallelStreams)
            {
                secondary->StartReplicationOperationPump(secondary);
            }
        }

        void AddActiveReplicas(size_t numberOfActives)
        {
            size_t startIndex = secondaries_.size();
            AddIdleReplicas(numberOfActives);
            PromoteIdles(startIndex, startIndex + numberOfActives);
        }

        void AddIdleReplicas(size_t numberOfIdles)
        {
            vector<ReplicaInformation> replicas;
            size_t startIndex = secondaries_.size();
            
            for (size_t i = startIndex; i < startIndex + numberOfIdles; ++i)
            {
                ::FABRIC_REPLICA_ID secondaryId = primaryId_ + i + 1;
                ComProxyTestReplicatorSPtr secondary;
                CreateIdle(secondaryId, secondary);
                
                ReplicaInformation replica(
                    secondaryId,
                    ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
                    secondary->ReplicationEndpoint,
                    false);
                replicas.push_back(move(replica));

                secondaries_.push_back(move(secondary));
                secondaryIds_.push_back(move(secondaryId));
            }

            primary_->BuildIdles(replicas);
        }
    
        void PromoteIdles(
            size_t startIndex,
            size_t endIndex)
        {
            ASSERT_IF(
                startIndex >= secondaries_.size() || endIndex > secondaries_.size(),
                "Incorrect indexes {0}/{1}: {2} replicas",
                startIndex,
                endIndex, 
                secondaries_.size());
            vector<ReplicaInformation> replicas;
        
            // Transform idles to secondaries for as many secondaries are required
            for(size_t i = startIndex; i < endIndex; ++i)
            {
                ReplicaInformation replica(
                    secondaryIds_[i],
                    ::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY,
                    secondaries_[i]->ReplicationEndpoint,
                    false);
            
                replicas.push_back(std::move(replica));

                secondaries_[i]->ChangeRole(epoch_, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
            }
        
            primary_->UpdateCatchUpReplicaSetConfiguration(replicas);
            primary_->UpdateCurrentReplicaSetConfiguration(replicas);
        }
        
        void UpdateEpoch(LONGLONG newConfigVersion, LONGLONG dataLossVersion)
        {
            epoch_.ConfigurationNumber = newConfigVersion;
            epoch_.DataLossNumber = dataLossVersion;
        }

        void UpdateEpoch(LONGLONG newConfigVersion, LONGLONG dataLossVersion, FABRIC_SEQUENCE_NUMBER newProgress)
        {
            epoch_.ConfigurationNumber = newConfigVersion;
            epoch_.DataLossNumber = dataLossVersion;
            if (changeStateOnDataLoss_)
            {
                // Update the state provider epoch as well
                ManualResetEvent updateEpochEvent;
                ErrorCode error;

                ComPointer<IFabricStateProvider> stateProviderCopy = stateProvider_;
                ComProxyStateProvider stateProviderProxy(std::move(stateProviderCopy));
                AsyncOperationSPtr inner = stateProviderProxy.BeginUpdateEpoch(
                    epoch_,
                    newProgress,
                    [&error, &stateProviderProxy, &updateEpochEvent](AsyncOperationSPtr const & asyncOperation)
                    {
                        if (!asyncOperation->CompletedSynchronously)
                        {
                            error = stateProviderProxy.EndUpdateEpoch(asyncOperation);
                            updateEpochEvent.Set();
                        }
                    },
                    NULL);
                if (inner->CompletedSynchronously)
                {
                    error = stateProviderProxy.EndUpdateEpoch(inner);
                    updateEpochEvent.Set();
                }
                updateEpochEvent.WaitOne();
                VERIFY_SUCCEEDED(error.IsSuccess(), L"StateProvider.UpdateEpoch succeeds");
            }
        }

        void Close()
        {
            primary_->Close();
            for(size_t i = 0; i < secondaries_.size(); ++i)
            {
                secondaries_[i]->Close();
            }
        }

        static void CreateReplicator(
            __in ComReplicatorFactory & factory, 
            Guid partitionId, 
            bool hasPersistedState,
            __in_opt FABRIC_REPLICATOR_SETTINGS *settings,
            __out Common::ComPointer<IFabricStateReplicator> & replicator,
            __out ComTestStatefulServicePartition ** partition)
        {
            return CreateReplicator(factory, partitionId, hasPersistedState, settings, S_OK, replicator, partition);
        }

        static void CreateReplicator(
            __in ComReplicatorFactory & factory, 
            Guid partitionId, 
            bool hasPersistedState,
            __in_opt FABRIC_REPLICATOR_SETTINGS *settings,
            HRESULT expectedHr,
            __out Common::ComPointer<IFabricStateReplicator> & replicator,
            __out ComTestStatefulServicePartition ** partition)
        {
            auto partition2 = new ComTestStatefulServicePartition(partitionId);
            partition = &partition2;

            IReplicatorHealthClientSPtr temp;
            auto hr = factory.CreateReplicator(
                0LL,
                partition2,
                new ComTestStateProvider(0, -1, TestPublicApi::GetRoot()),
                settings,
                hasPersistedState,
                move(temp),
                replicator.InitializationAddress());

            VERIFY_ARE_EQUAL(expectedHr, hr, L"Create replicator");
        
            if (FAILED(hr))
            {
                return;
            }

            IFabricReplicator *replicatorObj;
            VERIFY_SUCCEEDED(
                replicator->QueryInterface(
                    IID_IFabricReplicator, 
                    reinterpret_cast<void**>(&replicatorObj)),
                L"Query for IFabricReplicator");

            IFabricStateReplicator *replicatorStateObj;
            VERIFY_SUCCEEDED(
                replicator->QueryInterface(
                    IID_IFabricStateReplicator, 
                    reinterpret_cast<void**>(&replicatorStateObj)),
                L"Query for IFabricStateReplicator");
        }

        static void CheckStrings(__in vector<wstring> & s1, __in vector<wstring> & s2)
        {
            sort(s1.begin(), s1.end());
            sort(s2.begin(), s2.end());

            wstring s1String;
            StringWriter writer(s1String);
            for (wstring const & s : s1)
            {
                writer << s <<L";";
            }

            wstring s2String;
            StringWriter writer2(s2String);
            for (wstring const & s : s2)
            {
                writer2 << s <<L";";
            }

            VERIFY_ARE_EQUAL_FMT(s1, s2, "CheckStrings: expected \"{0}\", actual \"{1}\"", s1String, s2String);
        }
                
    private:

        Common::Guid partitionId_;
        wstring failoverUnitId_;
        ComReplicatorFactory factory_;
        ::FABRIC_REPLICA_ID primaryId_;
        vector<::FABRIC_REPLICA_ID> secondaryIds_;
        ::FABRIC_EPOCH epoch_;
        bool hasPersistedState_;
        int64 numberOfCopyOps_;
        int64 numberOfCopyContextOps_;
        bool changeStateOnDataLoss_;
        ComProxyTestReplicatorSPtr primary_;
        vector<ComProxyTestReplicatorSPtr> secondaries_;
        ComPointer<IFabricStateProvider> stateProvider_;
    };

    // Faulty state provider that returns an error 
    // when generating an operation
    class ComTestFaultyStateProvider : public ComTestStateProvider
    {
    public:
        ComTestFaultyStateProvider(
            int64 numberOfCopyOps,
            int64 numberOfCopyContextOps,
            bool failGetCopyContext,
            Common::ComponentRoot const & root) 
            :   ComTestStateProvider(numberOfCopyOps, numberOfCopyContextOps, root, false),
                failGetCopyContext_(failGetCopyContext)
        {
        }

        HRESULT STDMETHODCALLTYPE GetCopyContext(
            /*[out, retval]*/ IFabricOperationDataStream **enumerator)
        {
            if (failGetCopyContext_)
            {
                return E_ABORT;
            }
            return ComTestStateProvider::GetCopyContext(enumerator);
        }

    protected: 
        virtual HRESULT CreateOperation(bool, __out Common::ComPointer<IFabricOperationData> &) const
        {
            return E_ABORT;
        }

    private:
        bool failGetCopyContext_;
    };
    } // anonymous namespace

    /***********************************
    * TestPublicApi methods
    **********************************/
    Common::ComponentRoot const & TestPublicApi::GetRoot()
    {
        static std::shared_ptr<DummyRoot> root = std::make_shared<DummyRoot>();
        return *root;
    }

    void VerifyOperationData(
        ComPointer<IFabricOperation> const & op,
        wchar_t ** expectedBufferContens,
        ULONG expectedBufferCount,
        FABRIC_SEQUENCE_NUMBER expectedSequenceNumber = 0)
    {
        FABRIC_OPERATION_METADATA const * metadata = op->get_Metadata();
        if (expectedSequenceNumber > 0)
        {
            VERIFY_ARE_EQUAL_FMT(metadata->SequenceNumber, expectedSequenceNumber, "sequenceNumber expected {0}, actual {1}", expectedSequenceNumber, metadata->SequenceNumber);
        }
        ULONG bufferCount;
        FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;
        HRESULT hr = op->GetData(&bufferCount, &replicaBuffers);
        VERIFY_SUCCEEDED(hr, L"GetData() failed.");
        VERIFY_ARE_EQUAL_FMT(bufferCount, expectedBufferCount, "bufferCount expected {0}, actual {1}", expectedBufferCount, bufferCount);
        VERIFY_IS_TRUE(replicaBuffers != NULL, L"replicaBuffers");
        for (ULONG idx = 0; idx < bufferCount; idx++)
        {
            VERIFY_IS_TRUE_FMT(replicaBuffers[idx].Buffer != NULL, "replicaBuffers[{0}].Buffer is NULL", idx);
            VERIFY_ARE_EQUAL_FMT(replicaBuffers[idx].BufferSize, wcslen(expectedBufferContens[idx])*sizeof(wchar_t), "replicaBuffers[{0}].BufferSize expepected {1}, actual {2}", idx, std::wstring(expectedBufferContens[idx]), replicaBuffers[idx].BufferSize);
            VERIFY_IS_TRUE_FMT(std::wstring((wchar_t*)replicaBuffers[idx].Buffer, replicaBuffers[idx].BufferSize/sizeof(wchar_t)).compare(std::wstring(expectedBufferContens[idx])) == 0, 
                "Buffer content expected [{0}], actual [{1}]",
                std::wstring((wchar_t*)replicaBuffers[idx].Buffer, replicaBuffers[idx].BufferSize/sizeof(wchar_t)),
                std::wstring(expectedBufferContens[idx]));
        }
    }
    BOOST_FIXTURE_TEST_SUITE(TestPublicApiSuite,TestPublicApi)

    BOOST_AUTO_TEST_CASE(TestStateReplicator1)
    {
        TestStateReplicator(true);
    }

    BOOST_AUTO_TEST_CASE(TestStateReplicator2)
    {
        TestStateReplicator(false);
    }

    BOOST_AUTO_TEST_CASE(TestDataLoss1)
    {
        TestDataLoss(true);
    }

    BOOST_AUTO_TEST_CASE(TestDataLoss2)
    {
        TestDataLoss(false);
    }

    ///
    /// Regression test for 3745027
    /// Owner: MCoskun
    ///
    BOOST_AUTO_TEST_CASE(BeginReplicateReturnsSOKIfAsyncOperationFails)
    {
        ReplicatorSPtr secondary0;
        ComPointer<ComTestStateProvider> s0StateProvider;
        FABRIC_REPLICA_ID s0Id = 2;

        FABRIC_EPOCH epoch;
        epoch.DataLossNumber = 1;
        epoch.ConfigurationNumber = 122;
        epoch.Reserved = NULL;

        int64 numberOfCopyOps = 2;
        int64 numberOfCopyContextOps = 1;

        auto transportSecondary0 = TestChangeRole::CreateAndOpen(
            s0Id, 
            numberOfCopyOps, numberOfCopyContextOps, epoch, FABRIC_REPLICA_ROLE_IDLE_SECONDARY, true, secondary0, s0StateProvider);
        Replicator & s0 = *(secondary0.get());

        TestChangeRole::ChangeRole(s0, epoch, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);

        auto comReplicator = make_com<ComReplicator>(s0);

        auto operation = make_com<ComTestOperation, IFabricOperationData>(L"ReplicatorChangeRole - test replicate operation");
        ComPointer<IFabricAsyncOperationContext> context;

        ManualResetEvent replicateDoneEvent;
        auto callback = make_com<TestFabricAsyncCallbackHelper>(
            [&comReplicator, &replicateDoneEvent](IFabricAsyncOperationContext *callbackContext)
        {
            FABRIC_SEQUENCE_NUMBER lsn;
            auto errorCode = comReplicator->EndReplicate(callbackContext, &lsn);

            VERIFY_ARE_EQUAL(FABRIC_E_NOT_PRIMARY, errorCode);

            replicateDoneEvent.Set();
        });

        FABRIC_SEQUENCE_NUMBER sequenceNumber;

        auto response = comReplicator->BeginReplicate(
            operation.GetRawPointer(),
            callback.GetRawPointer(),
            &sequenceNumber,
            context.InitializationAddress());

        // Verification
        // The BeginReplicate is expected to complete with S_OK even though the operation will fail with FABRIC_E_NOT_PRIMARY.
        VERIFY_ARE_EQUAL(S_OK, response);

        VERIFY_IS_TRUE(replicateDoneEvent.IsSet());

        // Clean up
        TestChangeRole::Close(s0);
        transportSecondary0->Stop();
    }

    // RDBug 4885269: V1 repl should not throw not primary exception when it is secondary
    BOOST_AUTO_TEST_CASE(TestBeginReplicateReturnsPartitionAccessWriteError)
    {
        bool hasPersistedState = true;

        ComTestOperation::WriteInfo(
            PublicTestSource,
            "Start TestBeginReplicateReturnsPartitionAccessWriteError, persisted state {0}",
            hasPersistedState);

        ReplicatorTestWrapper wrapper(
            hasPersistedState, 
            0, 
            0,
            false);
        
        ComTestStatefulServicePartition * partition = nullptr;
        wrapper.CreatePrimary(&partition);

        ManualResetEvent replicateDoneEvent;

        // RECONFIG Pending Error Code
        {
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING;
            partition->SetWriteStatus(writeStatus);
            replicateDoneEvent.Reset();
            auto replicateOp = wrapper.GetPrimary()->BeginReplicate(
                make_com<ComTestOperation, IFabricOperationData>(L"aBc"),
                [&wrapper, &replicateDoneEvent, &writeStatus](AsyncOperationSPtr const& operation) -> void
                {
                    ErrorCode error = wrapper.GetPrimary()->EndReplicate(-1, operation);
                    if (error.IsSuccess())
                    {
                        VERIFY_FAIL_FMT("Replicate succeeded instead of failing with error {0}.", writeStatus);
                    }

                    VERIFY_ARE_EQUAL(error.ReadValue(), Common::ErrorCodeValue::ReconfigurationPending);
    
                    replicateDoneEvent.Set();
                }, AsyncOperationSPtr());
            replicateDoneEvent.WaitOne();
        }

        // No Write Quorum Error Code
        {
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NO_WRITE_QUORUM;
            partition->SetWriteStatus(writeStatus);
            replicateDoneEvent.Reset();
            auto replicateOp = wrapper.GetPrimary()->BeginReplicate(
                make_com<ComTestOperation, IFabricOperationData>(L"aBc"),
                [&wrapper, &replicateDoneEvent, &writeStatus](AsyncOperationSPtr const& operation) -> void
                {
                    ErrorCode error = wrapper.GetPrimary()->EndReplicate(-1, operation);
                    if (error.IsSuccess())
                    {
                        VERIFY_FAIL_FMT("Replicate succeeded instead of failing with error {0}.", writeStatus);
                    }

                    VERIFY_ARE_EQUAL(error.ReadValue(), Common::ErrorCodeValue::NoWriteQuorum);
    
                    replicateDoneEvent.Set();
                }, AsyncOperationSPtr());
            replicateDoneEvent.WaitOne();
        }
 
        // Not Primary Error Code
        {
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY;
            partition->SetWriteStatus(writeStatus);
            replicateDoneEvent.Reset();
            auto replicateOp = wrapper.GetPrimary()->BeginReplicate(
                make_com<ComTestOperation, IFabricOperationData>(L"aBc"),
                [&wrapper, &replicateDoneEvent, &writeStatus](AsyncOperationSPtr const& operation) -> void
                {
                    ErrorCode error = wrapper.GetPrimary()->EndReplicate(-1, operation);
                    if (error.IsSuccess())
                    {
                        VERIFY_FAIL_FMT("Replicate succeeded instead of failing with error {0}.", writeStatus);
                    }

                    VERIFY_ARE_EQUAL(error.ReadValue(), Common::ErrorCodeValue::NotPrimary);
    
                    replicateDoneEvent.Set();
                }, AsyncOperationSPtr());
            replicateDoneEvent.WaitOne();
        }

        wrapper.Close();
    }

    BOOST_AUTO_TEST_CASE(TestIOperationData)
    {
        bool hasPersistedState = true;

        ComTestOperation::WriteInfo(
            PublicTestSource,
            "Start TestIOperationData, persisted state {0}",
            hasPersistedState);

        ReplicatorTestWrapper wrapper(
            hasPersistedState, 
            0, 
            0,
            false);
        
        ComTestStatefulServicePartition * partition = nullptr;
        wrapper.CreatePrimary(&partition);

        // Create an idle replica, but do not save it inside the wrapper
        ComProxyTestReplicatorSPtr secondary;
        ::FABRIC_REPLICA_ID secondaryId = 44446;
        wrapper.CreateIdle(secondaryId, secondary);
        
        vector<ReplicaInformation> replicas;
        replicas.push_back(ReplicaInformation(
            secondaryId,
            ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
            secondary->ReplicationEndpoint,
            false));
        wrapper.GetPrimary()->BuildIdles(replicas);
        
        // Promote the idle to secondary
        vector<ReplicaInformation> activeReplicas;
        activeReplicas.push_back(
            ReplicaInformation(
            secondaryId,
            ::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY,
            secondary->ReplicationEndpoint,
            false));
        wrapper.GetPrimary()->UpdateCatchUpReplicaSetConfiguration(activeReplicas);
        wrapper.GetPrimary()->UpdateCurrentReplicaSetConfiguration(activeReplicas);

        ManualResetEvent replicateDoneEvent;
        ManualResetEvent pumpDoneEvent;
        FABRIC_SEQUENCE_NUMBER expectedSequenceNumber = 1;

        // Normal operation data with 1 buffer
        {
            replicateDoneEvent.Reset();
            pumpDoneEvent.Reset();
            secondary->SetReplicationOperationProcessor(
                [&pumpDoneEvent](ComPointer<IFabricOperation> const & op) -> void
                {
                    if (op)
                    {
                        wchar_t * expectedContent[] = { L"aBc" };
                        VerifyOperationData(op, expectedContent, 1, 1);
                        pumpDoneEvent.Set();
                    }
                });

            auto replicateOp = wrapper.GetPrimary()->BeginReplicate(
                make_com<ComTestOperation, IFabricOperationData>(L"aBc"),
                [&wrapper, &replicateDoneEvent, &expectedSequenceNumber](AsyncOperationSPtr const& operation) -> void
                {
                    ErrorCode error = wrapper.GetPrimary()->EndReplicate(expectedSequenceNumber++, operation);
                    if (!error.IsSuccess())
                    {
                        VERIFY_FAIL_FMT("Replicate failed with error {0}.", error);
                    }
    
                    replicateDoneEvent.Set();
                }, AsyncOperationSPtr());
            replicateDoneEvent.WaitOne();
            pumpDoneEvent.WaitOne();
        }

        // Normal operation data with 2 buffers
        {
            replicateDoneEvent.Reset();
            pumpDoneEvent.Reset();
            secondary->SetReplicationOperationProcessor(
                [&pumpDoneEvent](ComPointer<IFabricOperation> const & op) -> void
                {
                    if (op)
                    {
                        wchar_t * expectedContent[] = { L"aBc", L"0!23" };
                        VerifyOperationData(op, expectedContent, 2);
                        pumpDoneEvent.Set();
                    }
                });
            auto replicateOp = wrapper.GetPrimary()->BeginReplicate(
                make_com<ComTestOperation, IFabricOperationData>(L"aBc", L"0!23"),
                [&wrapper, &replicateDoneEvent, &expectedSequenceNumber](AsyncOperationSPtr const& operation) -> void
                {
                    ErrorCode error = wrapper.GetPrimary()->EndReplicate(expectedSequenceNumber++, operation);
                    if (!error.IsSuccess())
                    {
                        VERIFY_FAIL_FMT("Replicate failed with error {0}.", error);
                    }
    
                    replicateDoneEvent.Set();
                }, AsyncOperationSPtr());
            replicateDoneEvent.WaitOne();
            pumpDoneEvent.WaitOne();
        }

        // Normal operation data with 2 buffers, 1 non-empty, 1 empty
        {
            replicateDoneEvent.Reset();
            pumpDoneEvent.Reset();
            secondary->SetReplicationOperationProcessor(
                [&pumpDoneEvent](ComPointer<IFabricOperation> const & op) -> void
                {
                    if (op)
                    {
                        wchar_t * expectedContent[] = { L"aBc", L"" };
                        VerifyOperationData(op, expectedContent, 2);
                        pumpDoneEvent.Set();
                    }
                });
            auto replicateOp = wrapper.GetPrimary()->BeginReplicate(
                make_com<ComTestOperation, IFabricOperationData>(L"aBc", L""),
                [&wrapper, &replicateDoneEvent, &expectedSequenceNumber](AsyncOperationSPtr const& operation) -> void
                {
                    ErrorCode error = wrapper.GetPrimary()->EndReplicate(expectedSequenceNumber++, operation);
                    if (!error.IsSuccess())
                    {
                        VERIFY_FAIL_FMT("Replicate failed with error {0}.", error);
                    }
    
                    replicateDoneEvent.Set();
                }, AsyncOperationSPtr());
            replicateDoneEvent.WaitOne();
            pumpDoneEvent.WaitOne();
        }

        // Normal operation data with 2 buffers, 1 empty, 1 non-empty
        {
            replicateDoneEvent.Reset();
            pumpDoneEvent.Reset();
            secondary->SetReplicationOperationProcessor(
                [&pumpDoneEvent](ComPointer<IFabricOperation> const & op) -> void
                {
                    if (op)
                    {
                        wchar_t * expectedContent[] = { L"", L"0!23" };
                        VerifyOperationData(op, expectedContent, 2);
                        pumpDoneEvent.Set();
                    }
                });
            auto replicateOp = wrapper.GetPrimary()->BeginReplicate(
                make_com<ComTestOperation, IFabricOperationData>(L"", L"0!23"),
                [&wrapper, &replicateDoneEvent, &expectedSequenceNumber](AsyncOperationSPtr const& operation) -> void
                {
                    ErrorCode error = wrapper.GetPrimary()->EndReplicate(expectedSequenceNumber++, operation);
                    if (!error.IsSuccess())
                    {
                        VERIFY_FAIL_FMT("Replicate failed with error {0}.", error);
                    }
    
                    replicateDoneEvent.Set();
                }, AsyncOperationSPtr());
            replicateDoneEvent.WaitOne();
            pumpDoneEvent.WaitOne();
        }

        // Normal operation data with 2 empty buffers
        {
            replicateDoneEvent.Reset();
            pumpDoneEvent.Reset();
            secondary->SetReplicationOperationProcessor(
                [&pumpDoneEvent](ComPointer<IFabricOperation> const & op) -> void
                {
                    if (op)
                    {
                        wchar_t * expectedContent[] = { L"", L"" };
                        VerifyOperationData(op, expectedContent, 2);
                        pumpDoneEvent.Set();
                    }
                });
            auto replicateOp = wrapper.GetPrimary()->BeginReplicate(
                make_com<ComTestOperation, IFabricOperationData>(L"", L""),
                [&wrapper, &replicateDoneEvent, &expectedSequenceNumber](AsyncOperationSPtr const& operation) -> void
                {
                    ErrorCode error = wrapper.GetPrimary()->EndReplicate(expectedSequenceNumber++, operation);
                    if (!error.IsSuccess())
                    {
                        VERIFY_FAIL_FMT("Replicate failed with error {0}.", error);
                    }
    
                    replicateDoneEvent.Set();
                }, AsyncOperationSPtr());
            replicateDoneEvent.WaitOne();
            pumpDoneEvent.WaitOne();
        }

        // Empty operation data with 1 buffer of 0 size
        {
            replicateDoneEvent.Reset();
            pumpDoneEvent.Reset();
            secondary->SetReplicationOperationProcessor(
                [&pumpDoneEvent](ComPointer<IFabricOperation> const & op) -> void
                {
                    if (op)
                    {
                        wchar_t * expectedContent[] = { L"" };
                        VerifyOperationData(op, expectedContent, 1);
                        pumpDoneEvent.Set();
                    }
                });
            auto replicateOp = wrapper.GetPrimary()->BeginReplicate(
                make_com<ComTestOperation, IFabricOperationData>(L""),
                [&wrapper, &replicateDoneEvent, &expectedSequenceNumber](AsyncOperationSPtr const& operation) -> void
                {
                    ErrorCode error = wrapper.GetPrimary()->EndReplicate(expectedSequenceNumber++, operation);
                    if (!error.IsSuccess())
                    {
                        VERIFY_FAIL_FMT("Replicate failed with error {0}.", error);
                    }
    
                    replicateDoneEvent.Set();
                }, AsyncOperationSPtr());
            replicateDoneEvent.WaitOne();
            pumpDoneEvent.WaitOne();
        }

        // Empty operation data with 0 buffer and NULL FABRIC_OPERATION_DATA_BUFFER
        {
            replicateDoneEvent.Reset();
            pumpDoneEvent.Reset();
            secondary->SetReplicationOperationProcessor(
                [&pumpDoneEvent](ComPointer<IFabricOperation> const & op) -> void
                {
                    if (op)
                    {
                        ULONG bufferCount;
                        FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;
                        HRESULT hr = op->GetData(&bufferCount, &replicaBuffers);
                        VERIFY_SUCCEEDED(hr, L"GetData() failed.");
                        VERIFY_ARE_EQUAL_FMT(bufferCount, 0ul, "bufferCount expected 0, actual {0}", bufferCount);
                        VERIFY_IS_TRUE(replicaBuffers == NULL, L"replicaBuffers");
                        pumpDoneEvent.Set();
                    }
                });
            auto replicateOp = wrapper.GetPrimary()->BeginReplicate(
                make_com<ComTestOperation, IFabricOperationData>(),
                [&wrapper, &replicateDoneEvent, &expectedSequenceNumber](AsyncOperationSPtr const& operation) -> void
                {
                    ErrorCode error = wrapper.GetPrimary()->EndReplicate(expectedSequenceNumber++, operation);
                    if (!error.IsSuccess())
                    {
                        VERIFY_FAIL_FMT("Replicate failed with error {0}.", error);
                    }
    
                    replicateDoneEvent.Set();
                }, AsyncOperationSPtr());
            replicateDoneEvent.WaitOne();
            pumpDoneEvent.WaitOne();
        }
        
        // Very large operation
        {
            auto replicateOp = wrapper.GetPrimary()->BeginReplicate(
                make_com<ComTestOperation, IFabricOperationData>(4, 1024*1024*50 + 1),
                [&wrapper, &replicateDoneEvent, &expectedSequenceNumber](AsyncOperationSPtr const& operation) -> void
                {
                    ErrorCode error = wrapper.GetPrimary()->EndReplicate(expectedSequenceNumber, operation);

                    VERIFY_ARE_EQUAL(true, operation->CompletedSynchronously);
                    VERIFY_ARE_EQUAL(ErrorCodeValue::REOperationTooLarge, error.ReadValue());
    
                    replicateDoneEvent.Set();
                }, AsyncOperationSPtr());
            replicateDoneEvent.WaitOne();
            pumpDoneEvent.WaitOne();
        }

        // Negative test cases
        for (int invalidType = 1; invalidType <= 3; invalidType++)
        {
            replicateDoneEvent.Reset();
            secondary->SetReplicationOperationProcessor(
                [&pumpDoneEvent, invalidType](ComPointer<IFabricOperation> const & op) -> void
                {
                    if (op)
                    {
                        VERIFY_FAIL_FMT("No operations should be pumped for invalidType {0}", invalidType);
                    }
                });
            auto replicateOp = wrapper.GetPrimary()->BeginReplicate(
                make_com<ComTestOperation, IFabricOperationData>(invalidType),
                [&wrapper, &replicateDoneEvent, &expectedSequenceNumber, invalidType](AsyncOperationSPtr const& operation) -> void
                {
                    ErrorCode error = wrapper.GetPrimary()->EndReplicate(expectedSequenceNumber, operation);
                    if (E_INVALIDARG != error.ToHResult())
                    {
                        if (error.IsSuccess())
                        {
                            expectedSequenceNumber++;
                        }
                        VERIFY_FAIL_FMT("BeginReplicate expects error E_INVALIDARG for invalidType {0}", invalidType);
                    }
                    replicateDoneEvent.Set();
                }, AsyncOperationSPtr());
            replicateDoneEvent.WaitOne();
        }

        pumpDoneEvent.Reset();
        secondary->SetReplicationOperationProcessor(
            [&pumpDoneEvent](ComPointer<IFabricOperation> const & op) -> void
            {
                VERIFY_IS_TRUE(!op, L"Expected null operation be pumped");
                pumpDoneEvent.Set();
            });
        wrapper.Close();
        secondary->Close();
        pumpDoneEvent.WaitOne();
    }

    BOOST_AUTO_TEST_CASE(TestCreateReplicatorWithDefaultAddress)
    {
        bool hasPersistedState = false;
        ComTestOperation::WriteInfo(
            PublicTestSource,
            "Start TestCreateReplicatorWithDefaultAddress, persisted state {0}",
            hasPersistedState);

        Common::Guid partitionId = Common::Guid::NewGuid();
        ComReplicatorFactory factory(TestPublicApi::GetRoot());
        VERIFY_IS_TRUE(factory.Open(L"0").IsSuccess(), L"ComReplicatorFactory opened");

        std::vector<wstring> actualAddresses;
        
        Common::ComPointer<IFabricStateReplicator> replicator3;
        ComTestStatefulServicePartition * partition = nullptr;
        ReplicatorTestWrapper::CreateReplicator(factory, partitionId, hasPersistedState, NULL, replicator3, &partition);
        factory.Test_GetFactoryDetails(actualAddresses);
        VERIFY_ARE_EQUAL(1, actualAddresses.size(), L"Number of addresses should be 1");
        VERIFY_ARE_EQUAL(actualAddresses[0], L"localhost:0", L"default address stored is not localhost:0");
        
        VERIFY_IS_TRUE(factory.Close().IsSuccess(), L"ComReplicatorFactory closed");
    }
// LINUXTODO
#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(TestCreateReplicator)
    {
        bool hasPersistedState = false;
        ComTestOperation::WriteInfo(
            PublicTestSource,
            "Start TestCreateReplicator, persisted state {0}",
            hasPersistedState);

        Common::Guid partitionId = Common::Guid::NewGuid();
        ComReplicatorFactory factory(TestPublicApi::GetRoot());
        VERIFY_IS_TRUE(factory.Open(L"0").IsSuccess(), L"ComReplicatorFactory opened");

        // Create replicators at multiple ports and check the factory

        USHORT basePort = 0;
        TestPortHelper::GetPorts(10, basePort);
        wstring addr1 = wformatString("127.0.0.1:{0}", basePort++);
        wstring addr2 = wformatString("127.0.0.1:{0}", basePort++);

        vector<wstring> addresses;
        addresses.push_back(addr1);
        addresses.push_back(addr2);

        FABRIC_REPLICATOR_SETTINGS settings1;
        settings1.ReplicatorAddress = addresses[0].c_str();
        settings1.Flags = FABRIC_REPLICATOR_ADDRESS;
        settings1.Reserved = NULL;
        Common::ComPointer<IFabricStateReplicator> replicator1;
        ComTestStatefulServicePartition * partition1 = nullptr;
        ReplicatorTestWrapper::CreateReplicator(factory, partitionId, hasPersistedState, &settings1, replicator1, &partition1);
        
        // Create replicators at multiple ports and check the factory
        FABRIC_REPLICATOR_SETTINGS settings2;
        settings2.ReplicatorAddress = addresses[1].c_str();
        settings2.Flags = FABRIC_REPLICATOR_ADDRESS;
        settings2.Reserved = NULL;
        Common::ComPointer<IFabricStateReplicator> replicator2;
        ComTestStatefulServicePartition * partition2 = nullptr;
        ReplicatorTestWrapper::CreateReplicator(factory, partitionId, hasPersistedState, &settings2, replicator2, &partition2);
        
        vector<wstring> actualAddresses;
        
        factory.Test_GetFactoryDetails(actualAddresses);
        ReplicatorTestWrapper::CheckStrings(addresses, actualAddresses);
        
        // Try to create a replicator with a different security setting - should fail
        FABRIC_REPLICATOR_SETTINGS settings3;
        settings3.ReplicatorAddress = addresses[0].c_str();
        settings3.Flags = FABRIC_REPLICATOR_ADDRESS | FABRIC_REPLICATOR_SECURITY;
        settings3.Reserved = NULL;

        Transport::SecuritySettings securitySettings;
        auto error = Transport::SecuritySettings::FromConfiguration(
            L"X509",
            X509Default::StoreName(),
            wformatString(X509Default::StoreLocation()),
            wformatString(X509FindType::FindBySubjectName),
            L"CN=WinFabric-Test-SAN1-Alice",
            L"",
            wformatString(Transport::ProtectionLevel::EncryptAndSign),
            L"",
            SecurityConfig::X509NameMap(),
            SecurityConfig::IssuerStoreKeyValueMap(),
            L"Replication.Test.ComFabricReplicator.com",
            L"",
            L"",
            L"",
            securitySettings);

        VERIFY_IS_TRUE(error.IsSuccess());
        FABRIC_SECURITY_CREDENTIALS securityCredentials;
        Common::ScopedHeap heap;
        securitySettings.ToPublicApi(heap, securityCredentials);

        settings3.SecurityCredentials = &securityCredentials;

        Common::ComPointer<IFabricStateReplicator> invalid;
        ComTestStatefulServicePartition * invalidP = nullptr;
        ReplicatorTestWrapper::CreateReplicator(factory, partitionId, hasPersistedState, &settings3, E_INVALIDARG, invalid, &invalidP);
        
        VERIFY_IS_TRUE(factory.Close().IsSuccess(), L"ComReplicatorFactory closed");
    }
#endif
    BOOST_AUTO_TEST_CASE(TestCancelBuildIdle)
    {
        bool hasPersistedState = true;
        int64 numberOfCopyOps = 13891;
        int64 numberOfCopyContextOps = hasPersistedState ? 0 : -1;
        
        ComTestOperation::WriteInfo(
            PublicTestSource,
            "Start TestCancelBuildIdle, persisted state {0}",
            hasPersistedState);

        ReplicatorTestWrapper wrapper(
            hasPersistedState, 
            numberOfCopyOps, 
            numberOfCopyContextOps,
            false);

        ComTestStatefulServicePartition * partition = nullptr;
        wrapper.CreatePrimary(&partition);
        ComProxyTestReplicatorSPtr secondary;
        ::FABRIC_REPLICA_ID secondaryId = 12341;
        wrapper.CreateIdle(secondaryId, secondary);

        ReplicaInformation replica(
            secondaryId,
            ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
            secondary->ReplicationEndpoint,
            false);

        // Try to build a replica 
        // and cancel the build process
        wrapper.GetPrimary()->BuildIdle(replica, true /*cancel*/, false /*successExpected*/, Common::TimeSpan::FromMilliseconds(40));

        wrapper.Close();
        secondary->Close();
    }

    BOOST_AUTO_TEST_CASE(TestBuildUnreachable)
    {
        bool hasPersistedState = true;
        int64 numberOfCopyOps = 1;
        int64 numberOfCopyContextOps = hasPersistedState ? 0 : -1;

        ComTestOperation::WriteInfo(
            PublicTestSource,
            "Start TestBuildUnreachable, persisted state {0}",
            hasPersistedState);

        ReplicatorTestWrapper wrapper(
            hasPersistedState,
            numberOfCopyOps,
            numberOfCopyContextOps,
            false);

        ComTestStatefulServicePartition * partition = nullptr;
        wrapper.CreatePrimary(&partition);

        ReplicaInformation replica(
            12341,
            ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
            L"10.11.12.13:50713/c646fd04-4f04-4005-8e07-1e3e29b5d11e-12341;8b54171e-5c77-4e8d-a324-2d9469c19eb5;0",
            false);

        auto healthCheckTimeout = Common::TimeSpan::FromSeconds(90); //60 seconds maximum transport timeout + 5 seconds replicator health report cache timeout + 25 seconds margin
        auto cancelTimeout = Common::TimeSpan::FromSeconds(90);
        // Try to build a replica
        auto primary = wrapper.GetPrimary();
        ManualResetEvent copyDone;

        Common::Threadpool::Post([&]
            {
                primary->BuildIdle(replica, true /*cancel*/, false /*successExpected*/, cancelTimeout);
                copyDone.Set();
            },
            Common::TimeSpan::FromSeconds(0));

        ComReplicator * comReplicator = reinterpret_cast<ComReplicator*>(primary->ComReplicator.GetRawPointer());
        HealthClientSPtr healthClient = dynamic_pointer_cast<HealthClient>(comReplicator->ReplicatorEngine.Test_HealthClient);
        ASSERT_IF(healthClient == nullptr, "health client cannot be null.");

        auto startTicks = Common::DateTime::Now();
        TestHealthClientReportEntry expectedWarningReport(
            SystemHealthReportCode::RE_RemoteReplicatorConnectionStatusFailed,
            L"",
            L"",
            -1,
            Common::TimeSpan::FromSeconds(0)
        );
        TestHealthClientReportEntry expectedOkReport(
            SystemHealthReportCode::RE_RemoteReplicatorConnectionStatusOk,
            L"",
            L"",
            -1,
            Common::TimeSpan::FromSeconds(0)
        );

        ManualResetEvent verificationDone;
        auto checkTimer = Common::Timer::Create("healthWarningCheckTimer", [&](Common::TimerSPtr const timer)
        {
            auto nowTicks = Common::DateTime::Now();
            if ((nowTicks - startTicks) < healthCheckTimeout) {

                if (healthClient->IsReported(expectedWarningReport))
                {
                    verificationDone.Set();
                }
                else
                {
                    timer->Change(Common::TimeSpan::FromSeconds(1));
                }
            }
        });
        checkTimer->Change(Common::TimeSpan::FromSeconds(0), Common::TimeSpan::FromSeconds(0));
        VERIFY_IS_TRUE(verificationDone.WaitOne(healthCheckTimeout), L"Did not observe warning on a stuck build.");

        VERIFY_IS_TRUE(copyDone.WaitOne(cancelTimeout), L"Copy async operation did not complete.");
        
        Sleep(2000);
        VERIFY_IS_TRUE(healthClient->IsReported(expectedOkReport), L"Expected OK health report after copy close is not observed.");

        checkTimer->Cancel();
        wrapper.Close();
    }

    BOOST_AUTO_TEST_CASE(TestBuildIdleWithSecondaryCopyContextErrors)
    {
        bool hasPersistedState = true;
        int64 numberOfCopyOps = 13891;
        int64 numberOfCopyContextOps = hasPersistedState ? 120 : -1;
        
        ComTestOperation::WriteInfo(
            PublicTestSource,
            "Start TestBuildIdleWithSecondaryCopyContextErrors, persisted state {0}",
            hasPersistedState);

        // For the secondary state provider, 
        // use a faulty one that returns an error when trying to generate an operation
        ReplicatorTestWrapper wrapper(
            hasPersistedState, 
            numberOfCopyOps, 
            numberOfCopyContextOps,
            false);

        ComTestStatefulServicePartition * partition = nullptr;
        wrapper.CreatePrimary(&partition);

        // Build an idle when GetNext returns an error
        ComProxyTestReplicatorSPtr secondary;
        ::FABRIC_REPLICA_ID secondaryId = 12342;
        ComPointer<::IFabricStateProvider> stateProvider(make_com<ComTestFaultyStateProvider, ::IFabricStateProvider>(
            numberOfCopyOps, 
            numberOfCopyContextOps, 
            false,
            TestPublicApi::GetRoot()));

        ComTestStatefulServicePartition * partition1 = nullptr;
        wrapper.CreateIdle(secondaryId, stateProvider, secondary, &partition1);

        ReplicaInformation replica(
            secondaryId,
            ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
            secondary->ReplicationEndpoint,
            false);

        // Try to build a replica
        wrapper.GetPrimary()->BuildIdle(replica, false /*cancel*/, false /*successExpected*/, Common::TimeSpan::FromMilliseconds(40));
        
        // Build an idle when GetCopyContext returns an error
        ComProxyTestReplicatorSPtr secondary2;
        ::FABRIC_REPLICA_ID secondaryId2 = 12343;
        ComPointer<::IFabricStateProvider> stateProvider2(make_com<ComTestFaultyStateProvider, ::IFabricStateProvider>(
            numberOfCopyOps, 
            numberOfCopyContextOps, 
            true /*failGetCopyContext*/,
            TestPublicApi::GetRoot()));

        ComTestStatefulServicePartition * partition2 = nullptr;
        wrapper.CreateIdle(secondaryId2, stateProvider2, secondary2, &partition2);

        ReplicaInformation replica2(
            secondaryId2,
            ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
            secondary2->ReplicationEndpoint,
            false);

        // Try to build a replica
        wrapper.GetPrimary()->BuildIdle(replica2, false /*cancel*/, false /*successExpected*/, Common::TimeSpan::FromMilliseconds(40));
        
        wrapper.Close();
        secondary->Close();
        secondary2->Close();
    }
    
    BOOST_AUTO_TEST_CASE(TestCancelCatchUp)
    {
        bool hasPersistedState = true;
        int64 numberOfCopyOps = 0;
        int64 numberOfCopyContextOps = hasPersistedState ? 0 : -1;
        
        ComTestOperation::WriteInfo(
            PublicTestSource,
            "Start TestCancelCatchUp, persisted state {0}",
            hasPersistedState);

        ReplicatorTestWrapper wrapper(
            hasPersistedState, 
            numberOfCopyOps, 
            numberOfCopyContextOps,
            false);

        ComTestStatefulServicePartition * partition = nullptr;
        wrapper.CreatePrimary(&partition);

        // Create an idle replica, but do not save it inside the wrapper
        ComProxyTestReplicatorSPtr secondary;
        ::FABRIC_REPLICA_ID secondaryId = 44447;
        wrapper.CreateIdle(secondaryId, secondary);
        
        vector<ReplicaInformation> replicas;
        replicas.push_back(ReplicaInformation(
            secondaryId,
            ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
            secondary->ReplicationEndpoint,
            false));
        wrapper.GetPrimary()->BuildIdles(replicas);
        
        // Promote the idle to secondary, but also set the quorum to a big number
        // Any replicate operation will now block
        vector<ReplicaInformation> activeReplicas;
        activeReplicas.push_back(
            ReplicaInformation(
            secondaryId,
            ::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY,
            secondary->ReplicationEndpoint,
            false));
        wrapper.GetPrimary()->UpdateCatchUpReplicaSetConfiguration(
            activeReplicas, 3);

        // Close the replica, to make sure it doesn't reply
        secondary->Close();

        // Try to replicate - this will block
        // until the configuration is updated
        ManualResetEvent replicateDoneEvent;
        auto replicateOp = wrapper.GetPrimary()->BeginReplicate(
            [&wrapper, &replicateDoneEvent](AsyncOperationSPtr const& operation) -> void
            {
                ErrorCode error = wrapper.GetPrimary()->EndReplicate(1, operation);
                if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::OperationCanceled))
                {
                    VERIFY_FAIL_FMT("Replicate failed with error {0}.", error);
                }

                replicateDoneEvent.Set();
            }, AsyncOperationSPtr());
                
        // Cancel catchup
        ManualResetEvent catchUpDoneEvent;
        auto catchUpOp = wrapper.GetPrimary()->BeginWaitForCatchUpQuorum(
            ::FABRIC_REPLICA_SET_QUORUM_ALL,
            [&wrapper, &catchUpDoneEvent](AsyncOperationSPtr const& operation) -> void
            {
                ErrorCode error = wrapper.GetPrimary()->EndWaitForCatchUpQuorum(operation);
                if (error.IsSuccess())
                {
                    VERIFY_FAIL_FMT("Catch up succeeded when failure expected.");
                }

                Trace.WriteInfo(PublicTestSource, "Catch up failed with error {0}.", error);
                catchUpDoneEvent.Set();
            }, AsyncOperationSPtr());

        Threadpool::Post([&catchUpOp]() 
        {
            Trace.WriteInfo(PublicTestSource, "Cancel catch up.");
            catchUpOp->Cancel();
        });
        
        VERIFY_IS_TRUE(catchUpDoneEvent.WaitOne(10000), L"Catch up done");

        // Remove the down replica
        vector<ReplicaInformation> activeReplicas2;
        wrapper.GetPrimary()->UpdateCatchUpReplicaSetConfiguration(activeReplicas2);
        wrapper.GetPrimary()->UpdateCurrentReplicaSetConfiguration(activeReplicas2);
        VERIFY_IS_TRUE(replicateDoneEvent.WaitOne(10000), L"Replication done");
        
        wrapper.Close();
    }

    BOOST_AUTO_TEST_SUITE_END()

    void TestPublicApi::TestStateReplicator(bool hasPersistedState)
    {
        ComTestOperation::WriteInfo(
            PublicTestSource,
            "Start TestStateReplicator, persisted state {0}",
            hasPersistedState);

        int64 numberOfCopyOps = 0;
        int64 numberOfCopyContextOps = hasPersistedState ? 0 : -1;

        ReplicatorTestWrapper wrapper(
            hasPersistedState, 
            numberOfCopyOps, 
            numberOfCopyContextOps,
            false);

        // Create primary and an active replica
        ComTestStatefulServicePartition * partition = nullptr;
        wrapper.CreatePrimary(&partition);
        wrapper.AddActiveReplicas(1);
        
        wrapper.GetPrimary()->Replicate(1, true);

        wrapper.Close();

        // Test replicate after the primary is closed - should fail
        wrapper.GetPrimary()->Replicate(2, false);               
    }

    void TestPublicApi::TestDataLoss(bool changeStateOnDataLoss)
    {
        bool hasPersistedState = true;
        int64 numberOfCopyOps = 0;
        int64 numberOfCopyContextOps = hasPersistedState ? 0 : -1;
                
        ComTestOperation::WriteInfo(
            PublicTestSource,
            "Start TestDataLoss, persisted state {0}, change state on data loss {1}",
            hasPersistedState,
            changeStateOnDataLoss);

        ReplicatorTestWrapper wrapper(
            hasPersistedState, 
            numberOfCopyOps, 
            numberOfCopyContextOps,
            changeStateOnDataLoss);

        ComTestStatefulServicePartition * partition = nullptr;
        wrapper.CreatePrimary(&partition);
        wrapper.GetPrimary()->CheckCurrentProgress(0);

        FABRIC_SEQUENCE_NUMBER newProgress = changeStateOnDataLoss ? 24 : 0;
        if (changeStateOnDataLoss)
        {
            // Update epoch so that when the state provider
            // changes state, it provides new progress
            wrapper.UpdateEpoch(300, 4, newProgress);            
        }
        
        wrapper.GetPrimary()->OnDataLoss(changeStateOnDataLoss);
        wrapper.GetPrimary()->CheckCurrentProgress(newProgress);

        wrapper.Close();
    }

    bool TestPublicApi::Setup()
    {
        return TRUE;
    }
}
