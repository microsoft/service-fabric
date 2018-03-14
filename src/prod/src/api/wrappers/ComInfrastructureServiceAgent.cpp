// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace ServiceModel;

// ********************************************************************************************************************
// ComAsyncOperationContext Classes
//

class ComInfrastructureServiceAgent::ComStartInfrastructureTaskAsyncOperation : public ComAsyncOperationContext
{
    DENY_COPY(ComStartInfrastructureTaskAsyncOperation);
   
public:
    ComStartInfrastructureTaskAsyncOperation(
        __in IInfrastructureServiceAgent & impl,
        FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION * taskDescription)
        : ComAsyncOperationContext()
        , impl_(impl)
        , taskDescription_(taskDescription)
        , timeout_(TimeSpan::Zero)
    {
    }

    virtual ~ComStartInfrastructureTaskAsyncOperation() { }

    HRESULT Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in DWORD timeoutMilliseconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);

        if (SUCCEEDED(hr))
        {
            timeout_ = TimeSpan::FromMilliseconds(timeoutMilliseconds);
        }

        return hr;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }
        ComPointer<ComAsyncOperationContext> thisCPtr(context, IID_IFabricAsyncOperationContext);
        return thisCPtr->End();
    } 

protected:
    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = impl_.BeginStartInfrastructureTask(
            taskDescription_,
            timeout_,
            [this](AsyncOperationSPtr const & operation) { this->OnStartInfrastructureTaskComplete(operation, false); },
            proxySPtr);
        this->OnStartInfrastructureTaskComplete(operation, true);
    }

private:
    void OnStartInfrastructureTaskComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        ErrorCode error = impl_.EndStartInfrastructureTask(operation);

        this->TryComplete(operation->Parent, error.ToHResult());
    }

    IInfrastructureServiceAgent & impl_;
    FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION * taskDescription_;
    TimeSpan timeout_;
};

class ComInfrastructureServiceAgent::ComFinishInfrastructureTaskAsyncOperation : public ComAsyncOperationContext
{
    DENY_COPY(ComFinishInfrastructureTaskAsyncOperation);
   
public:
    ComFinishInfrastructureTaskAsyncOperation(
        __in IInfrastructureServiceAgent & impl,
        wstring const & taskId,
        uint64 instanceId)
        : ComAsyncOperationContext()
        , impl_(impl)
        , taskId_(taskId)
        , instanceId_(instanceId)
        , timeout_(TimeSpan::Zero)
    {
    }

    virtual ~ComFinishInfrastructureTaskAsyncOperation() { }

    HRESULT Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in DWORD timeoutMilliseconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);

        if (SUCCEEDED(hr))
        {
            timeout_ = TimeSpan::FromMilliseconds(timeoutMilliseconds);
        }

        return hr;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }
        ComPointer<ComAsyncOperationContext> thisCPtr(context, IID_IFabricAsyncOperationContext);
        return thisCPtr->End();
    } 

protected:
    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = impl_.BeginFinishInfrastructureTask(
            taskId_,
            instanceId_,
            timeout_,
            [this](AsyncOperationSPtr const & operation) { this->OnFinishInfrastructureTaskComplete(operation, false); },
            proxySPtr);
        this->OnFinishInfrastructureTaskComplete(operation, true);
    }

private:
    void OnFinishInfrastructureTaskComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        ErrorCode error = impl_.EndFinishInfrastructureTask(operation);

        this->TryComplete(operation->Parent, error.ToHResult());
    }

    IInfrastructureServiceAgent & impl_;
    wstring taskId_;
    uint64 instanceId_;
    TimeSpan timeout_;
};

// {34a16309-07f5-49c6-bac9-626d181a2e52}
static const GUID CLSID_ComQueryInfrastructureTaskAsyncOperation = 
{ 0x34a16309, 0x07f5, 0x49c6, { 0xba, 0xc9, 0x62, 0x6d, 0x18, 0x1a, 0x2e, 0x52 } };

class ComInfrastructureServiceAgent::ComQueryInfrastructureTaskAsyncOperation : public ComAsyncOperationContext
{
    DENY_COPY(ComQueryInfrastructureTaskAsyncOperation);
   
