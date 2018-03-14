// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHealthClient.h"
#include "ComProxyTestReplicator.h"
#include "ComTestStatefulServicePartition.h"
#include "ComTestStateProvider.h"
#include "ComTestOperation.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
 
namespace ReplicationUnitTest
{
    using namespace Common;
    using namespace Reliability::ReplicationComponent;
    using namespace std;


    StringLiteral const ComProxyTestReplicator::ComProxyTestReplicatorSource("ComProxyTestReplicatorTest");

    void ComProxyTestReplicator::CreateComProxyTestReplicator(
            Common::Guid const & partitionId,
            __in ComReplicatorFactory & factory,
            ::FABRIC_REPLICA_ID replicaId,
            ComPointer<::IFabricStateProvider> const & provider,
            bool hasPersistedState,
            bool supportsParallelStreams,
            Replicator::ReplicatorInternalVersion version,
            bool batchEnabled,
            __out ComProxyTestReplicatorSPtr & replicatorProxy,
            __out ComTestStatefulServicePartition ** outpartition)
    {
        Common::ComPointer<::IFabricStateReplicator> replicator;
        HealthClientSPtr healthClient = ReplicationUnitTest::HealthClient::Create(true);
        HRESULT hr = S_OK;
        *outpartition = new ComTestStatefulServicePartition(partitionId);
        if (version == Replicator::V1)
        {
            hr = factory.CreateReplicator(
                replicaId,
                *outpartition,
                provider.GetRawPointer(),
                NULL /*replicatorSettings*/,
                hasPersistedState ? TRUE : FALSE,
                move(healthClient),
                replicator.InitializationAddress());
            VERIFY_SUCCEEDED(hr, L"Factory.CreateReplicator succeeded");
        }
        else if (version == Replicator::V1Plus)
        {
            hr = factory.CreateReplicatorV1Plus(
                replicaId,
                *outpartition,
                provider.GetRawPointer(),
                NULL /*replicatorSettings*/,
                hasPersistedState ? TRUE : FALSE,
                batchEnabled ? TRUE : FALSE,
                move(healthClient),
                replicator.InitializationAddress());
            VERIFY_SUCCEEDED(hr, L"Factory.CreateReplicatorV1Plus succeeded");
        }

        IFabricReplicator *replicatorObj;
        hr = replicator->QueryInterface(
            IID_IFabricReplicator, 
            reinterpret_cast<void**>(&replicatorObj));
        VERIFY_SUCCEEDED(hr, L"Query for IFabricReplicator");

        ComPointer<::IFabricReplicator> replicatorPtr;
        replicatorPtr.SetAndAddRef(replicatorObj);

        replicatorProxy = make_shared<ComProxyTestReplicator>(replicatorPtr, supportsParallelStreams, batchEnabled);
    }

    ComProxyTestReplicator::ComProxyTestReplicator(
        ComPointer<::IFabricReplicator> const & replicator,
        bool supportsParallelStreams,
        bool batchEnabled) 
        :   replicator_(replicator),
            stateReplicator_(replicator, ::IID_IFabricStateReplicator),
            stateReplicator2_(replicator, ::IID_IFabricStateReplicator2),
            internalStateReplicator_(replicator, ::IID_IFabricInternalStateReplicator),
            primary_(replicator, ::IID_IFabricPrimaryReplicator),
            supportsParallelStreams_(supportsParallelStreams),
            batchEnabled_(batchEnabled),
            endpoint_()
    {
        ASSERT_IFNOT(replicator_, "Replicator didn't implement IFabricReplicator");
        ASSERT_IFNOT(stateReplicator_, "Replicator didn't implement IFabricStateReplicator");
        ASSERT_IFNOT(stateReplicator2_, "Replicator didn't implement IFabricStateReplicator2");
        ASSERT_IFNOT(internalStateReplicator_, "Replicator didn't implement IFabricInternalStateReplicator");
        ASSERT_IFNOT(primary_, "Replicator didn't implement IFabricPrimaryReplicator");
    }

    // *********************************
    // IFabricReplicator methods
    // *********************************

    // *********************************
    // IFabricReplicator Open
    // *********************************
    class ComProxyTestReplicator::OpenAsyncOperation : public Common::ComProxyAsyncOperation
    {
        DENY_COPY(OpenAsyncOperation);
    public:
        OpenAsyncOperation(
            ComPointer<IFabricReplicator> const & repl,
            Common::AsyncCallback callback,
            Common::AsyncOperationSPtr const & parent) 
            :   Common::ComProxyAsyncOperation(callback, parent),
                repl_(repl),
                replicationEndpoint_()
        {
        }

        static ErrorCode End(
            Common::AsyncOperationSPtr const & asyncOperation, 
            __out std::wstring & replicationEndpoint)
        {
            auto casted = AsyncOperation::End<OpenAsyncOperation>(asyncOperation);
            replicationEndpoint = move(casted->replicationEndpoint_);
            return casted->Error;
        }

    protected:
        HRESULT BeginComAsyncOperation(
            IFabricAsyncOperationCallback * callback, 
            IFabricAsyncOperationContext ** context)
        {
            return repl_->BeginOpen(callback, context);
        }

        HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
        {
            HRESULT hr;
            ComPointer<IFabricStringResult> stringResult;
            hr = repl_->EndOpen(context, stringResult.InitializationAddress());

            if(SUCCEEDED(hr))
            {
                hr = StringUtility::LpcwstrToWstring(stringResult->get_String(), true /*acceptNull*/, replicationEndpoint_);
            }

            return hr;
        }

    private:
        ComPointer<IFabricReplicator> const & repl_;
        std::wstring replicationEndpoint_;
    };

