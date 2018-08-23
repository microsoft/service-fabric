// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

StringLiteral const TraceComponent("ComStatefulServiceReplica");

// ********************************************************************************************************************
// ComStatefulServiceReplica::OpenOperationContext Implementation
//

// {3D4A1434-AB78-4BC0-93AE-1979D6F5652F}
static const GUID CLSID_ComStatefulServiceReplica_OpenOperationContext = 
{ 0x3d4a1434, 0xab78, 0x4bc0, { 0x93, 0xae, 0x19, 0x79, 0xd6, 0xf5, 0x65, 0x2f } };

class ComStatefulServiceReplica::OpenOperationContext :
    public ComAsyncOperationContext
{
    DENY_COPY(OpenOperationContext);

    COM_INTERFACE_AND_DELEGATE_LIST(
        OpenOperationContext, 
        CLSID_ComStatefulServiceReplica_OpenOperationContext,
        OpenOperationContext,
        ComAsyncOperationContext)

    public:
        OpenOperationContext(__in ComStatefulServiceReplica & owner) 
            : ComAsyncOperationContext(),
            owner_(owner),
            openMode_(),
            partition_()
        {
        }

        virtual ~OpenOperationContext()
        {
        }

        HRESULT Initialize(
            __in ComponentRootSPtr const & rootSPtr,
            __in FABRIC_REPLICA_OPEN_MODE openMode,
            __in IFabricStatefulServicePartition *partition,
            __in IFabricAsyncOperationCallback * callback)
        {
            HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
            if (FAILED(hr)) { return hr; }

            if (partition == NULL) { return E_POINTER; }

            openMode_ = openMode;
            partition_.SetAndAddRef(partition);

            return S_OK;
        }

        static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricReplicator **replicator)
        {
            if (context == NULL || replicator == NULL) { return E_POINTER; }

            ComPointer<OpenOperationContext> thisOperation(context, CLSID_ComStatefulServiceReplica_OpenOperationContext);
            auto hr = thisOperation->Result;
            if (SUCCEEDED(hr))
            {
                ASSERT_IF(thisOperation->replicator_.GetRawPointer() == NULL, "The replicator must be non null.");
                hr = thisOperation->replicator_->QueryInterface(IID_IFabricReplicator, reinterpret_cast<void**>(replicator));
            }

            return hr;
        }

    protected:
        virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
        {
            auto operation = owner_.impl_->BeginOpen(
                openMode_,
                partition_,
                [this](AsyncOperationSPtr const & operation){ this->FinishOpen(operation, false); },
                proxySPtr);

            this->FinishOpen(operation, true);
        }

    private:
        void FinishOpen(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = owner_.impl_->EndOpen(operation, replicator_);
            TryComplete(operation->Parent, error.ToHResult());
        }

    private:
        ComStatefulServiceReplica & owner_;
        FABRIC_REPLICA_OPEN_MODE openMode_;
        ComPointer<IFabricStatefulServicePartition> partition_;
        ComPointer<IFabricReplicator> replicator_;
};


// ********************************************************************************************************************
// ComStatefulServiceReplica::ChangeRoleOperationContext Implementation
//

// {70C1A224-6DF4-4FF2-AE12-EC0B79B3D940}
static const GUID CLSID_ComStatefulServiceReplica_ChangeRoleOperationContext = 
{ 0x70c1a224, 0x6df4, 0x4ff2, { 0xae, 0x12, 0xec, 0xb, 0x79, 0xb3, 0xd9, 0x40 } };

class ComStatefulServiceReplica::ChangeRoleOperationContext :
    public ComAsyncOperationContext
{
    DENY_COPY(ChangeRoleOperationContext);

    COM_INTERFACE_AND_DELEGATE_LIST(
        ChangeRoleOperationContext, 
        CLSID_ComStatefulServiceReplica_ChangeRoleOperationContext,
        ChangeRoleOperationContext,
        ComAsyncOperationContext)

    public:
        ChangeRoleOperationContext(__in ComStatefulServiceReplica & owner) 
            : ComAsyncOperationContext(),
            owner_(owner),
            newRole_(),
            serviceAddress_()
        {
        }

        virtual ~ChangeRoleOperationContext()
        {
        }

        HRESULT Initialize(
            __in ComponentRootSPtr const & rootSPtr,
            __in FABRIC_REPLICA_ROLE newRole,
            __in IFabricAsyncOperationCallback * callback)
        {
            HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
            if (FAILED(hr)) { return hr; }

            newRole_ = newRole;
            return S_OK;
        }

        static HRESULT End(__in IFabricAsyncOperationContext * context, IFabricStringResult **serviceAddress)
        {
            if (context == NULL || serviceAddress == NULL) { return E_POINTER; }

            ComPointer<ChangeRoleOperationContext> thisOperation(context, CLSID_ComStatefulServiceReplica_ChangeRoleOperationContext);
            auto hr = thisOperation->Result;
            
            if (SUCCEEDED(hr))
            {
                ComPointer<IFabricStringResult> serviceAddressResult = make_com<ComStringResult, IFabricStringResult>(thisOperation->serviceAddress_);
                hr = serviceAddressResult->QueryInterface(IID_IFabricStringResult, reinterpret_cast<void**>(serviceAddress));
            }

            return hr;
        }

    protected:
        virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
        {
            auto operation = owner_.impl_->BeginChangeRole(
                newRole_,
                [this](AsyncOperationSPtr const & operation){ this->FinishOpen(operation, false); },
                proxySPtr);

            this->FinishOpen(operation, true);
        }

    private:
        void FinishOpen(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = owner_.impl_->EndChangeRole(operation, serviceAddress_);
            TryComplete(operation->Parent, error.ToHResult());
        }

    private:
        ComStatefulServiceReplica & owner_;
        FABRIC_REPLICA_ROLE newRole_;
        wstring serviceAddress_;
};