    COM_INTERFACE_AND_DELEGATE_LIST(
        ComQueryInfrastructureTaskAsyncOperation,
        CLSID_ComQueryInfrastructureTaskAsyncOperation,
        ComQueryInfrastructureTaskAsyncOperation,
        ComAsyncOperationContext)

public:
    ComQueryInfrastructureTaskAsyncOperation(
        __in IInfrastructureServiceAgent & impl)
        : ComAsyncOperationContext()
        , impl_(impl)
        , timeout_(TimeSpan::Zero)
    {
    }

    virtual ~ComQueryInfrastructureTaskAsyncOperation() { }

    HRESULT Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in DWORD timeoutMilliseconds,
        __in IFabricAsyncOperationCallback * callback)
    {
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);

        if (SUCCEEDED(hr))
        {
            timeout_ = TimeSpan::FromMilliseconds(timeoutMilliseconds);
        }

        return hr;
    }

    static HRESULT End(__in IFabricAsyncOperationContext * context, __out IFabricInfrastructureTaskQueryResult ** result)
    {
        if ((context == NULL) || (result == NULL))
        {
            return E_POINTER;
        }

        ComPointer<ComQueryInfrastructureTaskAsyncOperation> thisCPtr(context, CLSID_ComQueryInfrastructureTaskAsyncOperation);
        
        auto hr = thisCPtr->Result;

        if (SUCCEEDED(hr))
        {
            vector<InfrastructureTaskQueryResult> resultList;
            auto error = thisCPtr->nativeResult_.MoveList(resultList);

            if (error.IsSuccess())
            {
                ComPointer<IFabricInfrastructureTaskQueryResult> cPtr = make_com<ComQueryResult, IFabricInfrastructureTaskQueryResult>(move(resultList));
                hr = cPtr->QueryInterface(IID_IFabricInfrastructureTaskQueryResult, reinterpret_cast<void**>(result));
            }
            else
            {
                hr = error.ToHResult();
            }
        }

        return hr;
    } 

protected:
    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        auto operation = impl_.BeginQueryInfrastructureTask(
            timeout_,
            [this](AsyncOperationSPtr const & operation) { this->OnQueryInfrastructureTaskComplete(operation, false); },
            proxySPtr);
        this->OnQueryInfrastructureTaskComplete(operation, true);
    }

private:
    void OnQueryInfrastructureTaskComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto error = impl_.EndQueryInfrastructureTask(operation, nativeResult_);

        this->TryComplete(operation->Parent, error.ToHResult());
    }

    IInfrastructureServiceAgent & impl_;
    QueryResult nativeResult_;
    TimeSpan timeout_;
};

class ComInfrastructureServiceAgent::ComQueryResult
    : public IFabricInfrastructureTaskQueryResult
    , private Common::ComUnknownBase
{
    DENY_COPY_ASSIGNMENT(ComQueryResult)

    BEGIN_COM_INTERFACE_LIST(ComQueryResult)  
        COM_INTERFACE_ITEM(IID_IUnknown,IFabricInfrastructureTaskQueryResult)                         
        COM_INTERFACE_ITEM(IID_IFabricInfrastructureTaskQueryResult,IFabricInfrastructureTaskQueryResult)
    END_COM_INTERFACE_LIST()

public:
    explicit ComQueryResult(vector<InfrastructureTaskQueryResult> && resultList)
    {
        queryResult_ = heap_.AddItem<FABRIC_INFRASTRUCTURE_TASK_QUERY_RESULT_LIST>();
        auto array = heap_.AddArray<FABRIC_INFRASTRUCTURE_TASK_QUERY_RESULT_ITEM>(resultList.size());

        queryResult_->Count = static_cast<ULONG>(resultList.size());
        queryResult_->Items = array.GetRawArray();

        for (size_t ix = 0; ix < resultList.size(); ++ix)
        {
            FABRIC_INFRASTRUCTURE_TASK_QUERY_RESULT_ITEM * arrayItem = &array[ix];
            InfrastructureTaskQueryResult const & listItem = resultList[ix];

            arrayItem->Description = heap_.AddItem<FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION>().GetRawPointer();
            listItem.Description.ToPublicApi(heap_, *arrayItem->Description);

            arrayItem->State = Management::ClusterManager::InfrastructureTaskState::ToPublicApi(listItem.State);
        }
    }

    const FABRIC_INFRASTRUCTURE_TASK_QUERY_RESULT_LIST * STDMETHODCALLTYPE get_Result(void)
    {
        return queryResult_.GetRawPointer();
    }

private:
    Common::ScopedHeap heap_;
    Common::ReferencePointer<FABRIC_INFRASTRUCTURE_TASK_QUERY_RESULT_LIST> queryResult_;
};