    void ComProxyTestReplicator::Open()
    {
        ManualResetEvent openCompleteEvent;
        AsyncOperation::CreateAndStart<OpenAsyncOperation>(
            replicator_,
            [this, &openCompleteEvent](AsyncOperationSPtr const& asyncOperation) -> void
            {
                wstring endpoint;
                ErrorCode error = OpenAsyncOperation::End(asyncOperation, endpoint);
                if (error.IsSuccess())
                {
                    Trace.WriteInfo(ComProxyTestReplicatorSource, "Replicator opened successfully at {0}.", endpoint);
                    this->endpoint_ = move(endpoint);
                    ComTestOperation::WriteInfo(
                        ComProxyTestReplicatorSource,
                        "{0}: Replicator opened succcessfully",
                        this->endpoint_);

                }
                else
                {
                    VERIFY_FAIL(L"Error opening replicator");
                }
                openCompleteEvent.Set();
            }, AsyncOperationSPtr());

        VERIFY_IS_TRUE(openCompleteEvent.WaitOne(10000), L"Open done");
    }
     
    // *********************************
    // IFabricReplicator Close
    // *********************************
    class ComProxyTestReplicator::CloseAsyncOperation : public Common::ComProxyAsyncOperation
    {
        DENY_COPY(CloseAsyncOperation);
    public:
        CloseAsyncOperation(
            ComPointer<::IFabricReplicator> const & repl,
            Common::AsyncCallback callback,
            Common::AsyncOperationSPtr const & parent) 
            :   Common::ComProxyAsyncOperation(callback, parent),
                repl_(repl)
        {
        }

        static ErrorCode End(
            Common::AsyncOperationSPtr const & asyncOperation)
        {
            auto casted = AsyncOperation::End<CloseAsyncOperation>(asyncOperation);
            return casted->Error;
        }

    protected:
        HRESULT BeginComAsyncOperation(
            IFabricAsyncOperationCallback * callback, 
            IFabricAsyncOperationContext ** context)
        {
            return repl_->BeginClose(callback, context);
        }

        HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
        {
            return repl_->EndClose(context);
        }

    private:
        ComPointer<IFabricReplicator> const & repl_;
    };

    void ComProxyTestReplicator::Close()
    {
        ManualResetEvent closeCompleteEvent;
        AsyncOperation::CreateAndStart<CloseAsyncOperation>(
            replicator_,
            [this, &closeCompleteEvent](AsyncOperationSPtr const& asyncOperation) -> void
            {
                ErrorCode error = CloseAsyncOperation::End(asyncOperation);
                if (error.IsSuccess())
                {
                    Trace.WriteInfo(ComProxyTestReplicatorSource, "Replicator closed successfully.");
                }
                else
                {
                    VERIFY_FAIL(L"Error closing replicator");
                }
                closeCompleteEvent.Set();
            }, AsyncOperationSPtr());

        VERIFY_IS_TRUE(closeCompleteEvent.WaitOne(10000), L"Close done");
    }
    
    // *********************************
    // IFabricReplicator ChangeRole
    // *********************************
    class ComProxyTestReplicator::ChangeRoleAsyncOperation : public Common::ComProxyAsyncOperation
    {
        DENY_COPY(ChangeRoleAsyncOperation);
    public:
        ChangeRoleAsyncOperation(
            ComPointer<::IFabricReplicator> const & repl,
            ::FABRIC_EPOCH const & epoch,
            ::FABRIC_REPLICA_ROLE newRole,
            Common::AsyncCallback callback,
            Common::AsyncOperationSPtr const & parent) 
            :   Common::ComProxyAsyncOperation(callback, parent),
                repl_(repl),
                epoch_(epoch),
                role_(newRole)
        {
        }

        static ErrorCode End(
            Common::AsyncOperationSPtr const & asyncOperation)
        {
            auto casted = AsyncOperation::End<ChangeRoleAsyncOperation>(asyncOperation);
            return casted->Error;
        }

    protected:
        HRESULT BeginComAsyncOperation(
            IFabricAsyncOperationCallback * callback, 
            IFabricAsyncOperationContext ** context)
        {
            return repl_->BeginChangeRole(&epoch_, role_, callback, context);
        }

        HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
        {
            return repl_->EndChangeRole(context);
        }

    private:
        ComPointer<IFabricReplicator> const & repl_;
        ::FABRIC_EPOCH epoch_;
        ::FABRIC_REPLICA_ROLE role_;
    };

    void ComProxyTestReplicator::ChangeRole(
        ::FABRIC_EPOCH const & epoch,
        ::FABRIC_REPLICA_ROLE newRole)
    {
        ManualResetEvent changeRoleCompleteEvent;
        AsyncOperation::CreateAndStart<ChangeRoleAsyncOperation>(
            replicator_,
            epoch,
            newRole,
            [this, &changeRoleCompleteEvent](AsyncOperationSPtr const& asyncOperation) -> void
            {
                ErrorCode error = ChangeRoleAsyncOperation::End(asyncOperation);
                if (error.IsSuccess())
                {
                    Trace.WriteInfo(ComProxyTestReplicatorSource, "Replicator changed role successfully.");
                }
                else
                {
                    VERIFY_FAIL(L"Error changing replicator role");
                }
                changeRoleCompleteEvent.Set();
            }, AsyncOperationSPtr());

        VERIFY_IS_TRUE(changeRoleCompleteEvent.WaitOne(10000), L"ChangeRole done");
    }