// ********************************************************************************************************************
// ComStatefulServiceReplica::CloseOperationContext Implementation
//

// {229D2F95-19DE-4AB0-BDD5-2D9267960872}
static const GUID CLSID_ComStatefulServiceReplica_CloseOperationContext = 
{ 0x229d2f95, 0x19de, 0x4ab0, { 0xbd, 0xd5, 0x2d, 0x92, 0x67, 0x96, 0x8, 0x72 } };

class ComStatefulServiceReplica::CloseOperationContext :
    public ComAsyncOperationContext
{
    DENY_COPY(CloseOperationContext);

    COM_INTERFACE_AND_DELEGATE_LIST(
        CloseOperationContext, 
        CLSID_ComStatefulServiceReplica_CloseOperationContext,
        CloseOperationContext,
        ComAsyncOperationContext)

    public:
        CloseOperationContext(__in ComStatefulServiceReplica & owner) 
            : ComAsyncOperationContext(),
            owner_(owner)
        {
        }

        virtual ~CloseOperationContext()
        {
        }

        HRESULT Initialize(
            __in ComponentRootSPtr const & rootSPtr,
            __in IFabricAsyncOperationCallback * callback)
        {
            HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
            if (FAILED(hr)) { return hr; }

            return S_OK;
        }

        static HRESULT End(__in IFabricAsyncOperationContext * context)
        {
            if (context == NULL) { return E_POINTER; }

            ComPointer<CloseOperationContext> thisOperation(context, CLSID_ComStatefulServiceReplica_CloseOperationContext);
            return thisOperation->Result;
        }

    protected:
        virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
        {
            auto operation = owner_.impl_->BeginClose(
                [this](AsyncOperationSPtr const & operation){ this->FinishOpen(operation, false); },
                proxySPtr);

            this->FinishOpen(operation, true);
        }

    private:
        void FinishOpen(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = owner_.impl_->EndClose(operation);
            TryComplete(operation->Parent, error.ToHResult());
        }

    private:
        ComStatefulServiceReplica & owner_;
};


// ********************************************************************************************************************
// ComStatefulServiceReplica Implementation
//

ComStatefulServiceReplica::ComStatefulServiceReplica(IStatefulServiceReplicaPtr const & impl)
    : IFabricStatefulServiceReplica(),
    ComUnknownBase(),
    impl_(impl)
{
}

ComStatefulServiceReplica::~ComStatefulServiceReplica()
{
}

HRESULT ComStatefulServiceReplica::BeginOpen( 
    /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
    /* [in] */ IFabricStatefulServicePartition *partition,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<OpenOperationContext> operation 
        = make_com<OpenOperationContext>(*this);
    hr = operation->Initialize(
        root,
        openMode,
        partition,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));

}

HRESULT ComStatefulServiceReplica::EndOpen( 
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricReplicator **replicator)
{
    return ComUtility::OnPublicApiReturn(OpenOperationContext::End(context, replicator));
}

HRESULT ComStatefulServiceReplica::BeginChangeRole( 
    /* [in] */ FABRIC_REPLICA_ROLE newRole,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<ChangeRoleOperationContext> operation 
        = make_com<ChangeRoleOperationContext>(*this);
    hr = operation->Initialize(
        root,
        newRole,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComStatefulServiceReplica::EndChangeRole( 
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **serviceAddress)
{
    return ComUtility::OnPublicApiReturn(ChangeRoleOperationContext::End(context, serviceAddress));
}

HRESULT ComStatefulServiceReplica::BeginClose( 
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<CloseOperationContext> operation 
        = make_com<CloseOperationContext>(*this);
    hr = operation->Initialize(
        root,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComStatefulServiceReplica::EndClose( 
    /* [in] */ IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(CloseOperationContext::End(context));
}

void ComStatefulServiceReplica::Abort(void)
{
    impl_->Abort();
}

HRESULT ComStatefulServiceReplica::GetStatus(
    /* [out, retval] */ IFabricStatefulServiceReplicaStatusResult ** result)
{
    if (result == nullptr) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    IStatefulServiceReplicaStatusResultPtr resultImpl;
    auto error = impl_->GetQueryStatus(resultImpl);

    if (!error.IsSuccess()) 
    { 
        Trace.WriteInfo(TraceComponent, "GetQueryStatus failed: {0}", error);

        return ComUtility::OnPublicApiReturn(error.ToHResult()); 
    }

    TESTASSERT_IF(resultImpl.get() == nullptr, "Result cannot be null.");
    auto resultCPtr = WrapperFactory::create_com_wrapper(resultImpl);
    *result = resultCPtr.DetachNoRelease();

    return S_OK;
}

HRESULT ComStatefulServiceReplica::UpdateInitializationData(
    /* [in] */ ULONG bufferSize,
    /* [in] */ const BYTE * buffer)
{
    vector<byte> bytes;
    if (buffer != nullptr && bufferSize > 0)
    {
        bytes.reserve(bufferSize);
        bytes.insert(bytes.begin(), buffer, buffer + bufferSize);
    }

    return impl_->UpdateInitializationData(move(bytes)).ToHResult();
}
