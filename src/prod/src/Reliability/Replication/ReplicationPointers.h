// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// This header file contains all shared, unique, weak pointer types well-known in replication
// as well as forward declarations of corresponding types

namespace Reliability
{
    namespace ReplicationComponent
    {
        struct ReplicatorFactoryConstructorParameters;
        class IReplicatorFactory;
        typedef std::unique_ptr<IReplicatorFactory> IReplicatorFactoryUPtr;

        class REConfig;

        class REInternalSettings;
        typedef std::shared_ptr<REInternalSettings> REInternalSettingsSPtr;

        class ComOperation;
        typedef Common::ComPointer<ComOperation> ComOperationCPtr;
        // TODO: 132314: Infrastructure:  need to add ref_ptr<T>
        // Replace raw pointer with ref_ptr
        typedef ComOperation* ComOperationRawPtr;
        typedef std::vector<ComOperation*> ComOperationRawPtrVector;

        typedef std::function<void(FABRIC_SEQUENCE_NUMBER)> EnumeratorLastOpCallback;

        typedef std::shared_ptr<Common::ReaderQueue<ComOperationCPtr>> DispatchQueueSPtr;

        typedef std::function<bool(ComOperationCPtr const &, bool requestAck, FABRIC_SEQUENCE_NUMBER completedSeqNumber)> SendOperationCallback;

        typedef std::function<void(ComOperationCPtr const &)> OperationCallback;

        typedef std::function<void(ComOperation &)> OperationAckCallback;

        typedef std::function<bool(bool shouldTrace)> SendCallback;

        class ReplicaInformation;
        typedef std::vector<ReplicaInformation> ReplicaInformationVector;
        typedef ReplicaInformationVector::const_iterator ReplicaInformationVectorCIter;
        
        class OperationQueue;
        typedef std::unique_ptr<OperationQueue> OperationQueueUPtr;

        class ReliableOperationSender;
        typedef std::shared_ptr<ReliableOperationSender> ReliableOperationSenderSPtr;

        class ComProxyAsyncEnumOperationData;
        class ComProxyStateProvider;

        class CopySender;
        typedef std::shared_ptr<CopySender> CopySenderSPtr;

        class CopyContextReceiver;
        typedef std::shared_ptr<CopyContextReceiver> CopyContextReceiverSPtr;

        class ReplicationTransport;
        typedef std::shared_ptr<ReplicationTransport> ReplicationTransportSPtr;
        typedef std::weak_ptr<ReplicationTransport> ReplicationTransportWPtr;

        class ReplicaManager;
        typedef std::unique_ptr<ReplicaManager> ReplicaManagerUPtr;

        class ReplicationSession;
        typedef std::shared_ptr<ReplicationSession> ReplicationSessionSPtr;

        class Replicator;
        typedef std::shared_ptr<Replicator> ReplicatorSPtr;

        class PrimaryReplicator;
        typedef std::shared_ptr<PrimaryReplicator> PrimaryReplicatorSPtr;
        typedef std::weak_ptr<PrimaryReplicator> PrimaryReplicatorWPtr;

        class SecondaryReplicator;
        typedef std::shared_ptr<SecondaryReplicator> SecondaryReplicatorSPtr;

        class OperationStream;
        typedef std::shared_ptr<OperationStream> OperationStreamSPtr;
        typedef std::weak_ptr<OperationStream> OperationStreamWPtr;

        typedef std::pair<FABRIC_EPOCH, FABRIC_SEQUENCE_NUMBER> ProgressVectorEntry;
        typedef std::unique_ptr<ProgressVectorEntry> ProgressVectorEntryUPtr;

        class REPerformanceCounters;
        typedef std::shared_ptr<REPerformanceCounters> REPerformanceCountersSPtr;

        typedef std::function<void()> ReplicationAckProgressCallback;

        class IReplicatorHealthClient;
        typedef std::shared_ptr<IReplicatorHealthClient> IReplicatorHealthClientSPtr;

        class BatchedHealthReporter;
        typedef std::shared_ptr<BatchedHealthReporter> BatchedHealthReporterSPtr;

        class ApiMonitoringWrapper;
        typedef std::shared_ptr<ApiMonitoringWrapper> ApiMonitoringWrapperSPtr;
    }
}