    // *********************************
    // IFabricReplicator UpdateEpoch
    // *********************************
    class ComProxyTestReplicator::UpdateEpochAsyncOperation : public Common::ComProxyAsyncOperation
    {
    public:
        UpdateEpochAsyncOperation(
            ComPointer<::IFabricReplicator> const & replicator, 
            ::FABRIC_EPOCH const * epoch,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
              : ComProxyAsyncOperation(callback, parent),
                replicator_(replicator),
                epoch_(epoch)
        {
        }

        static ErrorCode End(
            AsyncOperationSPtr const & asyncOperation)
        {
            auto casted = AsyncOperation::End<UpdateEpochAsyncOperation>(asyncOperation);
            return casted->Error;
        }

    protected:
        virtual HRESULT BeginComAsyncOperation(
            __in IFabricAsyncOperationCallback * callback, 
            __out IFabricAsyncOperationContext ** context)
        {
            return replicator_->BeginUpdateEpoch(
                epoch_,
                callback,
                context);
        }

        virtual HRESULT EndComAsyncOperation(
            __in IFabricAsyncOperationContext * context)
        {
            return replicator_->EndUpdateEpoch(context);
        }

    private:
        ComPointer<::IFabricReplicator> const & replicator_;
        ::FABRIC_EPOCH const * epoch_;
    };

    Common::AsyncOperationSPtr ComProxyTestReplicator::BeginUpdateEpoch(
        ::FABRIC_EPOCH const * epoch,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<UpdateEpochAsyncOperation>(
            replicator_,
            epoch,
            callback,
            parent);
    }

    Common::ErrorCode ComProxyTestReplicator::EndUpdateEpoch(Common::AsyncOperationSPtr const & asyncOperation)
    {
        return UpdateEpochAsyncOperation::End(asyncOperation);
    }

    void ComProxyTestReplicator::CheckCurrentProgress(FABRIC_SEQUENCE_NUMBER expectedProgress)
    {
        FABRIC_SEQUENCE_NUMBER progress;
        HRESULT hr = replicator_->GetCurrentProgress(&progress);
        VERIFY_SUCCEEDED(hr, L"GetCurrentProgress done");
        VERIFY_ARE_EQUAL_FMT(
            progress, 
            expectedProgress, 
            "Progress {0}, expected {1}", progress, expectedProgress);
    }

    // *********************************
    // IFabricStateReplicator - Replicate
    // *********************************
    class ComProxyTestReplicator::ReplicateAsyncOperation : public Common::ComProxyAsyncOperation
    {
    public:
        ReplicateAsyncOperation(
            ComPointer<IFabricStateReplicator> const & repl, 
            ComPointer<IFabricOperationData> && op,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
              : ComProxyAsyncOperation(callback, parent),
                repl_(repl),
                opPointer_(move(op)),
                sequenceNumber_(Constants::NonInitializedLSN)
        {
        }

        static ErrorCode End(
            AsyncOperationSPtr const & asyncOperation,
            __out FABRIC_SEQUENCE_NUMBER & sequenceNumber)
        {
            auto casted = AsyncOperation::End<ReplicateAsyncOperation>(asyncOperation);
            sequenceNumber = casted->sequenceNumber_;
            return casted->Error;
        }

    protected:
        virtual HRESULT BeginComAsyncOperation(
            __in IFabricAsyncOperationCallback * callback, 
            __out IFabricAsyncOperationContext ** context)
        {
            return repl_->BeginReplicate(
                opPointer_.GetRawPointer(), 
                callback, 
                &sequenceNumber_,
                context);
        }

        virtual HRESULT EndComAsyncOperation(
            __in IFabricAsyncOperationContext * context)
        {
            FABRIC_SEQUENCE_NUMBER sequenceNumber = 0xBAADF00D;
            HRESULT result = repl_->EndReplicate(context, &sequenceNumber);

            if (SUCCEEDED(result))
            {
                ASSERT_IFNOT(sequenceNumber == this->sequenceNumber_, "Sequence numbers don't match");
            }

            return result;
        }

    private:
        ComPointer<IFabricStateReplicator> const & repl_;
        ComPointer<IFabricOperationData> opPointer_;
        FABRIC_SEQUENCE_NUMBER sequenceNumber_;
    };

    // *********************************
    // IFabricInternalStateReplicator - ReplicateBatch
    // *********************************
    class ComProxyTestReplicator::ReplicateBatchAsyncOperation : public Common::ComProxyAsyncOperation
    {
    public:
        ReplicateBatchAsyncOperation(
            ComPointer<IFabricInternalStateReplicator> const & repl, 
            ComPointer<IFabricBatchOperationData> && batchOp,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
              : ComProxyAsyncOperation(callback, parent),
                repl_(repl),
                batchOpPointer_(move(batchOp))
        {
        }

        static ErrorCode End(
            AsyncOperationSPtr const & asyncOperation)
        {
            auto casted = AsyncOperation::End<ReplicateBatchAsyncOperation>(asyncOperation);
            return casted->Error;
        }

    protected:
        virtual HRESULT BeginComAsyncOperation(
            __in IFabricAsyncOperationCallback * callback, 
            __out IFabricAsyncOperationContext ** context)
        {
            return repl_->BeginReplicateBatch(
                batchOpPointer_.GetRawPointer(), 
                callback, 
                context);
        }

        virtual HRESULT EndComAsyncOperation(
            __in IFabricAsyncOperationContext * context)
        {
            HRESULT result = repl_->EndReplicateBatch(context);

            return result;
        }

    private:
        ComPointer<IFabricInternalStateReplicator> const & repl_;
        ComPointer<IFabricBatchOperationData> batchOpPointer_;
    };