// ********************************************************************************************************************
// ComInfrastructureServiceAgent::ComInfrastructureServiceAgent Implementation
//

ComInfrastructureServiceAgent::ComInfrastructureServiceAgent(IInfrastructureServiceAgentPtr const & impl)
    : IFabricInfrastructureServiceAgent()
    , ComUnknownBase()
    , impl_(impl)
{
}

ComInfrastructureServiceAgent::~ComInfrastructureServiceAgent()
{
    // Break lifetime cycle between ISAgent and ServiceRoutingAgentProxy
    impl_->Release();
}

HRESULT STDMETHODCALLTYPE ComInfrastructureServiceAgent::RegisterInfrastructureServiceFactory(
    IFabricStatefulServiceFactory * factory)
{
    return ComUtility::OnPublicApiReturn(impl_->RegisterInfrastructureServiceFactory(factory).ToHResult());
}

HRESULT STDMETHODCALLTYPE ComInfrastructureServiceAgent::RegisterInfrastructureService(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId,
    IFabricInfrastructureService * comInterface,
    IFabricStringResult ** serviceAddress)
{
    if (serviceAddress == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    wstring serviceAddressResult;

    impl_->RegisterInfrastructureService(
        partitionId, 
        replicaId, 
        WrapperFactory::create_rooted_com_proxy(comInterface),
        serviceAddressResult);

    auto result = make_com<ComStringResult, IFabricStringResult>(serviceAddressResult);
    *serviceAddress = result.DetachNoRelease();

    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT STDMETHODCALLTYPE ComInfrastructureServiceAgent::UnregisterInfrastructureService(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId)
{
    impl_->UnregisterInfrastructureService(partitionId, replicaId);

    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT STDMETHODCALLTYPE ComInfrastructureServiceAgent::BeginStartInfrastructureTask(
    /* [in] */ FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION * taskDescription,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    auto operation = make_com<ComStartInfrastructureTaskAsyncOperation>(*impl_.get(), taskDescription);

    HRESULT hr = operation->Initialize(
        impl_.get_Root(),
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComInfrastructureServiceAgent::EndStartInfrastructureTask(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(ComStartInfrastructureTaskAsyncOperation::End(context));
}

HRESULT STDMETHODCALLTYPE ComInfrastructureServiceAgent::BeginFinishInfrastructureTask(
    /* [in] */ LPCWSTR taskId,
    /* [in] */ ULONGLONG instanceId,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    wstring parsedTaskId;
    HRESULT hr = StringUtility::LpcwstrToWstring(taskId, false, parsedTaskId);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto operation = make_com<ComFinishInfrastructureTaskAsyncOperation>(*impl_.get(), parsedTaskId, instanceId);

    hr = operation->Initialize(
        impl_.get_Root(),
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComInfrastructureServiceAgent::EndFinishInfrastructureTask(
    /* [in] */ IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(ComFinishInfrastructureTaskAsyncOperation::End(context));
}

HRESULT STDMETHODCALLTYPE ComInfrastructureServiceAgent::BeginQueryInfrastructureTask(
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    auto operation = make_com<ComQueryInfrastructureTaskAsyncOperation>(*impl_.get());

    auto hr = operation->Initialize(
        impl_.get_Root(),
        timeoutMilliseconds,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT STDMETHODCALLTYPE ComInfrastructureServiceAgent::EndQueryInfrastructureTask(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricInfrastructureTaskQueryResult ** result)
{
    return ComUtility::OnPublicApiReturn(ComQueryInfrastructureTaskAsyncOperation::End(context, result));
}
