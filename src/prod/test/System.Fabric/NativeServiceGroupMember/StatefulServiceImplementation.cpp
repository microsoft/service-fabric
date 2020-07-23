// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "StatefulServiceImplementation.h" 
#include "Util.h"

using namespace NativeServiceGroupMember;

StatefulServiceImplementation::StatefulServiceImplementation(wstring const & partnerUri) :
    partnerUri_(partnerUri),
    drainer_(nullptr)
{
}

StatefulServiceImplementation::~StatefulServiceImplementation()
{
}

STDMETHODIMP StatefulServiceImplementation::BeginOpen( 
    __in FABRIC_REPLICA_OPEN_MODE openMode,
    __in IFabricStatefulServicePartition *partition,
    __in IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context) 
{
    UNREFERENCED_PARAMETER(openMode);

    if (partition == nullptr || callback == nullptr || context == nullptr)
    {
        return E_POINTER;
    }

    try
    {
        auto hr = partition->QueryInterface(IID_IFabricServiceGroupPartition, serviceGroupPartition_.VoidInitializationAddress());
        if (FAILED(hr)) { return hr; }

        hr = partition->CreateReplicator(this, NULL, replicator_.InitializationAddress(), stateReplicator_.InitializationAddress());
        if (FAILED(hr)) { return hr; }

        ComPointer<IFabricAsyncOperationContext> dummy = make_com<DummyAsyncContext, IFabricAsyncOperationContext>();

        hr = dummy->QueryInterface(context);
        if (FAILED(hr)) { return hr; }

        callback->Invoke(dummy.GetRawPointer());
        return hr;
    }
    catch (bad_alloc const &) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}

STDMETHODIMP StatefulServiceImplementation::EndOpen( 
    __in IFabricAsyncOperationContext *context,
    __out IFabricReplicator **replicator) 
{
    if (context == nullptr || replicator == NULL)
    {
        return E_POINTER;
    }

    try
    {
        return replicator_->QueryInterface(replicator);
    }
    catch (bad_alloc const &) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}

STDMETHODIMP StatefulServiceImplementation::BeginChangeRole( 
    __in FABRIC_REPLICA_ROLE newRole,
    __in IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context)
{
    if (callback == nullptr || context == nullptr)
    {
        return E_POINTER;
    }

    DrainOperationStreams* drain = NULL;

    {
        Common::AcquireExclusiveLock lock(lockDrainer_);

        if (NULL != drainer_)
        {
            //
            // if new role is not secondary, stop wait for streams to be drained
            //
            if ((FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY != newRole) &&
                (FABRIC_REPLICA_ROLE_IDLE_SECONDARY != newRole))
            {
                drain = drainer_;
                drainer_ = NULL;
            }
        }
        else
        {
            //
            // If new role is secondary, start draining streams
            //
            if ((FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY == newRole) ||
                (FABRIC_REPLICA_ROLE_IDLE_SECONDARY == newRole))
            {
                drainer_ = new (std::nothrow) DrainOperationStreams(stateReplicator_.GetRawPointer());
                if (NULL == drainer_)
                {
                    return E_OUTOFMEMORY;
                }

                drainer_->AddRef();

                this->AddRef();
                Common::Threadpool::Post(
                    [this]() -> void
                    {
                        DrainOperationStreams* drainer = NULL;

                        {
                            Common::AcquireExclusiveLock lock(this->lockDrainer_);
                            drainer = drainer_;
                            if (NULL != drainer)
                            {
                                drainer->AddRef();
                            }
                        }

                        if (NULL != drainer)
                        {
                            drainer->Start();
                            drainer->Release();
                        }
                        this->Release();
                    });
            }
        }
    }

    if (NULL != drain)
    {
        drain->Drain();
        drain->Release();
        drain = NULL;
    }

    try
    {
        ComPointer<IFabricAsyncOperationContext> dummy = make_com<DummyAsyncContext, IFabricAsyncOperationContext>();

        HRESULT hr = dummy->QueryInterface(context);
        if (FAILED(hr)) { return hr; }

        callback->Invoke(dummy.GetRawPointer());
        return hr;
    }
    catch (bad_alloc const &) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}

STDMETHODIMP StatefulServiceImplementation::EndChangeRole( 
    __in IFabricAsyncOperationContext *context,
    __out IFabricStringResult **serviceAddress) 
{
    if (context == nullptr)
    {
        return E_POINTER;
    }

    try
    {
        ComPointer<IFabricStringResult> address = make_com<StringResult, IFabricStringResult>();
        return address->QueryInterface(serviceAddress);
    }
    catch (bad_alloc const &) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}

STDMETHODIMP StatefulServiceImplementation::BeginClose( 
    __in IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context) 
{
    if (callback == nullptr || context == nullptr)
    {
        return E_POINTER;
    }

    try
    {
        ComPointer<IFabricAsyncOperationContext> dummy = make_com<DummyAsyncContext, IFabricAsyncOperationContext>();

        HRESULT hr = dummy->QueryInterface(context);
        if (FAILED(hr)) { return hr; }

        callback->Invoke(dummy.GetRawPointer());
        return hr;
    }
    catch (bad_alloc const &) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}

STDMETHODIMP StatefulServiceImplementation::EndClose( 
    __in IFabricAsyncOperationContext *context) 
{
    if (context == nullptr)
    {
        return E_POINTER;
    }

    Abort();

    return S_OK;
}

STDMETHODIMP_(void) StatefulServiceImplementation::Abort(void)
{
}

//
// IFabricStateProvider members
//