    void ComProxyTestReplicator::Replicate(
        FABRIC_SEQUENCE_NUMBER expectedSequenceNumber,
        bool expectSuccess)
    {
        ManualResetEvent replicateDoneEvent;
        ComPointer<IFabricOperationData> op = make_com<ComTestOperation, IFabricOperationData>(
            L"ReplicateCopyTest - test replicate operation");
        auto asyncOp = AsyncOperation::CreateAndStart<ReplicateAsyncOperation>(
            stateReplicator_,
            move(op),
            [&replicateDoneEvent, expectedSequenceNumber, expectSuccess] (AsyncOperationSPtr const & asyncOperation)
            {
                FABRIC_SEQUENCE_NUMBER sequenceNumber;
                ErrorCode error = ReplicateAsyncOperation::End(asyncOperation, sequenceNumber);
                bool success = error.IsSuccess();
                VERIFY_ARE_EQUAL_FMT(
                    success, 
                    expectSuccess, 
                        "Expected success verified ({0}), operation completed with error code {1}", 
                        expectSuccess,
                        error);

                if (success)
                {
                    Trace.WriteInfo(ComProxyTestReplicatorSource, "Replicate operation returned {0} sequence number.", sequenceNumber);
                    VERIFY_ARE_EQUAL(expectedSequenceNumber, sequenceNumber, 
                        L"Replicate operation returned expected sequence number");
                }

                replicateDoneEvent.Set();
            },
            AsyncOperationSPtr());

        replicateDoneEvent.WaitOne();  
    }

    AsyncOperationSPtr ComProxyTestReplicator::BeginReplicate(
        Common::AsyncCallback const & callback, 
        Common::AsyncOperationSPtr const & parent)
    {
        ComPointer<IFabricOperationData> op = make_com<ComTestOperation, IFabricOperationData>(
            L"ReplicateCopyTest - test replicate operation");
        return AsyncOperation::CreateAndStart<ReplicateAsyncOperation>(
            stateReplicator_,
            move(op),
            callback,
            parent);
    }

    AsyncOperationSPtr ComProxyTestReplicator::BeginReplicate(
        ComPointer<IFabricOperationData> && op,
        Common::AsyncCallback const & callback, 
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<ReplicateAsyncOperation>(
            stateReplicator_,
            move(op),
            callback,
            parent);
    }

    Common::ErrorCode ComProxyTestReplicator::EndReplicate(
        FABRIC_SEQUENCE_NUMBER expectedSequenceNumber,
        Common::AsyncOperationSPtr const & asyncOperation)
    {
        FABRIC_SEQUENCE_NUMBER sequenceNumber;
        ErrorCode error = ReplicateAsyncOperation::End(asyncOperation, sequenceNumber);
        if (error.IsSuccess())
        {
            VERIFY_ARE_EQUAL(expectedSequenceNumber, sequenceNumber, 
                L"Replicate operation returned expected sequence number");
        }

        return error;
    }

    AsyncOperationSPtr ComProxyTestReplicator::BeginReplicateBatch(
        ComPointer<IFabricBatchOperationData> && batchOp,
        Common::AsyncCallback const & callback, 
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<ReplicateBatchAsyncOperation>(
            internalStateReplicator_,
            move(batchOp),
            callback,
            parent);
    }

    Common::ErrorCode ComProxyTestReplicator::EndReplicateBatch(
        Common::AsyncOperationSPtr const & asyncOperation)
    {
        return ReplicateBatchAsyncOperation::End(asyncOperation);
    }

    Common::ErrorCode ComProxyTestReplicator::ReserveSequenceNumber(
        BOOLEAN alwaysReserveWhenPrimary,
        FABRIC_SEQUENCE_NUMBER * sequenceNumber)
    {
        return ErrorCode::FromHResult(internalStateReplicator_->ReserveSequenceNumber(alwaysReserveWhenPrimary, sequenceNumber));
    }

    // *********************************
    // IFabricStateReplicator - Operation pump
    // *********************************
    class ComProxyTestReplicator::GetOperationAsyncOperation : public ComProxyAsyncOperation
    {
    public:
        GetOperationAsyncOperation(
            ComPointer<::IFabricOperationStream> const & stream, 
            std::wstring const & purpose,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
                : ComProxyAsyncOperation(callback, parent),
                stream_(stream),
                purpose_(purpose),
                opPointer_()
        {
        }

        static ErrorCode End(
            AsyncOperationSPtr const & asyncOperation,
            __out ComPointer<IFabricOperation> & op,
            __out wstring & purpose)
        {
            auto casted = AsyncOperation::End<GetOperationAsyncOperation>(asyncOperation);
            std::swap(op, casted->opPointer_);
            std::swap(purpose, casted->purpose_);
            return casted->Error;
        }

    protected:
        virtual HRESULT BeginComAsyncOperation(
            __in IFabricAsyncOperationCallback * callback, 
            __out IFabricAsyncOperationContext ** context)
        {
            return stream_->BeginGetOperation(callback, context);
        }

        virtual HRESULT EndComAsyncOperation(
            __in IFabricAsyncOperationContext * context)
        {
            return stream_->EndGetOperation(context, opPointer_.InitializationAddress());
        }

    private:
        ComPointer<::IFabricOperationStream> stream_;
        wstring purpose_;
        ComPointer<IFabricOperation> opPointer_;
    };
        
