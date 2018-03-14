// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Store
{
    StringLiteral const TraceComponent("ComCopyContextEnumerator");

    class ComCopyContextEnumerator::ComCopyContextContext : public Common::ComAsyncOperationContext
    {
        DENY_COPY(ComCopyContextContext);

    public:
        ComCopyContextContext(__in ComCopyContextEnumerator & owner) 
            : owner_(owner)
        {
            WriteNoise(
                TraceComponent, 
                "{0}: {1} ComCopyContextContext::ctor",
                owner_.TraceId,
                static_cast<void*>(this));
        }

        ~ComCopyContextContext() 
        {
            WriteNoise(
                TraceComponent, 
                "{0}: {1} ComCopyContextContext::~dtor",
                owner_.TraceId,
                static_cast<void*>(this));
        }

        HRESULT STDMETHODCALLTYPE End(__out Common::ComPointer<ComReplicatedOperationData> & operation);

    protected:
        void OnStart(AsyncOperationSPtr const &);

    private:
        ComCopyContextEnumerator & owner_;
        Common::ComPointer<ComReplicatedOperationData> replicatedOperationData_;
    };
    
    // ************************
    // ComCopyContextEnumerator
    // ************************

    ComCopyContextEnumerator::ComCopyContextEnumerator(
        Store::PartitionedReplicaId const & partitionedReplicaId,
        bool isEpochValid,
        ::FABRIC_EPOCH const & currentEpoch,
        ::FABRIC_SEQUENCE_NUMBER lastOperationLSN,
        Common::ComponentRoot const & root) 
        : ReplicaActivityTraceComponent(partitionedReplicaId, Common::ActivityId())
        , isEpochValid_(isEpochValid)
        , currentEpoch_(currentEpoch)
        , lastOperationLSN_(lastOperationLSN)
        , root_(root.shared_from_this())
        , isContextSent_(false)
    { 
        WriteNoise(
            TraceComponent, 
            "{0}: {1} ComCopyContextEnumerator::ctor: valid={2}; epoch={3}.{4:X}; lastSequence={5}",
            this->TraceId,
            static_cast<void*>(this),
            isEpochValid,
            currentEpoch.DataLossNumber,
            currentEpoch.ConfigurationNumber,
            lastOperationLSN);
    }

    ComCopyContextEnumerator::~ComCopyContextEnumerator()
    { 
        WriteNoise(
            TraceComponent, 
            "{0}: {1} ComCopyContextEnumerator::~dtor",
            this->TraceId,
            static_cast<void*>(this));
    }

    HRESULT STDMETHODCALLTYPE ComCopyContextEnumerator::BeginGetNext(
        __in IFabricAsyncOperationCallback * callback, 
        __out IFabricAsyncOperationContext ** operationContext)
    {
        if (callback == NULL || operationContext == NULL)
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        ComAsyncOperationContextCPtr copyContextContext = make_com<ComCopyContextContext,ComAsyncOperationContext>(*this);

        HRESULT hr = copyContextContext->Initialize(root_->shared_from_this(), callback);
        if (!SUCCEEDED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        hr = copyContextContext->Start(copyContextContext);
        if (!SUCCEEDED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        hr = copyContextContext->QueryInterface(
            IID_IFabricAsyncOperationContext,
            reinterpret_cast<void**>(operationContext));
        return ComUtility::OnPublicApiReturn(hr);
    }

    HRESULT STDMETHODCALLTYPE ComCopyContextEnumerator::EndGetNext(
        __in IFabricAsyncOperationContext * operationContext, 
        __out IFabricOperationData ** result)
    {
        if (operationContext == NULL || result == NULL)
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        ComPointer<ComReplicatedOperationData> replicatedOperationData;
        ComPointer<ComCopyContextContext> typedContext(operationContext, IID_IFabricAsyncOperationContext);

        HRESULT hr = typedContext->End(replicatedOperationData);
        if (!SUCCEEDED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        if (replicatedOperationData->IsEmpty)
        {
            *result = NULL;
            return ComUtility::OnPublicApiReturn(S_OK);
        }
        else
        {
            hr = replicatedOperationData->QueryInterface(IID_IFabricOperationData, reinterpret_cast<void**>(result));
            return ComUtility::OnPublicApiReturn(hr);
        }
    }

    // *********************
    // ComCopyContextContext
    // *********************

    void ComCopyContextEnumerator::ComCopyContextContext::OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        WriteNoise(
            TraceComponent, 
            "{0}: get copy context: valid = {1} epoch = {2}.{3:X} last sequence = {4} isSent = {5}",
            owner_.TraceId,
            owner_.isEpochValid_,
            owner_.currentEpoch_.DataLossNumber,
            owner_.currentEpoch_.ConfigurationNumber,
            owner_.lastOperationLSN_,
            owner_.isContextSent_);

        Serialization::IFabricSerializableStreamUPtr serializedStreamUPtr;

        ErrorCode error(ErrorCodeValue::Success);

        if (!owner_.isContextSent_)
        {
            owner_.isContextSent_ = true;

            CopyContextData copyContextData(
                owner_.TraceId, 
                owner_.isEpochValid_, 
                owner_.currentEpoch_, 
                owner_.lastOperationLSN_,
                owner_.ReplicaId);

            error = FabricSerializer::Serialize(&copyContextData, serializedStreamUPtr);
        }

        if (error.IsSuccess())
        {
            replicatedOperationData_ = make_com<ComReplicatedOperationData>(Ktl::Move(serializedStreamUPtr));
        }

        TryComplete(proxySPtr, error.ToHResult());
    }

    HRESULT STDMETHODCALLTYPE ComCopyContextEnumerator::ComCopyContextContext::End(__out ComPointer<ComReplicatedOperationData> & result)
    {
        HRESULT hr = ComAsyncOperationContext::End();
        if (!SUCCEEDED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        result.Swap(replicatedOperationData_);
        return ComUtility::OnPublicApiReturn(S_OK);
    }
}
