// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComStateProvider::UpdateEpochOperationContext Implementation
//

// {D80AF29B-0FB0-4C68-9EFB-B541582444F6}
static const GUID CLSID_ComStateProvider_UpdateEpochOperationContext = 
{ 0xd80af29b, 0xfb0, 0x4c68, { 0x9e, 0xfb, 0xb5, 0x41, 0x58, 0x24, 0x44, 0xf6 } };

class ComStateProvider::UpdateEpochOperationContext :
    public ComAsyncOperationContext
{
    DENY_COPY(UpdateEpochOperationContext);

    COM_INTERFACE_AND_DELEGATE_LIST(
        UpdateEpochOperationContext, 
        CLSID_ComStateProvider_UpdateEpochOperationContext,
        UpdateEpochOperationContext,
        ComAsyncOperationContext)

    public:
        UpdateEpochOperationContext(__in ComStateProvider & owner) 
            : ComAsyncOperationContext(),
            owner_(owner),
            epoch_(),
            previousEpochLastSequenceNumber_()
        {
        }

        virtual ~UpdateEpochOperationContext()
        {
        }

        HRESULT Initialize(
            __in ComponentRootSPtr const & rootSPtr,
            __in FABRIC_EPOCH const * epoch,
            __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
            __in IFabricAsyncOperationCallback * callback)
        {
            HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
            if (FAILED(hr)) { return hr; }

            if (epoch == NULL) { return E_POINTER; }
            if (epoch->Reserved != NULL) { return E_INVALIDARG; }

            epoch_ = *epoch;
            previousEpochLastSequenceNumber_ = previousEpochLastSequenceNumber;

            return S_OK;
        }

        static HRESULT End(__in IFabricAsyncOperationContext * context)
        {
            if (context == NULL) { return E_POINTER; }

            ComPointer<UpdateEpochOperationContext> thisOperation(context, CLSID_ComStateProvider_UpdateEpochOperationContext);
            return thisOperation->Result;
        }

    protected:
        virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
        {
            auto operation = owner_.impl_->BeginUpdateEpoch(
                epoch_,
                previousEpochLastSequenceNumber_,
                [this](AsyncOperationSPtr const & operation){ this->FinishUpdateEpoch(operation, false); },
                proxySPtr);

            this->FinishUpdateEpoch(operation, true);
        }

    private:
        void FinishUpdateEpoch(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = owner_.impl_->EndUpdateEpoch(operation);
            TryComplete(operation->Parent, error.ToHResult());
        }

    private:
        ComStateProvider & owner_;
        FABRIC_EPOCH epoch_;
        FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber_;
};

// ********************************************************************************************************************
// ComStateProvider::OnDataLossOperationContext Implementation
//

// {39B26CFA-AF5E-4528-8318-87CECDE9D30F}
static const GUID CLSID_ComStateProvider_OnDataLossOperationContext = 
{ 0x39b26cfa, 0xaf5e, 0x4528, { 0x83, 0x18, 0x87, 0xce, 0xcd, 0xe9, 0xd3, 0xf } };

class ComStateProvider::OnDataLossOperationContext :
    public ComAsyncOperationContext
{
  DENY_COPY(OnDataLossOperationContext);

    COM_INTERFACE_AND_DELEGATE_LIST(
        OnDataLossOperationContext, 
        CLSID_ComStateProvider_OnDataLossOperationContext,
        OnDataLossOperationContext,
        ComAsyncOperationContext)

    public:
        OnDataLossOperationContext(__in ComStateProvider & owner) 
            : ComAsyncOperationContext(),
            owner_(owner),
            isStateChanged_(false)
        {
        }

        virtual ~OnDataLossOperationContext()
        {
        }

        static HRESULT End(__in IFabricAsyncOperationContext * context, __out BOOLEAN * isStateChanged)
        {
            if (context == NULL) { return E_POINTER; }
            if (isStateChanged == NULL) { return E_POINTER; }

            ComPointer<OnDataLossOperationContext> thisOperation(context, CLSID_ComStateProvider_OnDataLossOperationContext);
            *isStateChanged = (thisOperation->isStateChanged_) ? TRUE : FALSE;
            return thisOperation->Result;
        }

    protected:
        virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
        {
            auto operation = owner_.impl_->BeginOnDataLoss(
                [this](AsyncOperationSPtr const & operation){ this->FinishOnDataLoss(operation, false); },
                proxySPtr);

            this->FinishOnDataLoss(operation, true);
        }

    private:
        void FinishOnDataLoss(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = owner_.impl_->EndOnDataLoss(operation, isStateChanged_);
            TryComplete(operation->Parent, error.ToHResult());
        }

    private:
        ComStateProvider & owner_;
        bool isStateChanged_;
};