    class ComProxyTestReplicator::GetBatchOperationAsyncOperation : public ComProxyAsyncOperation
    {
    public:
        GetBatchOperationAsyncOperation(
            ComPointer<::IFabricBatchOperationStream> const & batchStream, 
            std::wstring const & purpose,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
                : ComProxyAsyncOperation(callback, parent),
                batchStream_(batchStream),
                purpose_(purpose),
                batchOpPointer_()
        {
        }

        static ErrorCode End(
            AsyncOperationSPtr const & asyncOperation,
            __out ComPointer<IFabricBatchOperation> & op,
            __out wstring & purpose)
        {
            auto casted = AsyncOperation::End<GetBatchOperationAsyncOperation>(asyncOperation);
            std::swap(op, casted->batchOpPointer_);
            std::swap(purpose, casted->purpose_);
            return casted->Error;
        }

    protected:
        virtual HRESULT BeginComAsyncOperation(
            __in IFabricAsyncOperationCallback * callback, 
            __out IFabricAsyncOperationContext ** context)
        {
            return batchStream_->BeginGetBatchOperation(callback, context);
        }

        virtual HRESULT EndComAsyncOperation(
            __in IFabricAsyncOperationContext * context)
        {
            return batchStream_->EndGetBatchOperation(context, batchOpPointer_.InitializationAddress());
        }

    private:
        ComPointer<::IFabricBatchOperationStream> batchStream_;
        wstring purpose_;
        ComPointer<IFabricBatchOperation> batchOpPointer_;
    };
        
    void ComProxyTestReplicator::StartOperationPump(
        ComPointer<::IFabricOperationStream> const & stream,
        wstring const & streamPurpose,
        ComProxyTestReplicatorSPtr const & root)
    {
        for(;;)
        {
            auto op = AsyncOperation::CreateAndStart<GetOperationAsyncOperation>(
                stream, 
                streamPurpose,
                [root, stream, streamPurpose, this](AsyncOperationSPtr const& asyncOp)->void 
                { 
                    if (!asyncOp->CompletedSynchronously && 
                       FinishGetOperation(asyncOp, root))
                    {
                        StartOperationPump(stream, streamPurpose, root);
                    }
                }, 
                AsyncOperationSPtr());

            if (!op->CompletedSynchronously)
            {
                // Callback will be executed
                break;
            }
            else if(!FinishGetOperation(op, root))
            {
                // The last operation was received, no need to continue
                break;
            }
        }
    }

    void ComProxyTestReplicator::StartBatchOperationPump(
        ComPointer<::IFabricBatchOperationStream> const & batchStream,
        wstring const & streamPurpose,
        ComProxyTestReplicatorSPtr const & root)
    {
        for(;;)
        {
            auto op = AsyncOperation::CreateAndStart<GetBatchOperationAsyncOperation>(
                batchStream, 
                streamPurpose,
                [root, batchStream, streamPurpose, this](AsyncOperationSPtr const& asyncOp)->void 
                { 
                    if (!asyncOp->CompletedSynchronously && FinishGetBatchOperation(asyncOp))
                    {
                        StartBatchOperationPump(batchStream, streamPurpose, root);
                    }
                }, 
                AsyncOperationSPtr());

            if (!op->CompletedSynchronously)
            {
                // Callback will be executed
                break;
            }
            else if(!FinishGetBatchOperation(op))
            {
                // The last operation was received, no need to continue
                break;
            }
        }
    }

    bool ComProxyTestReplicator::FinishGetBatchOperation(
        AsyncOperationSPtr const & asyncOperation)
    {
        ComPointer<IFabricBatchOperation> batchOp;
        wstring streamPurpose;
        ErrorCode error = GetBatchOperationAsyncOperation::End(
            asyncOperation, batchOp, streamPurpose);
        if (!error.IsSuccess()) 
        { 
            VERIFY_FAIL_FMT("GetOperation returned error {0}", error); 
        }

        if (replicationBatchOperationProcessor_)
        {
            replicationBatchOperationProcessor_(batchOp);
        }

        if (batchOp.GetRawPointer())
        {
            FABRIC_SEQUENCE_NUMBER firstSequenceNumber = batchOp->get_FirstSequenceNumber();
            FABRIC_SEQUENCE_NUMBER lastSequenceNumber = batchOp->get_LastSequenceNumber();

            ComTestOperation::WriteInfo(
                ComProxyTestReplicatorSource,
                "{0}: GetBatchOperation returned lsn range [{1}, {2}]",
                streamPurpose, 
                firstSequenceNumber,
                lastSequenceNumber);

/*            ULONG bufferCount = 0;
            FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;
            VERIFY_SUCCEEDED(op->GetData(&bufferCount, &replicaBuffers));*/

            if (firstSequenceNumber <= 0)
            {
                VERIFY_FAIL(L"EndGetBatchOperation returned non-null op with invalid firstSequenceNumber");
            }
            if (lastSequenceNumber <= 0)
            {
                VERIFY_FAIL(L"EndGetBatchOperation returned non-null op with invalid lastSequenceNumber");
            }
            batchOp->Acknowledge();
            return true;
        }
        else
        {
            ComTestOperation::WriteInfo(ComProxyTestReplicatorSource, "{0}: Received last NULL batch operation", streamPurpose);
            return false;
        }
    }

    bool ComProxyTestReplicator::FinishGetOperation(
        AsyncOperationSPtr const & asyncOperation,
        ComProxyTestReplicatorSPtr const & root)
    {
        ComPointer<IFabricOperation> op;
        wstring streamPurpose;
        ErrorCode error = GetOperationAsyncOperation::End(
            asyncOperation, op, streamPurpose);
        if (!error.IsSuccess()) 
        { 
            VERIFY_FAIL_FMT("GetOperation returned error {0}", error); 
        }

        if (replicationOperationProcessor_)
        {
            replicationOperationProcessor_(op);
        }

        if (op.GetRawPointer())
        {
            FABRIC_OPERATION_METADATA const *opMetadata = op->get_Metadata();

            ComTestOperation::WriteInfo(
                ComProxyTestReplicatorSource,
                "{0}: GetOperation returned {1} {2}",
                streamPurpose, 
                opMetadata->Type,
                opMetadata->SequenceNumber);

            ULONG bufferCount = 0;
            FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;
            VERIFY_SUCCEEDED(op->GetData(&bufferCount, &replicaBuffers));

            if (opMetadata->SequenceNumber <= 0)
            {
                VERIFY_FAIL(L"EndGetOperation returned non-null op with invalid lsn");
            }
            op->Acknowledge();
            return true;
        }
        else
        {
            ComTestOperation::WriteInfo(ComProxyTestReplicatorSource, "{0}: Received last NULL operation", streamPurpose);
            if (streamPurpose == Constants::CopyOperationTrace)
            {
                // The copy stream is done.
                // Start replication pump.
                if (!supportsParallelStreams_)
                {
                    if (batchEnabled_)
                    {
                        StartBatchReplicationOperationPump(root);
                    }
                    else
                    {
                        StartReplicationOperationPump(root);
                    }
                }
            }
            return false;
        }
    }
        
