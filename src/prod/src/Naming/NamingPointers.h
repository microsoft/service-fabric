// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    typedef Common::ComPointer<IFabricServicePartitionResolutionChangeHandler> IFabricServicePartitionResolutionChangeHandlerCPtr;

    class NamingClient;
    typedef std::shared_ptr<NamingClient> NamingClientSPtr;

    class NamingService;
    typedef std::unique_ptr<NamingService> NamingServiceUPtr;
    
    class ServiceAddressTracker;
    typedef std::shared_ptr<ServiceAddressTracker> ServiceAddressTrackerSPtr;

    class ServiceAddressTrackerManager;
    typedef std::shared_ptr<ServiceAddressTrackerManager> ServiceAddressTrackerManagerSPtr;

    class PendingResolutionRequestAsyncOperation;
    typedef std::shared_ptr<PendingResolutionRequestAsyncOperation> PendingResolutionRequestAsyncOperationSPtr;

    class ResolvedServicePartition;
    typedef std::shared_ptr<ResolvedServicePartition> ResolvedServicePartitionSPtr;

    class AddressDetectionFailure;
    typedef std::shared_ptr<AddressDetectionFailure> AddressDetectionFailureSPtr;

    class NameRangeTuple;
    typedef std::unique_ptr<NameRangeTuple> NameRangeTupleUPtr;

    class FileTransferGateway;
    typedef std::unique_ptr<FileTransferGateway> FileTransferGatewayUPtr;

    class EntreeServiceTransport;
    typedef std::shared_ptr<EntreeServiceTransport> EntreeServiceTransportSPtr;
}