STDMETHODIMP StatefulServiceImplementation::BeginUpdateEpoch( 
    __in const FABRIC_EPOCH *epoch,
    __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
    __in IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context)
{
    UNREFERENCED_PARAMETER(epoch);
    UNREFERENCED_PARAMETER(previousEpochLastSequenceNumber);

    if (callback == nullptr || context == nullptr)
    {
        return E_POINTER;
    }

    try
    {
        ComPointer<IFabricAsyncOperationContext> dummy = make_com<DummyAsyncContext, IFabricAsyncOperationContext>();

        HRESULT hr = dummy->QueryInterface(context);
        if (FAILED(hr)) { return hr; }

        callback->Invoke(dummy.GetRawPointer());
        return hr;
    }
    catch (bad_alloc const &) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}

STDMETHODIMP StatefulServiceImplementation::EndUpdateEpoch( 
    __in IFabricAsyncOperationContext *context)
{
    if (context == nullptr)
    {
        return E_POINTER;
    }

    return S_OK;
}

STDMETHODIMP StatefulServiceImplementation::GetLastCommittedSequenceNumber( 
    __out FABRIC_SEQUENCE_NUMBER *sequenceNumber)
{
    if (nullptr == sequenceNumber)
    {
        return E_POINTER;
    }

    *sequenceNumber = 0;
    return S_OK;
}

STDMETHODIMP StatefulServiceImplementation::BeginOnDataLoss( 
    __in IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context)
{
    if (callback == nullptr || context == nullptr)
    {
        return E_POINTER;
    }

    try
    {
        ComPointer<IFabricAsyncOperationContext> dummy = make_com<DummyAsyncContext, IFabricAsyncOperationContext>();

        HRESULT hr = dummy->QueryInterface(context);
        if (FAILED(hr)) { return hr; }

        callback->Invoke(dummy.GetRawPointer());
        return hr;
    }
    catch (bad_alloc const &) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}

STDMETHODIMP StatefulServiceImplementation::EndOnDataLoss( 
    __in IFabricAsyncOperationContext *context,
    __out BOOLEAN *isStateChanged)
{
    if (nullptr == context || nullptr == isStateChanged)
    {
        return E_POINTER;
    }

    *isStateChanged = FALSE;

    return S_OK;
}

STDMETHODIMP StatefulServiceImplementation::GetCopyContext( 
    __out IFabricOperationDataStream **copyContextStream)
{
    if (copyContextStream == nullptr)
    {
        return E_POINTER;
    }

    try
    {
        ComPointer<IFabricOperationDataStream> stream = make_com<NullOperationDataStream, IFabricOperationDataStream>();
        return stream->QueryInterface(copyContextStream);
    }
    catch (bad_alloc const &) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}

STDMETHODIMP StatefulServiceImplementation::GetCopyState( 
    __in FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
    __in IFabricOperationDataStream *copyContextStream,
    __out IFabricOperationDataStream **copyStateStream)
{
    UNREFERENCED_PARAMETER(uptoSequenceNumber);
    UNREFERENCED_PARAMETER(copyContextStream);

    if (copyStateStream == nullptr)
    {
        return E_POINTER;
    }

    try
    {
        ComPointer<IFabricOperationDataStream> stream = make_com<NullOperationDataStream, IFabricOperationDataStream>();
        return stream->QueryInterface(copyStateStream);
    }
    catch (bad_alloc const &) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}

//
// ISimpleAdder members
//
STDMETHODIMP StatefulServiceImplementation::Add( 
    __in LONG first,
    __in LONG second,
    __out LONG *result)
{
    if (serviceGroupPartition_.GetRawPointer() == nullptr)
    {
        return E_UNEXPECTED;
    }

    if (result == nullptr)
    {
        return E_POINTER;
    }

    try
    {
        //
        // negative resolve, should fail
        //

        ComPointer<ISimpleAdder> dummy;

        wstring empty;
        wstring notUrl(L"test");
        wstring notExisting(L"fabric:/invalidapplication/invalidservice#invalidmember");

        auto hr = serviceGroupPartition_->ResolveMember(empty.c_str(), IID_ISimpleAdder, dummy.VoidInitializationAddress());
        if (SUCCEEDED(hr))
        {
            return E_UNEXPECTED;
        }

        hr = serviceGroupPartition_->ResolveMember(notUrl.c_str(), IID_ISimpleAdder, dummy.VoidInitializationAddress());
        if (SUCCEEDED(hr))
        {
            return E_UNEXPECTED;
        }

        hr = serviceGroupPartition_->ResolveMember(notExisting.c_str(), IID_ISimpleAdder, dummy.VoidInitializationAddress());
        if (SUCCEEDED(hr))
        {
            return E_UNEXPECTED;
        }

        hr = serviceGroupPartition_->ResolveMember(NULL, IID_ISimpleAdder, dummy.VoidInitializationAddress());
        if (SUCCEEDED(hr))
        {
            return E_UNEXPECTED;
        }

        hr = serviceGroupPartition_->ResolveMember(partnerUri_.c_str(), IID_ISimpleAdder, NULL);
        if (SUCCEEDED(hr))
        {
            return E_UNEXPECTED;
        }

        //
        // positive resolve
        //
        ComPointer<ISimpleAdder> other;

        hr = serviceGroupPartition_->ResolveMember(partnerUri_.c_str(), IID_ISimpleAdder, other.VoidInitializationAddress());
        if (FAILED(hr)) { return hr; }

        return other->Add(first, second, result);
    }
    catch (bad_alloc const &) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}