    void ComProxyTestReplicator::StartCopyOperationPump(ComProxyTestReplicatorSPtr const & root)
    {
        Common::ComPointer<::IFabricOperationStream> copyStream;
        VERIFY_SUCCEEDED(stateReplicator_->GetCopyStream(copyStream.InitializationAddress()));
        StartOperationPump(copyStream, Constants::CopyOperationTrace, root);
    }

    void ComProxyTestReplicator::StartReplicationOperationPump(ComProxyTestReplicatorSPtr const & root)
    {
        Common::ComPointer<::IFabricOperationStream> replicationStream;
        HRESULT hr = stateReplicator_->GetReplicationStream(replicationStream.InitializationAddress());
        if (SUCCEEDED(hr))
        {
            StartOperationPump(replicationStream, Constants::ReplOperationTrace, root);
        }
        else
        {
            VERIFY_IS_TRUE(ErrorCode::FromHResult(hr).IsError(Common::ErrorCodeValue::InvalidState) ||
                           ErrorCode::FromHResult(hr).IsError(Common::ErrorCodeValue::ObjectClosed), 
                           L"Unknown error returned from GetReplicationStream()");
        }
    }

    void ComProxyTestReplicator::StartBatchReplicationOperationPump(ComProxyTestReplicatorSPtr const & root)
    {
        Common::ComPointer<::IFabricBatchOperationStream> batchReplicationStream;
        HRESULT hr = internalStateReplicator_->GetBatchReplicationStream(batchReplicationStream.InitializationAddress());
        if (SUCCEEDED(hr))
        {
            StartBatchOperationPump(batchReplicationStream, Constants::ReplOperationTrace, root);
        }
        else
        {
            VERIFY_IS_TRUE(ErrorCode::FromHResult(hr).IsError(Common::ErrorCodeValue::InvalidState) ||
                           ErrorCode::FromHResult(hr).IsError(Common::ErrorCodeValue::ObjectClosed), 
                           L"Unknown error returned from GetReplicationStream()");
        }
    }

    // *********************************
    // IFabricPrimaryReplicator methods
    // *********************************

    // *********************************
    // IFabricPrimaryReplicator UpdateConfig
    // *********************************
    void ComProxyTestReplicator::UpdateCurrentReplicaSetConfiguration(
        Reliability::ReplicationComponent::ReplicaInformationVector const & currentReplicas)
    {
        ULONG currentQuorum = static_cast<ULONG>((currentReplicas.size() + 1) / 2 + 1);
        vector<FABRIC_REPLICA_INFORMATION> currentSecondaryReplicas;
        for(auto iter = currentReplicas.begin(); iter != currentReplicas.end(); ++iter)
        {
            FABRIC_REPLICA_INFORMATION replicaInfo = *(iter->Value);
            replicaInfo.Status = FABRIC_REPLICA_STATUS_UP;
            currentSecondaryReplicas.push_back(replicaInfo);
        }

        ULONG currentActiveSecondaryReplicaCount = static_cast<ULONG>(currentSecondaryReplicas.size());
        FABRIC_REPLICA_INFORMATION * currentActiveSecondaryReplicas = currentSecondaryReplicas.data();

        FABRIC_REPLICA_SET_CONFIGURATION currentConfig;
        currentConfig.ReplicaCount = currentActiveSecondaryReplicaCount;
        currentConfig.WriteQuorum = currentQuorum;
        currentConfig.Replicas = currentActiveSecondaryReplicas;
        currentConfig.Reserved = NULL;

        HRESULT hr = primary_->UpdateCurrentReplicaSetConfiguration(&currentConfig);
        VERIFY_SUCCEEDED(hr, L"UpdateCurrentReplicaSetConfiguration done");
    }

    void ComProxyTestReplicator::UpdateCatchUpReplicaSetConfiguration(
        Reliability::ReplicationComponent::ReplicaInformationVector const & currentReplicas)
    {
        ULONG currentQuorum = static_cast<ULONG>((currentReplicas.size() + 1) / 2 + 1);
        ULONG previousQuorum = 0;
        Reliability::ReplicationComponent::ReplicaInformationVector previousReplicas;
        return UpdateCatchUpReplicaSetConfiguration(previousReplicas, previousQuorum, currentReplicas, currentQuorum);
    }

    void ComProxyTestReplicator::UpdateCatchUpReplicaSetConfiguration(
        Reliability::ReplicationComponent::ReplicaInformationVector const & currentReplicas,
        ULONG currentQuorum)
    {
        ULONG previousQuorum = 0;
        Reliability::ReplicationComponent::ReplicaInformationVector previousReplicas;
        return UpdateCatchUpReplicaSetConfiguration(previousReplicas, previousQuorum, currentReplicas, currentQuorum);
    }