// ********************************************************************************************************************
// ComStateProvider::ComStateProvider Implementation
//

ComStateProvider::ComStateProvider(IStateProviderPtr const & impl)
    : IFabricStateProvider(),
    ComUnknownBase(),
    impl_(impl)
{
}

ComStateProvider::~ComStateProvider()
{
}


HRESULT ComStateProvider::BeginUpdateEpoch( 
    __in FABRIC_EPOCH const * epoch,
    __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
    __in IFabricAsyncOperationCallback *callback,
    __out IFabricAsyncOperationContext **context)
{
    auto error = impl_->Test_GetBeginUpdateEpochInjectedFault();
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<UpdateEpochOperationContext> operation 
        = make_com<UpdateEpochOperationContext>(*this);
    hr = operation->Initialize(
        root,
        epoch,
        previousEpochLastSequenceNumber,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComStateProvider::EndUpdateEpoch( 
    __in IFabricAsyncOperationContext *context)
{
    return ComUtility::OnPublicApiReturn(UpdateEpochOperationContext::End(context));
}

HRESULT ComStateProvider::GetLastCommittedSequenceNumber(
    __out FABRIC_SEQUENCE_NUMBER * sequenceNumber)
{
    if (sequenceNumber == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    auto error = impl_->GetLastCommittedSequenceNumber(*sequenceNumber);
    return ComUtility::OnPublicApiReturn(error.ToHResult());
}

HRESULT ComStateProvider::BeginOnDataLoss(
    __in IFabricAsyncOperationCallback * callback,
    __out IFabricAsyncOperationContext ** context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto root = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    ComPointer<OnDataLossOperationContext> operation 
        = make_com<OnDataLossOperationContext>(*this);
    hr = operation->Initialize(
        root,
        callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComStateProvider::EndOnDataLoss(
    __in IFabricAsyncOperationContext * context,
    __out BOOLEAN * isStateChangedResult)
{
    return ComUtility::OnPublicApiReturn(OnDataLossOperationContext::End(context, isStateChangedResult));
}

HRESULT ComStateProvider::GetCopyContext(
    __out IFabricOperationDataStream ** context)
{
    if (context == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    ComPointer<IFabricOperationDataStream> copyContextStream;
    auto error = impl_->GetCopyContext(copyContextStream);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    HRESULT hr = copyContextStream->QueryInterface(IID_IFabricOperationDataStream, reinterpret_cast<void**>(context));
    return ComUtility::OnPublicApiReturn(hr);      
}
    
HRESULT ComStateProvider::GetCopyState(
    __in FABRIC_SEQUENCE_NUMBER uptoOperationLSN,
    __in IFabricOperationDataStream * context, 
    __out IFabricOperationDataStream ** enumerator)
{
    if (enumerator == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    ComPointer<IFabricOperationDataStream> copyContextStream;
    copyContextStream.SetAndAddRef(context);

    ComPointer<IFabricOperationDataStream> copyStateStream;
    
    auto error = impl_->GetCopyState(uptoOperationLSN, copyContextStream, copyStateStream);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    HRESULT hr = copyStateStream->QueryInterface(IID_IFabricOperationDataStream, reinterpret_cast<void**>(enumerator));
    return ComUtility::OnPublicApiReturn(hr); 
}
