// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "service.h"
#include "Util.h"

namespace NativeServiceGroupMember
{
    class StatefulServiceImplementation : 
        public ISimpleAdder,
        public IFabricStatefulServiceReplica,
        public IFabricStateProvider,
        private ComUnknownBase
    {
        DENY_COPY(StatefulServiceImplementation)

        BEGIN_COM_INTERFACE_LIST(StatefulServiceImplementation)
            COM_INTERFACE_ITEM(IID_IUnknown, ISimpleAdder)
            COM_INTERFACE_ITEM(IID_ISimpleAdder, ISimpleAdder)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServiceReplica, IFabricStatefulServiceReplica)
            COM_INTERFACE_ITEM(IID_IFabricStateProvider, IFabricStateProvider)
        END_COM_INTERFACE_LIST()

    public:

        StatefulServiceImplementation(wstring const & partnerUri);
        ~StatefulServiceImplementation();

        //
        // IFabricStatefulServiceReplica members
        //

        STDMETHODIMP BeginOpen( 
            __in FABRIC_REPLICA_OPEN_MODE openMode,
            __in IFabricStatefulServicePartition *partition,
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        STDMETHODIMP EndOpen( 
            __in IFabricAsyncOperationContext *context,
            __out IFabricReplicator **replicator);
        
        STDMETHODIMP BeginChangeRole( 
            __in FABRIC_REPLICA_ROLE newRole,
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        STDMETHODIMP EndChangeRole( 
            __in IFabricAsyncOperationContext *context,
            __out IFabricStringResult **serviceAddress);
        
        STDMETHODIMP BeginClose( 
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        STDMETHODIMP EndClose( 
            __in IFabricAsyncOperationContext *context);
        
        STDMETHODIMP_(void) Abort( void);

        //
        // IFabricStateProvider members
        //

        STDMETHODIMP BeginUpdateEpoch( 
            __in const FABRIC_EPOCH *epoch,
            __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        STDMETHODIMP EndUpdateEpoch( 
            __in IFabricAsyncOperationContext *context);
        
        STDMETHODIMP GetLastCommittedSequenceNumber( 
            __out FABRIC_SEQUENCE_NUMBER *sequenceNumber);
        
        STDMETHODIMP BeginOnDataLoss( 
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        STDMETHODIMP EndOnDataLoss( 
            __in IFabricAsyncOperationContext *context,
            __out BOOLEAN *isStateChanged);
        
        STDMETHODIMP GetCopyContext( 
            __out IFabricOperationDataStream **copyContextStream);
        
        STDMETHODIMP GetCopyState( 
            __in FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
            __in IFabricOperationDataStream *copyContextStream,
            __out IFabricOperationDataStream **copyStateStream);

        //
        // ISimpleAdder members
        //
        STDMETHODIMP Add( 
            __in LONG first,
            __in LONG second,
            __out LONG *result);

    private:

        wstring partnerUri_;
        ComPointer<IFabricServiceGroupPartition> serviceGroupPartition_;
        ComPointer<IFabricReplicator> replicator_;
        ComPointer<IFabricStateReplicator> stateReplicator_;

        Common::ExclusiveLock lockDrainer_;
        DrainOperationStreams* drainer_;

        template <class ComImplementation, class T0>
        friend Common::ComPointer<ComImplementation> Common::make_com(T0 && a0);
    };
}