    void ComProxyTestReplicator::UpdateCatchUpReplicaSetConfiguration(
        Reliability::ReplicationComponent::ReplicaInformationVector const & previousReplicas,
        ULONG previousQuorum,
        Reliability::ReplicationComponent::ReplicaInformationVector const & currentReplicas,
        ULONG currentQuorum)
    {
        vector<FABRIC_REPLICA_INFORMATION> currentSecondaryReplicas;
        for(auto iter = currentReplicas.begin(); iter != currentReplicas.end(); ++iter)
        {
            FABRIC_REPLICA_INFORMATION replicaInfo = *(iter->Value);
            replicaInfo.Status = FABRIC_REPLICA_STATUS_UP;
            currentSecondaryReplicas.push_back(replicaInfo);
        }

        FABRIC_REPLICA_INFORMATION * currentActiveSecondaryReplicas = currentSecondaryReplicas.data();
        ULONG currentActiveSecondaryReplicaCount = static_cast<ULONG>(currentSecondaryReplicas.size());

        FABRIC_REPLICA_SET_CONFIGURATION currentConfig;
        currentConfig.ReplicaCount = currentActiveSecondaryReplicaCount;
        currentConfig.WriteQuorum = currentQuorum;
        currentConfig.Replicas = currentActiveSecondaryReplicas;
        currentConfig.Reserved = NULL;

        HRESULT hr;
        if (previousQuorum != 0)
        {
            vector<FABRIC_REPLICA_INFORMATION> previousSecondaryReplicas;
            for(auto iter = previousReplicas.begin(); iter != previousReplicas.end(); ++iter)
            {
                FABRIC_REPLICA_INFORMATION replicaInfo = *(iter->Value);
                replicaInfo.Status = FABRIC_REPLICA_STATUS_UP;
                previousSecondaryReplicas.push_back(replicaInfo);
            }

            FABRIC_REPLICA_INFORMATION * previousActiveSecondaryReplicas = previousSecondaryReplicas.data();
            ULONG previousActiveSecondaryReplicaCount = static_cast<ULONG>(previousSecondaryReplicas.size());

            FABRIC_REPLICA_SET_CONFIGURATION previousConfig;
            previousConfig.ReplicaCount = previousActiveSecondaryReplicaCount;
            previousConfig.WriteQuorum = previousQuorum;
            previousConfig.Replicas = previousActiveSecondaryReplicas;
            previousConfig.Reserved = NULL;
            hr = primary_->UpdateCatchUpReplicaSetConfiguration(&currentConfig, &previousConfig);
        }
        else
        {
            hr = primary_->UpdateCatchUpReplicaSetConfiguration(&currentConfig, NULL);
        }
        VERIFY_SUCCEEDED(hr, L"UpdateCatchUpReplicaSetConfiguration done");
    }

    // *********************************
    // IFabricPrimaryReplicator CatchUp
    // *********************************
    class ComProxyTestReplicator::CatchUpAsyncOperation : public Common::ComProxyAsyncOperation
    {
        DENY_COPY(CatchUpAsyncOperation);
    public:
        CatchUpAsyncOperation(
            ComPointer<IFabricPrimaryReplicator> const & repl,
            ::FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            :   Common::ComProxyAsyncOperation(callback, parent),
                repl_(repl),
                catchUpMode_(catchUpMode)
        {
        }            

        static Common::ErrorCode End(Common::AsyncOperationSPtr const & asyncOperation)
        {
            auto casted = AsyncOperation::End<CatchUpAsyncOperation>(asyncOperation);
            return casted->Error;
        }

    protected:
        HRESULT BeginComAsyncOperation(
            IFabricAsyncOperationCallback * callback, 
            IFabricAsyncOperationContext ** context)
        {
            return repl_->BeginWaitForCatchUpQuorum(catchUpMode_, callback, context);
        }

        HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
        {
            return repl_->EndWaitForCatchUpQuorum(context);
        }

    private:
        ComPointer<IFabricPrimaryReplicator> const & repl_;
        ::FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode_;
    };

    AsyncOperationSPtr ComProxyTestReplicator::BeginWaitForCatchUpQuorum( 
        ::FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
        Common::AsyncCallback const & callback, 
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<CatchUpAsyncOperation>(
            primary_, catchUpMode, callback, parent);
    }

    ErrorCode ComProxyTestReplicator::EndWaitForCatchUpQuorum(Common::AsyncOperationSPtr const & asyncOperation)
    {
        return CatchUpAsyncOperation::End(asyncOperation);
    }
    
    // *********************************
    // IFabricPrimaryReplicator Build Idle
    // *********************************
    class ComProxyTestReplicator::BuildIdleAsyncOperation : public Common::ComProxyAsyncOperation
    {
        DENY_COPY(BuildIdleAsyncOperation);
    public:
        BuildIdleAsyncOperation(
            ComPointer<IFabricPrimaryReplicator> const & repl,
            ReplicaInformation const & replica,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            :   Common::ComProxyAsyncOperation(callback, parent),
                repl_(repl),
                replica_(replica)
        {
        }            

        static Common::ErrorCode End(Common::AsyncOperationSPtr const & asyncOperation)
        {
            auto casted = AsyncOperation::End<BuildIdleAsyncOperation>(asyncOperation);
            return casted->Error;
        }

    protected:
        HRESULT BeginComAsyncOperation(
            IFabricAsyncOperationCallback * callback, 
            IFabricAsyncOperationContext ** context)
        {
            FABRIC_REPLICA_INFORMATION replicaInfo = *replica_.Value;
            replicaInfo.Status = FABRIC_REPLICA_STATUS_UP;
            return repl_->BeginBuildReplica(&replicaInfo, callback, context);
        }

        HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
        {
            return repl_->EndBuildReplica(context);
        }

    private:
        ComPointer<IFabricPrimaryReplicator> const & repl_;
        ReplicaInformation const & replica_;
    };

