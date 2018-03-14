// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability 
{ 
    namespace ReplicationComponent
    {

    using std::move;

    using namespace Common;
    using namespace Api;

    // *****************************
    // IOperationDataFactory methods
    // *****************************

    class ComManagedOperationData : public IFabricOperationData, private Common::ComUnknownBase
    {
        DENY_COPY(ComManagedOperationData)
        COM_INTERFACE_LIST1(ComManagedOperationData, IID_IFabricOperationData, IFabricOperationData)

    public:
        ComManagedOperationData(ULONG * segmentSizes, ULONG segmentSizesCount)
            : bytes_()
        {
            ULONG size = 0;
            for (ULONG i = 0; i < segmentSizesCount; ++i)
            {
                size += segmentSizes[i];
            }

            bytes_.resize(size);
            BYTE * dataPtr = bytes_.data();

            for (ULONG i = 0; i < segmentSizesCount; ++i)
            {
                FABRIC_OPERATION_DATA_BUFFER buffer;
                buffer.Buffer = dataPtr;
                buffer.BufferSize = segmentSizes[i];

                dataPtr += buffer.BufferSize;

                replicaBuffers_.push_back(buffer);
            }
        }
        
        // This does a mem cpy and stores the buffers
        ComManagedOperationData(
            ULONG bufferCount,
            FABRIC_OPERATION_DATA_BUFFER_EX1 const * buffers)
        {
            ULONG size = 0;
            for (ULONG count = 0; count < bufferCount; count++)
            {
                size += buffers[count].Count;
            }

            bytes_.resize(size);
            BYTE * dataPtr = bytes_.data();

            for (ULONG count = 0; count < bufferCount; count++)
            {
                FABRIC_OPERATION_DATA_BUFFER storeBuffer;
                storeBuffer.Buffer = dataPtr;
                storeBuffer.BufferSize = buffers[count].Count;

                auto err= memcpy_s(
                    dataPtr,
                    storeBuffer.BufferSize,
                    buffers[count].Buffer + buffers[count].Offset,
                    buffers[count].Count);

                ASSERT_IFNOT(err == ERROR_SUCCESS, "ComManagedOperationData failed to copy buffer with error code {0}", err);
                    
                dataPtr += buffers[count].Count;

                replicaBuffers_.push_back(storeBuffer);
            }
        }

        ~ComManagedOperationData()
        { 
        }

        virtual HRESULT STDMETHODCALLTYPE GetData(
            /* [out] */ ULONG * count,
            /* [retval][out] */ FABRIC_OPERATION_DATA_BUFFER const ** buffers)
        {
            if (!count || !buffers)
            {
                return ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *count = static_cast<ULONG>(replicaBuffers_.size());

            if (replicaBuffers_.empty())
            {
                *buffers = nullptr;
            }
            else
            {
                *buffers = replicaBuffers_.data();
            }

            return ComUtility::OnPublicApiReturn(S_OK);
        }

    private:
        std::vector<BYTE> bytes_;

        // This list has pointers and sizes into the above bytes_ buffer
        std::vector<FABRIC_OPERATION_DATA_BUFFER> replicaBuffers_;
    };

    ComReplicator::ComReplicator(Replicator & replicator) :
        ComUnknownBase(),
        IFabricPrimaryReplicator(),
        IFabricStateReplicator2(),
        replicator_(replicator),
        root_(replicator.shared_from_this())
    {
    }

    HRESULT ComReplicator::CreateReplicator(
        FABRIC_REPLICA_ID replicaId,
        Common::Guid const & partitionId,
        __in IFabricStatefulServicePartition * partition,
        __in IFabricStateProvider * stateProvider,
        BOOLEAN hasPersistedState,
        REInternalSettingsSPtr && config, 
        ReplicationTransportSPtr && transport,
        IReplicatorHealthClientSPtr && healthClient,
        IFabricStateReplicator **replicator)
    {
        {
            if (replicator == NULL || 
                partition == NULL || 
                stateProvider == NULL) 
            { 
                return ComUtility::OnPublicApiReturn(E_POINTER); 
            }

            ComPointer<IFabricStatefulServicePartition> partitionPointer;
            partitionPointer.SetAndAddRef(partition);

            ComPointer<IFabricStateProvider> stateProviderPointer;
            stateProviderPointer.SetAndAddRef(stateProvider);

            bool persistedState = (hasPersistedState == TRUE) ? true : false;
            ReplicatorSPtr replicatorSPtr = std::make_shared<Replicator>(
                move(config), 
                replicaId,
                partitionId,
                persistedState,
                move(ComProxyStatefulServicePartition(move(partitionPointer))), 
                move(ComProxyStateProvider(move(stateProviderPointer))), 
                Replicator::V1,
                move(transport),
                move(healthClient));

            ComPointer<ComReplicator> replicatorCPtr = make_com<ComReplicator>(*replicatorSPtr.get());
            return ComUtility::OnPublicApiReturn(replicatorCPtr->QueryInterface(::IID_IFabricStateReplicator, (LPVOID*)replicator));
        }
    }

    HRESULT ComReplicator::CreateReplicatorV1Plus(
        FABRIC_REPLICA_ID replicaId,
        Common::Guid const & partitionId,
        __in IFabricStatefulServicePartition * partition,
        __in IFabricStateProvider * stateProvider,
        BOOLEAN hasPersistedState,
        REInternalSettingsSPtr && config, 
        BOOLEAN batchEnabled,
        ReplicationTransportSPtr && transport,
        IReplicatorHealthClientSPtr && healthClient,
        IFabricStateReplicator **replicator)
    {
        UNREFERENCED_PARAMETER(batchEnabled);

        {
            if (replicator == NULL || 
                partition == NULL || 
                stateProvider == NULL) 
            { 
                return ComUtility::OnPublicApiReturn(E_POINTER); 
            }

            ComPointer<IFabricStatefulServicePartition> partitionPointer;
            partitionPointer.SetAndAddRef(partition);

            ComPointer<IFabricStateProvider> stateProviderPointer;
            stateProviderPointer.SetAndAddRef(stateProvider);

            bool persistedState = (hasPersistedState == TRUE) ? true : false;
            ReplicatorSPtr replicatorSPtr = std::make_shared<Replicator>(
                move(config), 
                replicaId,
                partitionId,
                persistedState,
                move(ComProxyStatefulServicePartition(move(partitionPointer))), 
                move(ComProxyStateProvider(move(stateProviderPointer))), 
                Replicator::V1Plus,
                move(transport),
                move(healthClient));

            ComPointer<ComReplicator> replicatorCPtr = make_com<ComReplicator>(*replicatorSPtr.get());
            return ComUtility::OnPublicApiReturn(replicatorCPtr->QueryInterface(::IID_IFabricStateReplicator, (LPVOID*)replicator));
        }
    }

    // *****************************
    // IFabricStateReplicator methods
    // *****************************
    HRESULT STDMETHODCALLTYPE ComReplicator::BeginReplicate(
        /* [in] */ IFabricOperationData *operationData,
        /* [in] */ IFabricAsyncOperationCallback *callback,
        /* [out, retval] */ FABRIC_SEQUENCE_NUMBER * sequenceNumber,
        /* [out, retval] */ IFabricAsyncOperationContext ** context)
    {
        if (replicator_.Version > Replicator::V1)
        {
            return ComUtility::OnPublicApiReturn(E_ACCESSDENIED);
        }

        if (context == NULL || callback == NULL || operationData == NULL || sequenceNumber == NULL) 
        {
            return ComUtility::OnPublicApiReturn(E_POINTER); 
        }

        ComPointer<IFabricOperationData> userOperation;
        userOperation.SetAndAddRef(operationData);
        
        // Do not allow empty data operation
        std::wstring errorMessage;
        HRESULT hr = ValidateOperation(userOperation, static_cast<LONG>(replicator_.Config->MaxReplicationMessageSize), errorMessage);
        if (FAILED(hr)) 
        {
            ReplicatorEventSource::Events->PrimaryReplicateFailed(
                replicator_.PartitionId,
                replicator_.get_ReplicationEndpointId(),
                errorMessage);

            return ComUtility::OnPublicApiReturn(hr); 
        }
        
        ComPointer<ReplicateOperation> replicateOperation = make_com<ReplicateOperation>(
            replicator_, move(userOperation));
        hr = replicateOperation->Initialize(root_, callback);
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        ComAsyncOperationContextCPtr baseOperation;
        baseOperation.SetNoAddRef(replicateOperation.DetachNoRelease());
        hr = baseOperation->Start(baseOperation);
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

#ifdef DBG
        *sequenceNumber = dynamic_cast<ReplicateOperation*>(baseOperation.GetRawPointer())->SequenceNumber;
#else
        *sequenceNumber = static_cast<ReplicateOperation*>(baseOperation.GetRawPointer())->SequenceNumber;
#endif

        *context = baseOperation.DetachNoRelease();

        return ComUtility::OnPublicApiReturn(hr);
    }

    HRESULT STDMETHODCALLTYPE ComReplicator::EndReplicate(
        /* [in] */ IFabricAsyncOperationContext *context,
        /* [out] */ FABRIC_SEQUENCE_NUMBER * sequenceNumber)
    {
        return ComUtility::OnPublicApiReturn(ReplicateOperation::End(context, sequenceNumber));
    }

    HRESULT STDMETHODCALLTYPE ComReplicator::GetReplicationStream(
        /* [out, retval] */ IFabricOperationStream **stream)
    {
        if (stream == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

        OperationStreamSPtr operationStream;
        ErrorCode error = replicator_.GetReplicationStream(operationStream);
        if (error.IsSuccess())
        {
            auto streamCPtr = make_com<ComOperationStream, IFabricOperationStream>(
                move(operationStream));
            *stream = streamCPtr.DetachNoRelease();
        }

        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    HRESULT STDMETHODCALLTYPE ComReplicator::GetCopyStream(
        /* [out, retval] */ IFabricOperationStream **stream)
    {
        if (stream == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

        OperationStreamSPtr operationStream;
        ErrorCode error = replicator_.GetCopyStream(operationStream);
        if (error.IsSuccess())
        {
            auto streamCPtr = make_com<ComOperationStream, IFabricOperationStream>(
                move(operationStream));
            *stream = streamCPtr.DetachNoRelease();
        }

        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    // *****************************
    // IFabricReplicator methods
    // *****************************
    HRESULT ComReplicator::BeginOpen(
        /* [in] */ IFabricAsyncOperationCallback *callback,
        /* [retval][out] */ IFabricAsyncOperationContext **context) 
    {
        if (context == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

        ComPointer<OpenOperation> openOperation = make_com<OpenOperation>(replicator_);

        HRESULT hr = openOperation->Initialize(root_, callback);
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        ComAsyncOperationContextCPtr baseOperation;
        baseOperation.SetNoAddRef(openOperation.DetachNoRelease());
        hr = baseOperation->Start(baseOperation);
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        *context = baseOperation.DetachNoRelease();
        return ComUtility::OnPublicApiReturn(S_OK);
    }

    HRESULT ComReplicator::EndOpen( 
        /* [in] */ IFabricAsyncOperationContext *context,
        /* [retval][out] */ IFabricStringResult **replicationEndpoint)
    { 
        return ComUtility::OnPublicApiReturn(OpenOperation::End(context, replicationEndpoint));
    }

    HRESULT ComReplicator::BeginChangeRole( 
        /* [in] */ FABRIC_EPOCH const * epoch,
        /* [in] */ FABRIC_REPLICA_ROLE role,
        /* [in] */ IFabricAsyncOperationCallback *callback,
        /* [retval][out] */ IFabricAsyncOperationContext **context) 
    {
        if (epoch == NULL || callback == NULL || context == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }
        FAIL_IF_RESERVED_FIELD_NOT_NULL(epoch);

        ComPointer<ChangeRoleOperation> changeRoleOperation = make_com<ChangeRoleOperation>(
            replicator_, 
            *epoch,
            role);

        HRESULT hr = changeRoleOperation->Initialize(root_, callback);
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        ComAsyncOperationContextCPtr baseOperation;
        baseOperation.SetNoAddRef(changeRoleOperation.DetachNoRelease());
        hr = baseOperation->Start(baseOperation);
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        *context = baseOperation.DetachNoRelease();
        return ComUtility::OnPublicApiReturn(S_OK);
    }

    HRESULT ComReplicator::EndChangeRole( 
        /* [in] */ IFabricAsyncOperationContext *context)
    { 
        return ComUtility::OnPublicApiReturn(ChangeRoleOperation::End(context));
    }

    HRESULT STDMETHODCALLTYPE ComReplicator::BeginUpdateEpoch( 
        /* [in] */ FABRIC_EPOCH const * epoch,
        /* [in] */ IFabricAsyncOperationCallback *callback,
        /* [retval][out] */ IFabricAsyncOperationContext **context)
    {
        if (epoch == NULL || callback == NULL || context == NULL) 
        { 
            return ComUtility::OnPublicApiReturn(E_POINTER); 
        }
        FAIL_IF_RESERVED_FIELD_NOT_NULL(epoch);
        ComPointer<UpdateEpochOperation> updateEpochOperation = make_com<UpdateEpochOperation>(
            replicator_, *epoch);

        HRESULT hr = updateEpochOperation->Initialize(root_, callback);
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        ComAsyncOperationContextCPtr baseOperation;
        baseOperation.SetNoAddRef(updateEpochOperation.DetachNoRelease());
        hr = baseOperation->Start(baseOperation);
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        *context = baseOperation.DetachNoRelease();
        return ComUtility::OnPublicApiReturn(S_OK);
}

    HRESULT STDMETHODCALLTYPE ComReplicator::EndUpdateEpoch( 
        /* [in] */ IFabricAsyncOperationContext *context)
    {
        return ComUtility::OnPublicApiReturn(UpdateEpochOperation::End(context));
    }

    HRESULT ComReplicator::BeginClose( 
        /* [in] */ IFabricAsyncOperationCallback *callback,
        /* [retval][out] */ IFabricAsyncOperationContext **context) 
    {
        if (callback == NULL || context == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

        ComPointer<CloseOperation> closeOperation = make_com<CloseOperation>(replicator_);

        HRESULT hr = closeOperation->Initialize(root_, callback);
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        ComAsyncOperationContextCPtr baseOperation;
        baseOperation.SetNoAddRef(closeOperation.DetachNoRelease());
        hr = baseOperation->Start(baseOperation);
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        *context = baseOperation.DetachNoRelease();
        return ComUtility::OnPublicApiReturn(S_OK);
    }
        
    HRESULT ComReplicator::EndClose( 
        /* [in] */ IFabricAsyncOperationContext *context) 
    {
        return ComUtility::OnPublicApiReturn(CloseOperation::End(context));
    }

    void ComReplicator::Abort()
    {
        replicator_.Abort();
    }

    HRESULT ComReplicator::GetCurrentProgress( 
        /* [out] */ FABRIC_SEQUENCE_NUMBER *lastSequenceNumber)
    {
        if (lastSequenceNumber == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }
        ErrorCode error = replicator_.GetCurrentProgress(*lastSequenceNumber);
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    HRESULT ComReplicator::GetCatchUpCapability( 
        /* [out] */ FABRIC_SEQUENCE_NUMBER *firstSequenceNumber)
    {
        if (firstSequenceNumber == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }
        ErrorCode error = replicator_.GetCatchUpCapability(*firstSequenceNumber);
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    HRESULT ComReplicator::UpdateReplicatorSettings(
        /* [in] */ FABRIC_REPLICATOR_SETTINGS const * replicatorSettings)
    {
        if (replicatorSettings == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }
        FABRIC_REPLICATOR_SETTINGS_EX1 * replicatorSettingsEx1 = (FABRIC_REPLICATOR_SETTINGS_EX1 *) replicatorSettings->Reserved;
        if (replicatorSettingsEx1 != NULL)
        {
            FABRIC_REPLICATOR_SETTINGS_EX2 * replicatorSettingsEx2 = (FABRIC_REPLICATOR_SETTINGS_EX2 *)replicatorSettingsEx1->Reserved;

            if (replicatorSettingsEx2 != NULL)
            {
                FABRIC_REPLICATOR_SETTINGS_EX3 * replicatorSettingsEx3 = (FABRIC_REPLICATOR_SETTINGS_EX3 *)replicatorSettingsEx2->Reserved;

                if (replicatorSettingsEx3 != NULL)
                {
                    FABRIC_REPLICATOR_SETTINGS_EX4 * replicatorSettingsEx4 = (FABRIC_REPLICATOR_SETTINGS_EX4 *)replicatorSettingsEx3->Reserved;
                    FAIL_IF_OPTIONAL_PARAM_RESERVED_FIELD_NOT_NULL(replicatorSettingsEx4);
                }
            }
        }
        
        std::wstring errorCause;
        Common::StringWriter errorWriter(errorCause);
        errorWriter.WriteLine("Security settings is the only supported setting that can be changed using UpdateReplicatorSettings");

        // Check if any non upgradable settings have changed
        bool hasBreakingChangesInNewSettings = false;
        {
            DWORD effective_initialPrimaryReplicationQueueSize = 0;
            DWORD effective_maxPrimaryReplicationQueueSize = 0;
            DWORD effective_maxPrimaryReplicationQueueMemorySize = 0;

            DWORD effective_initialSecondaryReplicationQueueSize = 0;
            DWORD effective_maxSecondaryReplicationQueueSize = 0;
            DWORD effective_maxSecondaryReplicationQueueMemorySize = 0;

            DWORD flags = replicatorSettings->Flags;
            REInternalSettingsSPtr config = replicator_.Config;

            ReplicatorSettingsUPtr inputSettings;
            auto error = ReplicatorSettings::FromPublicApi(
                *replicatorSettings,
                inputSettings);

            if (!error.IsSuccess())
            {
                return error.ToHResult();
            }

            ASSERT_IFNOT(flags == inputSettings->Flags, "Input Flags should be equal to calculated flags");

            if (!hasBreakingChangesInNewSettings &&
                (flags & FABRIC_REPLICATOR_ADDRESS) != 0)
            {
                std::wstring replicatorAddress(inputSettings->ReplicatorAddress);
                hasBreakingChangesInNewSettings = !StringUtility::AreEqualCaseInsensitive(config->ReplicatorAddress, replicatorAddress);

                if (hasBreakingChangesInNewSettings)
                    errorWriter.WriteLine("{0} mismatch {1}, {2}", "ReplicatorAddress", replicatorAddress, config->ReplicatorAddress);
            }

            // NOTE: Intentionally not comparing listen and publish address during update and ignoring it because we have the merged setting
            // while user might provide older values

            if (!hasBreakingChangesInNewSettings &&
                (flags & FABRIC_REPLICATOR_RETRY_INTERVAL) != 0)
            {
                hasBreakingChangesInNewSettings = config->RetryInterval != inputSettings->RetryInterval ?
                    true :
                    false;

                if (hasBreakingChangesInNewSettings)
                    errorWriter.WriteLine("{0} mismatch {1}, {2}", "RetryInterval", inputSettings->RetryInterval, config->RetryInterval);
            }

            if (!hasBreakingChangesInNewSettings &&
                (flags & FABRIC_REPLICATOR_BATCH_ACKNOWLEDGEMENT_INTERVAL) != 0)
            {
                hasBreakingChangesInNewSettings = config->BatchAcknowledgementInterval != inputSettings->BatchAcknowledgementInterval ?
                    true :
                    false;

                if (hasBreakingChangesInNewSettings)
                    errorWriter.WriteLine("{0} mismatch {1}, {2}", "BatchAcknowledgementInterval", inputSettings->BatchAcknowledgementInterval, config->BatchAcknowledgementInterval);
            }

            if (!hasBreakingChangesInNewSettings &&
                (flags & FABRIC_REPLICATOR_REQUIRE_SERVICE_ACK) != 0)
            {
                hasBreakingChangesInNewSettings = config->RequireServiceAck != inputSettings->RequireServiceAck ?
                    true :
                    false;

                if (hasBreakingChangesInNewSettings)
                    errorWriter.WriteLine("{0} mismatch {1}, {2}", "RequireServiceAck", inputSettings->RequireServiceAck, config->RequireServiceAck);
            }

            if (!hasBreakingChangesInNewSettings &&
                (flags & FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE) != 0)
            {
                effective_initialPrimaryReplicationQueueSize = static_cast<DWORD>(inputSettings->InitialReplicationQueueSize);
                effective_initialSecondaryReplicationQueueSize = static_cast<DWORD>(inputSettings->InitialReplicationQueueSize);

                errorWriter.WriteLine("Setting Initial Primary and Secondary Queue Size from config 'InitialReplicationQueueSize' value {0}", inputSettings->InitialReplicationQueueSize);
            }

            if (!hasBreakingChangesInNewSettings &&
                (flags & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE) != 0)
            {
                effective_maxPrimaryReplicationQueueSize = static_cast<DWORD>(inputSettings->MaxReplicationQueueSize);
                effective_maxSecondaryReplicationQueueSize = static_cast<DWORD>(inputSettings->MaxReplicationQueueSize);

                errorWriter.WriteLine("Setting Max Primary and Secondary Queue Size from config 'MaxReplicationQueueSize' value {0}", inputSettings->MaxReplicationQueueSize);
            }

            if (!hasBreakingChangesInNewSettings &&
                (flags & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE) != 0)
            {
                effective_maxPrimaryReplicationQueueMemorySize = static_cast<DWORD>(inputSettings->MaxReplicationQueueMemorySize);
                effective_maxSecondaryReplicationQueueMemorySize = static_cast<DWORD>(inputSettings->MaxReplicationQueueMemorySize);

                errorWriter.WriteLine("Setting Max Primary and Secondary Queue MemorySize from config 'MaxReplicationQueueMemorySize' value {0}", inputSettings->MaxReplicationQueueMemorySize);

                if ((flags & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE) == 0)
                {
                    effective_maxPrimaryReplicationQueueSize = 0;
                    effective_maxSecondaryReplicationQueueSize = 0;

                    errorWriter.WriteLine("Setting Max Primary and Secondary Queue Size to 0 since max memory size is configured");
                }
            }

            if (!hasBreakingChangesInNewSettings &&
                (flags & FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE) != 0)
            {
                hasBreakingChangesInNewSettings = static_cast<DWORD>(config->InitialCopyQueueSize) != inputSettings->InitialCopyQueueSize ?
                    true :
                    false;

                if (hasBreakingChangesInNewSettings)
                    errorWriter.WriteLine("{0} mismatch {1}, {2}", "InitialCopyQueueSize", inputSettings->InitialCopyQueueSize, config->InitialCopyQueueSize);
            }

            if (!hasBreakingChangesInNewSettings &&
                (flags & FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE) != 0)
            {
                hasBreakingChangesInNewSettings = static_cast<DWORD>(config->MaxCopyQueueSize) != inputSettings->MaxCopyQueueSize ?
                    true :
                    false;

                if (hasBreakingChangesInNewSettings)
                    errorWriter.WriteLine("{0} mismatch {1}, {2}", "MaxCopyQueueSize", inputSettings->MaxCopyQueueSize, config->MaxCopyQueueSize);
            }

            if (!hasBreakingChangesInNewSettings &&
                (flags & FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS) != 0)
            {
                hasBreakingChangesInNewSettings = config->SecondaryClearAcknowledgedOperations != inputSettings->SecondaryClearAcknowledgedOperations ?
                    true :
                    false;

                if (hasBreakingChangesInNewSettings)
                    errorWriter.WriteLine("{0} mismatch {1}, {2}", "SecondaryClearAcknowledgedOperations", inputSettings->SecondaryClearAcknowledgedOperations, config->SecondaryClearAcknowledgedOperations);
            }

            if (!hasBreakingChangesInNewSettings &&
                (flags & FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE) != 0)
            {
                hasBreakingChangesInNewSettings = static_cast<DWORD>(config->MaxReplicationMessageSize) != inputSettings->MaxReplicationMessageSize ?
                    true :
                    false;

                if (hasBreakingChangesInNewSettings)
                    errorWriter.WriteLine("{0} mismatch {1}, {2}", "MaxReplicationMessageSize", inputSettings->MaxReplicationMessageSize, config->MaxReplicationMessageSize);
            }

            if (!hasBreakingChangesInNewSettings &&
                (flags & FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK) != 0)
            {
                hasBreakingChangesInNewSettings = config->UseStreamFaultsAndEndOfStreamOperationAck != inputSettings->UseStreamFaultsAndEndOfStreamOperationAck ?
                    true :
                    false;

                if (hasBreakingChangesInNewSettings)
                    errorWriter.WriteLine("{0} mismatch {1}, {2}", "UseStreamFaultsAndEndOfStreamOperationAck", inputSettings->UseStreamFaultsAndEndOfStreamOperationAck, config->UseStreamFaultsAndEndOfStreamOperationAck);
            }

            if (!hasBreakingChangesInNewSettings &&
                (flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE) != 0)
            {
                effective_initialPrimaryReplicationQueueSize = static_cast<DWORD>(inputSettings->InitialPrimaryReplicationQueueSize);
                errorWriter.WriteLine("Setting Initial Primary Queue MemorySize from config 'InitialPrimaryReplicationQueueSize' value {0}", inputSettings->InitialPrimaryReplicationQueueSize);
            }

            if (!hasBreakingChangesInNewSettings &&
                (flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE) != 0)
            {
                effective_maxPrimaryReplicationQueueSize = static_cast<DWORD>(inputSettings->MaxPrimaryReplicationQueueSize);
                errorWriter.WriteLine("Setting Max Primary Queue Size from config 'MaxPrimaryReplicationQueueSize' value {0}", inputSettings->MaxPrimaryReplicationQueueSize);
            }

            if (!hasBreakingChangesInNewSettings &&
                (flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE) != 0)
            {
                effective_maxPrimaryReplicationQueueMemorySize = static_cast<DWORD>(inputSettings->MaxPrimaryReplicationQueueMemorySize);
                errorWriter.WriteLine("Setting Max Primary Queue MemorySize from config 'MaxPrimaryReplicationQueueMemorySize' value {0}", inputSettings->MaxPrimaryReplicationQueueMemorySize);
            }

            if (!hasBreakingChangesInNewSettings &&
                (flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE) != 0)
            {
                effective_initialSecondaryReplicationQueueSize = static_cast<DWORD>(inputSettings->InitialSecondaryReplicationQueueSize);
                errorWriter.WriteLine("Setting Initial Secondary Queue MemorySize from config 'InitialSecondaryReplicationQueueSize' value {0}", inputSettings->InitialSecondaryReplicationQueueSize);
            }

            if (!hasBreakingChangesInNewSettings &&
                (flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE) != 0)
            {
                effective_maxSecondaryReplicationQueueSize = static_cast<DWORD>(inputSettings->MaxSecondaryReplicationQueueSize);
                errorWriter.WriteLine("Setting Max Secondary Queue Size from config 'MaxSecondaryReplicationQueueSize' value {0}", inputSettings->MaxSecondaryReplicationQueueSize);
            }

            if (!hasBreakingChangesInNewSettings &&
                (flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE) != 0)
            {
                effective_maxSecondaryReplicationQueueMemorySize = static_cast<DWORD>(inputSettings->MaxSecondaryReplicationQueueMemorySize);
                errorWriter.WriteLine("Setting Max Secondary Queue MemorySize from config 'MaxSecondaryReplicationQueueMemorySize' value {0}", inputSettings->MaxSecondaryReplicationQueueMemorySize);
            }

            if (!hasBreakingChangesInNewSettings &&
                (flags & FABRIC_REPLICATOR_PRIMARY_WAIT_FOR_PENDING_QUORUMS_TIMEOUT) != 0)
            {
                hasBreakingChangesInNewSettings = config->PrimaryWaitForPendingQuorumsTimeout != inputSettings->PrimaryWaitForPendingQuorumsTimeout ?
                    true :
                    false;

                if (hasBreakingChangesInNewSettings)
                    errorWriter.WriteLine("{0} mismatch {1}, {2}", "PrimaryWaitForPendingQuorumsTimeout", inputSettings->PrimaryWaitForPendingQuorumsTimeout, config->PrimaryWaitForPendingQuorumsTimeout);
            }

            // Do this in the end, after all effective queue sizes are calculated above 
            if (!hasBreakingChangesInNewSettings &&
                ((flags & FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE) || (flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE)))
            {
                hasBreakingChangesInNewSettings = config->InitialPrimaryReplicationQueueSize != effective_initialPrimaryReplicationQueueSize ?
                    true :
                    false;
                
                if (hasBreakingChangesInNewSettings)
                    errorWriter.WriteLine("Current Initial Primary Queue Size {0} is not equal to input Initial Primary Queue Size {1}", config->InitialPrimaryReplicationQueueSize, effective_initialPrimaryReplicationQueueSize);
            }


            if (!hasBreakingChangesInNewSettings &&
                ((flags & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE) || (flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE)))
            {
                hasBreakingChangesInNewSettings = config->MaxPrimaryReplicationQueueSize != effective_maxPrimaryReplicationQueueSize ?
                    true :
                    false;

                if (hasBreakingChangesInNewSettings)
                    errorWriter.WriteLine("Current Max Primary Queue Size {0} is not equal to input Max Primary Queue Size {1}", config->MaxPrimaryReplicationQueueSize, effective_maxPrimaryReplicationQueueSize);
            }

            if (!hasBreakingChangesInNewSettings &&
                ((flags & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE) || (flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE)))
            {
                hasBreakingChangesInNewSettings = config->MaxPrimaryReplicationQueueMemorySize != effective_maxPrimaryReplicationQueueMemorySize ?
                    true :
                    false;

                if (hasBreakingChangesInNewSettings)
                    errorWriter.WriteLine("Current Max Primary Queue MemorySize {0} is not equal to input Max Primary Queue MemorySize {1}", config->MaxPrimaryReplicationQueueMemorySize, effective_maxPrimaryReplicationQueueMemorySize);
            }

            if (!hasBreakingChangesInNewSettings &&
                ((flags & FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE) || (flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE)))
            {
                hasBreakingChangesInNewSettings = config->InitialSecondaryReplicationQueueSize != effective_initialSecondaryReplicationQueueSize ?
                    true :
                    false;

                if (hasBreakingChangesInNewSettings)
                    errorWriter.WriteLine("Current Initial Secondary Queue Size {0} is not equal to input Initial Secondary Queue Size {1}", config->InitialSecondaryReplicationQueueSize, effective_initialSecondaryReplicationQueueSize);
            }

            if (!hasBreakingChangesInNewSettings &&
                ((flags & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE) || (flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE)))
            {
                hasBreakingChangesInNewSettings = config->MaxSecondaryReplicationQueueSize != effective_maxSecondaryReplicationQueueSize ?
                    true :
                    false;

                if (hasBreakingChangesInNewSettings)
                    errorWriter.WriteLine("Current Max Secondary Queue Size {0} is not equal to input Max Secondary Queue Size {1}", config->MaxSecondaryReplicationQueueSize, effective_maxSecondaryReplicationQueueSize);
            }

            if (!hasBreakingChangesInNewSettings &&
                ((flags & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE) || (flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE)))
            {
                hasBreakingChangesInNewSettings = config->MaxSecondaryReplicationQueueMemorySize != effective_maxSecondaryReplicationQueueMemorySize ?
                    true :
                    false;

                if (hasBreakingChangesInNewSettings)
                    errorWriter.WriteLine("Current Max Secondary Queue MemorySize {0} is not equal to input Max Secondary Queue MemorySize {1}", config->MaxSecondaryReplicationQueueMemorySize, effective_maxSecondaryReplicationQueueMemorySize);
            }
        }

        if (!hasBreakingChangesInNewSettings)
        { 
            ErrorCode error;
            if ((replicatorSettings->Flags & FABRIC_REPLICATOR_SECURITY) != 0)
            {
                if (NULL == replicatorSettings->SecurityCredentials)
                {
                    ReplicatorEventSource::Events->ReplicatorFactoryGeneralConfigError(
                        ReplicatorEngine.PartitionId,
                        ReplicatorEngine.ReplicaId,
                        L"Security Credentials is NULL, but FABRIC_REPLICATOR_SECURITY flag is enabled");

                    return ComUtility::OnPublicApiReturn(E_INVALIDARG);
                }

                Transport::SecuritySettings securitySettings;
                error = Transport::SecuritySettings::FromPublicApi(*(replicatorSettings->SecurityCredentials), securitySettings);
                if (error.IsSuccess())
                {
                    error = replicator_.UpdateSecuritySettings(securitySettings);
                }
            }

            return ComUtility::OnPublicApiReturn(error.ToHResult());
        }
        else
        {
            ReplicatorEventSource::Events->ReplicatorFactoryGeneralConfigError(
                ReplicatorEngine.PartitionId,
                ReplicatorEngine.ReplicaId,
                errorCause);

            return ComUtility::OnPublicApiReturn(E_INVALIDARG);
        }
    }

    // *****************************
    // IFabricStateReplicator2 methods
    // *****************************
    HRESULT ComReplicator::GetReplicatorSettings(
        /* [out, retval]*/ IFabricReplicatorSettingsResult ** replicatorSettings)
    {
        ReplicatorSettingsUPtr internalSettings;
        auto error = ReplicatorSettings::FromConfig(*replicator_.Config.get(), replicator_.Config->ReplicatorAddress, replicator_.Config->ReplicatorListenAddress, replicator_.Config->ReplicatorPublishAddress, *replicator_.ReplicatorTransport->SecuritySetting.get(), internalSettings);
        TESTASSERT_IFNOT(error.IsSuccess(), "Internal Replicator Settings cannot be invalid");

        return ComReplicatorSettingsResult::ReturnReplicatorSettingsResult(move(internalSettings), replicatorSettings);
    }


    // *****************************
    // IFabricPrimaryReplicator methods
    // *****************************
    HRESULT ComReplicator::UpdateCatchUpReplicaSetConfiguration( 
        /* [in] */ FABRIC_REPLICA_SET_CONFIGURATION const * currentConfiguration,
        /* [in] */ FABRIC_REPLICA_SET_CONFIGURATION const * previousConfiguration)
    {
        if (currentConfiguration == NULL ||
            ((currentConfiguration->ReplicaCount > 0) && (currentConfiguration->Replicas == NULL))) 
        { 
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }
        FAIL_IF_RESERVED_FIELD_NOT_NULL(currentConfiguration);
        FAIL_IF_OPTIONAL_PARAM_RESERVED_FIELD_NOT_NULL(previousConfiguration);
        
        ReplicaInformationVector previousReplicas;
        ULONG previousReplicaCount = 0;
        ULONG previousWriteQuorum = 0;
        if (previousConfiguration != NULL)
        {
            previousReplicaCount = previousConfiguration->ReplicaCount;
            previousWriteQuorum = previousConfiguration->WriteQuorum;
            FABRIC_REPLICA_INFORMATION const *previousActiveSecondaryReplicas = previousConfiguration->Replicas;
            if ((previousReplicaCount > 0) && (previousActiveSecondaryReplicas == NULL)) 
            { 
                return ComUtility::OnPublicApiReturn(E_POINTER);
            }

            for (ULONG i = 0; i < previousReplicaCount; ++i)
            {
                ASSERT_IFNOT(FABRIC_REPLICA_STATUS_UP == previousActiveSecondaryReplicas[i].Status, "Unexpected status");
                previousReplicas.push_back(ReplicaInformation(&previousActiveSecondaryReplicas[i]));
            } 
        }
                   
        ReplicaInformationVector currentReplicas;
        ULONG currentReplicaCount = currentConfiguration->ReplicaCount;
        ULONG currentWriteQuorum = currentConfiguration->WriteQuorum;
        FABRIC_REPLICA_INFORMATION const *currentActiveSecondaryReplicas = currentConfiguration->Replicas;
        for(ULONG i = 0; i < currentReplicaCount; ++i)
        {
            ASSERT_IFNOT(FABRIC_REPLICA_STATUS_UP == currentActiveSecondaryReplicas[i].Status, "Unexpected status");
            currentReplicas.push_back(ReplicaInformation(&currentActiveSecondaryReplicas[i]));
        }
        
        ErrorCode error = replicator_.UpdateCatchUpReplicaSetConfiguration(
            previousReplicas,
            previousWriteQuorum,
            currentReplicas, 
            currentWriteQuorum);
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    HRESULT ComReplicator::BeginWaitForCatchUpQuorum(
        /* [in] */ FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
        /* [in] */ IFabricAsyncOperationCallback *callback,
        /* [retval][out] */ IFabricAsyncOperationContext **context)
    {
        if (callback == NULL || context == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

        ComPointer<CatchupReplicaSetOperation> catchupOperation = make_com<CatchupReplicaSetOperation>(
            replicator_, catchUpMode);

        HRESULT hr = catchupOperation->Initialize(root_, callback);
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        ComAsyncOperationContextCPtr baseOperation;
        baseOperation.SetNoAddRef(catchupOperation.DetachNoRelease());
        hr = baseOperation->Start(baseOperation);
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        *context = baseOperation.DetachNoRelease();
        return ComUtility::OnPublicApiReturn(S_OK);
    }
        
    HRESULT ComReplicator::EndWaitForCatchUpQuorum( 
        /* [in] */ IFabricAsyncOperationContext *context)
    {
        return ComUtility::OnPublicApiReturn(CatchupReplicaSetOperation::End(context));
    }

    HRESULT ComReplicator::UpdateCurrentReplicaSetConfiguration( 
        /* [in] */ FABRIC_REPLICA_SET_CONFIGURATION const * currentConfiguration)
    {
        if (currentConfiguration == NULL ||
            ((currentConfiguration->ReplicaCount > 0) && (currentConfiguration->Replicas == NULL))) 
        { 
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }
        FAIL_IF_RESERVED_FIELD_NOT_NULL(currentConfiguration);
        
        ReplicaInformationVector currentReplicas;
        ULONG currentReplicaCount = currentConfiguration->ReplicaCount;
        ULONG currentWriteQuorum = currentConfiguration->WriteQuorum;
        FABRIC_REPLICA_INFORMATION const *currentActiveSecondaryReplicas = currentConfiguration->Replicas;
        for(ULONG i = 0; i < currentReplicaCount; ++i)
        {
            ASSERT_IFNOT(FABRIC_REPLICA_STATUS_UP == currentActiveSecondaryReplicas[i].Status, "Unexpected replica status");
            currentReplicas.push_back(ReplicaInformation(&currentActiveSecondaryReplicas[i]));
        }
        
        ErrorCode error = replicator_.UpdateCurrentReplicaSetConfiguration(
            currentReplicas, 
            currentWriteQuorum);
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }
          
    HRESULT ComReplicator::BeginBuildReplica( 
        /* [in] */ FABRIC_REPLICA_INFORMATION const *idleReplica,
        /* [in] */ IFabricAsyncOperationCallback *callback,
        /* [retval][out] */ IFabricAsyncOperationContext **context)
    {
        if (idleReplica == NULL || callback == NULL || context == NULL) 
        { 
            TraceWarning(
                Common::TraceTaskCodes::Replication,
                "ComReplicator",
                "E_POINTER returned from ComReplicator::BeginBuildReplica, callback==NULL: {0}, operationData==NULL: {1}, idleReplica==NULL: {2}",
                context == NULL,
                callback == NULL,
                idleReplica == NULL);                    
            return ComUtility::OnPublicApiReturn(E_POINTER); 
        }

        ASSERT_IF(idleReplica->Reserved == NULL, "Unexpected reserved field. Expected to receive FABIRC_REPLICA_INFORMATION_EX1");
        ASSERT_IFNOT(FABRIC_REPLICA_STATUS_UP == idleReplica->Status, "Unexpected replica status");

        ComPointer<BuildIdleReplicaOperation> buildIdleReplicaOperation = make_com<BuildIdleReplicaOperation>(
            replicator_, idleReplica);

        HRESULT hr = buildIdleReplicaOperation->Initialize(root_, callback);
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        ComAsyncOperationContextCPtr baseOperation;
        baseOperation.SetNoAddRef(buildIdleReplicaOperation.DetachNoRelease());
        hr = baseOperation->Start(baseOperation);
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        *context = baseOperation.DetachNoRelease();
        return ComUtility::OnPublicApiReturn(S_OK);
    }
        
    HRESULT ComReplicator::EndBuildReplica( 
        /* [in] */ IFabricAsyncOperationContext *context)
    {
        return ComUtility::OnPublicApiReturn(BuildIdleReplicaOperation::End(context));   
    }
            
    HRESULT ComReplicator::RemoveReplica(
        /* [in] */ FABRIC_REPLICA_ID replicaId)
    {
        ErrorCode error = replicator_.RemoveReplica(replicaId);
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    HRESULT ComReplicator::BeginOnDataLoss(
        /* [in] */ IFabricAsyncOperationCallback * callback,
        /* [out, retval] */ IFabricAsyncOperationContext ** context)
    {
        if (callback == NULL || context == NULL) 
        { 
            return ComUtility::OnPublicApiReturn(E_POINTER); 
        }

        ComPointer<OnDataLossOperation> onDataLossOperation = make_com<OnDataLossOperation>(
            replicator_);

        HRESULT hr = onDataLossOperation->Initialize(root_, callback);
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        ComAsyncOperationContextCPtr baseOperation;
        baseOperation.SetNoAddRef(onDataLossOperation.DetachNoRelease());
        hr = baseOperation->Start(baseOperation);
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        *context = baseOperation.DetachNoRelease();
        return ComUtility::OnPublicApiReturn(S_OK);
    }
    
    HRESULT ComReplicator::EndOnDataLoss(
        /* [in] */ IFabricAsyncOperationContext * context,
        /* [out, retval] */ BOOLEAN * isStateChanged)
    {
        return ComUtility::OnPublicApiReturn(OnDataLossOperation::End(context, isStateChanged));
    }

    // *****************************
    // IFabricInternalStateReplicator methods
    // *****************************

    HRESULT ComReplicator::BeginReplicateBatch(
        /* [in] */ IFabricBatchOperationData * batchOperationData,
        /* [in] */ IFabricAsyncOperationCallback * callback,
        /* [out, retval] */ IFabricAsyncOperationContext ** context)
    {
        UNREFERENCED_PARAMETER(batchOperationData);
        UNREFERENCED_PARAMETER(callback);
        UNREFERENCED_PARAMETER(context);
        return ComUtility::OnPublicApiReturn(E_NOTIMPL);
    }

    HRESULT ComReplicator::EndReplicateBatch(
        /* [in] */ IFabricAsyncOperationContext * context)
    {
        UNREFERENCED_PARAMETER(context);
        return ComUtility::OnPublicApiReturn(E_NOTIMPL);
    }

    HRESULT ComReplicator::ReserveSequenceNumber(
        /* [in] */ BOOLEAN alwaysReserveWhenPrimary,
        /* [out, retval] */ FABRIC_SEQUENCE_NUMBER * sequenceNumber)
    {
        UNREFERENCED_PARAMETER(alwaysReserveWhenPrimary);
        UNREFERENCED_PARAMETER(sequenceNumber);
        return ComUtility::OnPublicApiReturn(E_NOTIMPL);
    }

    HRESULT ComReplicator::GetBatchReplicationStream(
        /* [out, retval] */ IFabricBatchOperationStream **batchStream)
    {
        UNREFERENCED_PARAMETER(batchStream);
        return ComUtility::OnPublicApiReturn(E_NOTIMPL);
    }

    HRESULT ComReplicator::GetReplicationQueueCounters(
        /* [out] */ FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS * counters)
    {
        // There is no need to check that this API call is being made only on 
        // a V1Plus replicator as it only getting the queue counters 

        if (counters == NULL)
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        ErrorCode error = replicator_.GetReplicationQueueCounters(*counters);
        
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    // *****************************
    // IFabricInternalReplicator methods
    // *****************************
    HRESULT ComReplicator::GetReplicatorStatus(
        /* [out, retval] */ IFabricGetReplicatorStatusResult **replicatorStatus)
    {
        if (replicatorStatus == NULL)
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }
        
        ServiceModel::ReplicatorStatusQueryResultSPtr result;
        
        ErrorCode error = replicator_.GetReplicatorStatus(result);

        if (error.IsSuccess())
        {
            ComPointer<IFabricGetReplicatorStatusResult> replicatorStatusResult = make_com<ComGetReplicatorStatusResult, IFabricGetReplicatorStatusResult>(move(result));
            *replicatorStatus = replicatorStatusResult.DetachNoRelease();
        }

        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    HRESULT ComReplicator::CreateOperationData(
        __in ULONG * segmentSizes,
        __in ULONG segmentSizesCount,
        __out ::IFabricOperationData ** operationData)
    {
        if (!operationData)
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        auto operationDataCPtr = make_com<ComManagedOperationData>(
            segmentSizes,
            segmentSizesCount);

        *operationData = operationDataCPtr.DetachNoRelease();
        return ComUtility::OnPublicApiReturn(S_OK);
    }

    HRESULT ComReplicator::BeginReplicate2(
        /* [in] */ ULONG bufferCount,
        /* [in] */ FABRIC_OPERATION_DATA_BUFFER_EX1 const * buffers,
        /* [in] */ IFabricAsyncOperationCallback *callback,
        /* [out, retval] */ FABRIC_SEQUENCE_NUMBER * sequenceNumber,
        /* [out, retval] */ IFabricAsyncOperationContext ** context)
    {
        for (ULONG count = 0; count < bufferCount; count++)
        {
            if (buffers[count].Buffer == NULL || buffers[count].Count == 0)
            {
                return ComUtility::OnPublicApiReturn(E_POINTER); 
            }
        }

        auto operation = make_com<ComManagedOperationData>(
            bufferCount,
            buffers);
            
        HRESULT hr = BeginReplicate( 
            operation.GetRawPointer(),
            callback,
            sequenceNumber,
            context);

        return ComUtility::OnPublicApiReturn(hr);
    }


    } // end namespace ReplicationComponent
} // end namespace Reliability