    AsyncOperationSPtr ComProxyTestReplicator::BeginBuildReplica( 
        ReplicaInformation const & idleReplicaDescription,
        Common::AsyncCallback const & callback, 
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<BuildIdleAsyncOperation>(
            primary_, idleReplicaDescription, callback, parent);
    }

    ErrorCode ComProxyTestReplicator::EndBuildReplica(Common::AsyncOperationSPtr const & asyncOperation)
    {
        return BuildIdleAsyncOperation::End(asyncOperation);
    }

    void ComProxyTestReplicator::BuildIdle(
        ReplicaInformation const & replica,
        bool cancel,
        bool successExpected,
        Common::TimeSpan const & waitTimeBeforeCancel)
    {
        ManualResetEvent copyDoneEvent;
        ::FABRIC_REPLICA_ID replicaId = replica.Id;
        auto copyOp = BeginBuildReplica(
            replica,
            [this, replicaId, &copyDoneEvent, successExpected](AsyncOperationSPtr const& asyncOperation) -> void
            {
                ErrorCode error = this->EndBuildReplica(asyncOperation);
                VERIFY_ARE_EQUAL_FMT(error.IsSuccess(), successExpected,
                    "Copy {0}: returned {0}, expected success {1}.", 
                    replicaId, 
                    error, 
                    successExpected);
                copyDoneEvent.Set();
            }, AsyncOperationSPtr());
        
        if (cancel)
        {
            Sleep(static_cast<int>(waitTimeBeforeCancel.TotalMilliseconds()));
            Trace.WriteInfo(ComProxyTestReplicatorSource, "Cancel copy.");
            copyOp->Cancel();
        }

        VERIFY_IS_TRUE(copyDoneEvent.WaitOne(10000), L"BuildIdle done");
    }
       
    void ComProxyTestReplicator::BuildIdles(
        vector<ReplicaInformation> const & replicas)
    {
        ManualResetEvent copyDoneEvent;
        volatile size_t count = replicas.size();

        for(size_t i = 0; i < replicas.size(); ++i)
        {
            BeginBuildReplica(
                replicas[i],
                [this, &count, &copyDoneEvent](AsyncOperationSPtr const& asyncOperation) -> void
                {
                    ErrorCode error = this->EndBuildReplica(asyncOperation);
                    if (!error.IsSuccess())
                    {
                        VERIFY_FAIL_FMT("Copy failed with error {0}.", error);
                    }

                    if (0 == InterlockedDecrement64((volatile LONGLONG *)&count))
                    {
                        copyDoneEvent.Set();
                    }

                    Trace.WriteInfo(ComProxyTestReplicatorSource, "Copy done; {0} IDLE replicas to build left.", count);
                }, AsyncOperationSPtr());
        }

        VERIFY_IS_TRUE(copyDoneEvent.WaitOne(10000), L"BuildReplica done");
    }

    // *********************************
    // IFabricPrimaryReplicator OnDataLoss
    // *********************************
    class ComProxyTestReplicator::OnDataLossAsyncOperation : public Common::ComProxyAsyncOperation
    {
        DENY_COPY(OnDataLossAsyncOperation);
    public:
        OnDataLossAsyncOperation(
            ComPointer<::IFabricPrimaryReplicator> const & repl,
            Common::AsyncCallback callback,
            Common::AsyncOperationSPtr const & parent) 
            :   Common::ComProxyAsyncOperation(callback, parent),
                repl_(repl),
                isStateChanged_(false)
        {
        }

        static ErrorCode End(
            Common::AsyncOperationSPtr const & asyncOperation, 
            __out bool & isStateChanged)
        {
            auto casted = AsyncOperation::End<OnDataLossAsyncOperation>(asyncOperation);
            isStateChanged = casted->isStateChanged_;
            return casted->Error;
        }

    protected:
        HRESULT BeginComAsyncOperation(
            IFabricAsyncOperationCallback * callback, 
            IFabricAsyncOperationContext ** context)
        {
            return repl_->BeginOnDataLoss(callback, context);
        }

        HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
        {
            HRESULT hr;
            BOOLEAN stateChanged; 
            hr = repl_->EndOnDataLoss(context, &stateChanged);
            if(SUCCEEDED(hr))
            {
                isStateChanged_ = (stateChanged == TRUE) ? true : false;
            }

            return hr;
        }

    private:
        ComPointer<::IFabricPrimaryReplicator> const & repl_;
        bool isStateChanged_;
    };

    void ComProxyTestReplicator::OnDataLoss(bool expectStateChanged)
    {
        ManualResetEvent onDataLossCompleteEvent;
        AsyncOperation::CreateAndStart<OnDataLossAsyncOperation>(
            primary_,
            [this, &onDataLossCompleteEvent, &expectStateChanged](AsyncOperationSPtr const& asyncOperation) -> void
            {
                bool stateChanged;
                ErrorCode error = OnDataLossAsyncOperation::End(asyncOperation, stateChanged);
                if (error.IsSuccess())
                {
                    Trace.WriteInfo(ComProxyTestReplicatorSource, "Replicator onDataLoss completed successfully with {0} state changed.", stateChanged);
                    VERIFY_ARE_EQUAL_FMT(
                        expectStateChanged,
                        stateChanged,
                            "OnDataLoss change state: expected {0}, actual {1}", 
                            expectStateChanged, 
                            stateChanged);
                }
                else
                {
                    VERIFY_FAIL(L"Error onDataLoss");
                }

                onDataLossCompleteEvent.Set();
            }, AsyncOperationSPtr());

        VERIFY_IS_TRUE(onDataLossCompleteEvent.WaitOne(10000), L"OnDataLoss done");
    }
}
