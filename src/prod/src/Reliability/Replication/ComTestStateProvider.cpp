// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ComTestStateProvider.h"
#include "ComTestOperation.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
 
namespace ReplicationUnitTest
{
    using namespace std;
    using namespace Common;

    using namespace Reliability::ReplicationComponent;

    static FABRIC_EPOCH const InitialEpoch = {Constants::InvalidLSN, Constants::InvalidLSN, NULL};
    static Common::StringLiteral const TestStateProviderSource("TESTStateProvider");

    ComTestStateProvider::ComTestStateProvider(
        int64 numberOfCopyOperations,
        int64 numberOfCopyContextOperations,
        ComponentRoot const & root,
        bool changeStateOnDataLoss)
        :   IFabricStateProvider(),
            ComUnknownBase(),
            numberOfCopyOperations_(numberOfCopyOperations),
            numberOfCopyContextOperations_(numberOfCopyContextOperations),
            root_(root),
            changeStateOnDataLoss_(changeStateOnDataLoss),
            progressVector_()
    {
        ASSERT_IF(
            numberOfCopyContextOperations_ > numberOfCopyOperations_,
            "The number of copy context ops {0} must be smaller or equal to the number of copy ops {1}",
            numberOfCopyContextOperations_,
            numberOfCopyOperations_);

        ComTestOperation::WriteInfo(
            TestStateProviderSource,
            "ComTestStateProvider.ctor: {0} copy ops, {1} copy context ops", 
            numberOfCopyOperations_, 
            numberOfCopyContextOperations_);            

        // Add first entry, epoch 0, LSN -1
        progressVector_.push_back(pair<FABRIC_EPOCH, FABRIC_SEQUENCE_NUMBER>(
            InitialEpoch, 
            Constants::NonInitializedLSN));
    }

    std::wstring ComTestStateProvider::GetProgressVectorString() const
    {
        wstring content;
        StringWriter writer(content);
        for_each(progressVector_.begin(), progressVector_.end(),
            [&writer] (ProgressVectorEntry const & entry)
            {
                FABRIC_EPOCH const & epoch = entry.first;
                writer << epoch.DataLossNumber << "." << epoch.ConfigurationNumber << ":" << entry.second << ";";
            });

        return content;
    }

    void ComTestStateProvider::ClearProgressVector()
    {
        progressVector_.clear();
        progressVector_.push_back(pair<FABRIC_EPOCH, FABRIC_SEQUENCE_NUMBER>(
            InitialEpoch, 
            Constants::NonInitializedLSN));
    }

    HRESULT ComTestStateProvider::CreateOperation(
        bool lastOp,
        __out Common::ComPointer<IFabricOperationData> & op) const
    {
        if (!lastOp)
        {
            switch(std::rand() % 3)
            {
            case 0:
                op = make_com<ComTestOperation,IFabricOperationData>(L"Dummy State Provider generated Copy operation");
                break;
            case 1:
                op = make_com<ComTestOperation,IFabricOperationData>(L""); // operation data with 1 empty buffer
                break;
            default:
                op = make_com<ComTestOperation,IFabricOperationData>(); // operation data with 0 buffers
            }
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ComTestStateProvider::BeginUpdateEpoch( 
        /* [in] */ FABRIC_EPOCH const * epoch,
        /* [in] */ FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
        /* [in] */ IFabricAsyncOperationCallback *callback,
        /* [retval][out] */ IFabricAsyncOperationContext **context)
    {
        Common::AcquireWriteLock grab(progressVectorLock_);
        // Update previous entry with current provided LSN
        progressVector_[progressVector_.size() - 1].second = previousEpochLastSequenceNumber;
        progressVector_.push_back(pair<FABRIC_EPOCH, FABRIC_SEQUENCE_NUMBER>(
            *epoch, 
            Constants::NonInitializedLSN));

        ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();

        HRESULT hr = operation->Initialize(root_.shared_from_this(), callback);
        if (FAILED(hr)) { return hr; }

        ComAsyncOperationContextCPtr baseOperation;
        baseOperation.SetNoAddRef(operation.DetachNoRelease());
        hr = baseOperation->Start(baseOperation);
        if (FAILED(hr)) { return hr; }

        *context = baseOperation.DetachNoRelease();

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ComTestStateProvider::EndUpdateEpoch( 
        /* [in] */ IFabricAsyncOperationContext *context)
    {
        return ComCompletedAsyncOperationContext::End(context);
    }

    void ComTestStateProvider::WaitForConfigurationNumber(LONGLONG configurationNumber)
    {
        bool found = false;
        while (!found)
        {
            {
                Common::AcquireWriteLock grab(progressVectorLock_);
                if (!progressVector_.empty())
                {
                    found = (progressVector_.back().first.ConfigurationNumber == configurationNumber);
                }
            }
            if (!found)
            {
                Sleep(100);
            }
        }
    }

    HRESULT STDMETHODCALLTYPE ComTestStateProvider::GetLastCommittedSequenceNumber( 
        /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *sequenceNumber)
    {
        if (sequenceNumber == NULL)
        {
            return E_POINTER;
        }

        // Take progress from progress vector
        if (progressVector_.size() > 1)
        {
            // Last entry contains non-initialized LSN.,
            // so take LSN from previous entry
            *sequenceNumber = progressVector_[progressVector_.size() - 2].second;
        }
        else
        {
            *sequenceNumber = 0;
        }

        return S_OK;
    }

    HRESULT ComTestStateProvider::BeginOnDataLoss( 
        /* [in] */ IFabricAsyncOperationCallback *callback,
        /* [retval][out] */ IFabricAsyncOperationContext **context)
    {
        ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();

        HRESULT hr = operation->Initialize(root_.shared_from_this(), callback);
        if (FAILED(hr)) { return hr; }

        ComAsyncOperationContextCPtr baseOperation;
        baseOperation.SetNoAddRef(operation.DetachNoRelease());
        hr = baseOperation->Start(baseOperation);
        if (FAILED(hr)) { return hr; }

        *context = baseOperation.DetachNoRelease();
        return S_OK;
    }

    HRESULT ComTestStateProvider::EndOnDataLoss( 
        /* [in] */ IFabricAsyncOperationContext *context,
        /* [retval][string][out] */ BOOLEAN * isStateChanged)
    {
        *isStateChanged = changeStateOnDataLoss_ ? TRUE : FALSE;
        return ComCompletedAsyncOperationContext::End(context);
    }

    HRESULT ComTestStateProvider::GetCopyContext(
        /*[out, retval]*/ IFabricOperationDataStream **enumerator)
    {
        // If the number of operations specified is -1, leave the enumerator NULL;
        // otherwise, create it
        if (numberOfCopyContextOperations_ != Reliability::ReplicationComponent::Constants::NonInitializedLSN)
        {
            *enumerator = make_com<ComTestAsyncEnumOperationData>(
                L"SecondaryCopyContextEnum",
                numberOfCopyContextOperations_, 
                *this, 
                root_).DetachNoRelease();
        }

        return S_OK;
    }

    HRESULT ComTestStateProvider::GetCopyState(
        /*[in]*/ FABRIC_SEQUENCE_NUMBER uptoSequenceNumber, 
        /*[in]*/ IFabricOperationDataStream *contextEnumerator,
        /*[out, retval]*/ IFabricOperationDataStream ** enumerator)
    {
        ASSERT_IF(
            uptoSequenceNumber == Reliability::ReplicationComponent::Constants::NonInitializedLSN,
            "uptoSequenceNumber can't be {0}",
            uptoSequenceNumber);

        if (contextEnumerator != nullptr)
        {
            ComPointer<IFabricOperationDataStream> contextEnumPtr;
            contextEnumPtr.SetAndAddRef(contextEnumerator);
            *enumerator = make_com<ComTestAsyncEnumOperationData>(
                L"PrimaryCopyEnum",
                numberOfCopyOperations_, 
                numberOfCopyContextOperations_,
                move(contextEnumPtr),
                *this,
                root_).DetachNoRelease();
        }
        else
        {
            *enumerator = make_com<ComTestAsyncEnumOperationData>(
                L"PrimaryCopyEnum",
                numberOfCopyOperations_, 
                *this,
                root_).DetachNoRelease();         
        }
        return S_OK;
    }

    ComProxyStateProvider ComTestStateProvider::GetProvider(
        int64 numberOfCopyOperations,
        int64 numberOfCopyContextOperations,
        ComponentRoot const & root)
    {
        ComPointer<IFabricStateProvider> providerPointer = 
            make_com<ComTestStateProvider,IFabricStateProvider>(
                numberOfCopyOperations, 
                numberOfCopyContextOperations,
                root);
        return ComProxyStateProvider(std::move(providerPointer));
    }
    /*************************************
     * ComTestAsyncEnumOperation
    /*************************************/
    static const GUID CLSID_GetNextOperation = { 0x7567acbb, 0xe500, 0x4acb, { 0xb3, 0x75, 0x82, 0x7a, 0x65, 0x4f, 0x88, 0x75 } };
    class ComTestStateProvider::ComTestAsyncEnumOperationData::ComGetNextOperation : public ComAsyncOperationContext
    {
        DENY_COPY(ComGetNextOperation)

        COM_INTERFACE_LIST2(
            ComGetNextOperation,
            IID_IFabricAsyncOperationContext,
            IFabricAsyncOperationContext,
            CLSID_GetNextOperation,
            ComGetNextOperation)

    public:
        explicit ComGetNextOperation(
            __in ComPointer<ComTestAsyncEnumOperationData> enumerator)
            :
            comOperationPointer_(),
            comEnumeratorPointer_(enumerator)
        {
        }

        virtual ~ComGetNextOperation() { }

        HRESULT Initialize(
            __in IFabricAsyncOperationCallback * inCallback)
        {
            HRESULT hr = this->ComAsyncOperationContext::Initialize(comEnumeratorPointer_->rootSPtr_, inCallback);
            if (FAILED(hr)) { return hr; }
            return S_OK;
        }

        static HRESULT End(
            __in IFabricAsyncOperationContext * context, 
            __out IFabricOperationData ** operation)
        {
            if (context == NULL) { return E_INVALIDARG; }
            if (operation == NULL) { return E_INVALIDARG; }
            ComPointer<ComGetNextOperation> asyncOperation(context, CLSID_GetNextOperation);
            HRESULT hr = asyncOperation->ComAsyncOperationContext::End();
            if (FAILED(hr)) { return hr; }
            *operation = asyncOperation->comOperationPointer_.DetachNoRelease();
            return asyncOperation->Result;
        }

    private:
        virtual void OnStart(
            Common::AsyncOperationSPtr const & proxySPtr)
        {
            if (comEnumeratorPointer_->numberOfInnerOperations_ >= 0)
            {
                // Get one operation from the in enumerator
                // and generate an out operation;
                // The number of the inner operations is smaller or equal to the
                // number of outer operations.
                auto inner = comEnumeratorPointer_->inEnumerator_.BeginGetNext(
                    [this](AsyncOperationSPtr const & asyncOperation) { this->GetNextCallback(asyncOperation); },
                    proxySPtr);
            }
            else
            {
                GenerateOp(proxySPtr);
            }
        }

        void GetNextCallback(AsyncOperationSPtr const & asyncOperation)
        {
            ComPointer<IFabricOperationData> operation;
            bool isLast;
            auto error = comEnumeratorPointer_->inEnumerator_.EndGetNext(
                asyncOperation, 
                isLast,
                operation);

            if (!error.IsSuccess())
            {
                this->TryComplete(asyncOperation->Parent, error.ToHResult());
            }
            else
            {
                ComTestOperation::WriteInfo(
                    TestStateProviderSource,
                    "{0}: GetNext for inner enum succeeded, last {1}",
                    comEnumeratorPointer_->id_,
                    isLast);
                if (isLast && comEnumeratorPointer_->numberOfInnerOperations_ > 0)
                {
                    VERIFY_FAIL(L"Inner enumerator: There should be no operations left when done");
                }
                --comEnumeratorPointer_->numberOfInnerOperations_;
                GenerateOp(asyncOperation->Parent);
            }
        }

        void GenerateOp(AsyncOperationSPtr const & proxySPtr)
        {
            bool lastOp = !comEnumeratorPointer_->TryIncrementOperationNumber();
            HRESULT hr = comEnumeratorPointer_->stateProvider_.CreateOperation(
                lastOp,
                comOperationPointer_);
            if (lastOp)
            {
                ComTestOperation::WriteInfo(
                    TestStateProviderSource,
                    "{0}: Generate last NULL op",
                    comEnumeratorPointer_->id_);
            }
            else
            {
                ComTestOperation::WriteInfo(
                    TestStateProviderSource,
                    "{0}: Generate non-NULL Copy operation",
                    comEnumeratorPointer_->id_);                
            }

            // Complete sync and async, randomly,
            // to test both paths
            int random = std::rand() % 10; 
            if (random < 6)
            {
                Common::Threadpool::Post([this, proxySPtr, hr]() 
                { 
                    this->TryComplete(proxySPtr, hr); 
                });
            }
            else
            {
                // Complete sync
                this->TryComplete(proxySPtr, hr);
            }
        }

        ComPointer<ComTestStateProvider::ComTestAsyncEnumOperationData> comEnumeratorPointer_;
        ComPointer<IFabricOperationData> comOperationPointer_;
    };

    ComTestStateProvider::ComTestAsyncEnumOperationData::ComTestAsyncEnumOperationData(
        wstring const & id,
        int64 numberOfOperations,
        ComTestStateProvider const & stateProvider,
        Common::ComponentRoot const & root)
        :   IFabricOperationDataStream(),
            ComUnknownBase(), 
            id_(id),
            numberOfOperations_(numberOfOperations),
            numberOfInnerOperations_(-1),
            inEnumerator_(),
            currentIndex_(0),
            stateProvider_(stateProvider),
            rootSPtr_(root.shared_from_this())
    {
    }

    ComTestStateProvider::ComTestAsyncEnumOperationData::ComTestAsyncEnumOperationData(
        wstring const & id,
        int64 numberOfOperations,
        int64 numberOfInnerOperations,
        Common::ComPointer<IFabricOperationDataStream> && enumerator,
        ComTestStateProvider const & stateProvider,
        Common::ComponentRoot const & root)
        :   IFabricOperationDataStream(),
            ComUnknownBase(), 
            id_(id),
            numberOfOperations_(numberOfOperations),
            numberOfInnerOperations_(numberOfInnerOperations),
            inEnumerator_(move(enumerator)),
            currentIndex_(0),
            stateProvider_(stateProvider),
            rootSPtr_(root.shared_from_this())
    {
    }

    bool ComTestStateProvider::ComTestAsyncEnumOperationData::TryIncrementOperationNumber() const 
    { 
        if (currentIndex_.load() >= numberOfOperations_)
        {
            // last operation
            return false;
        }
        else
        {
            ++currentIndex_;
            return true;
        }
    }

    HRESULT ComTestStateProvider::ComTestAsyncEnumOperationData::BeginGetNext(
        /*[in]*/ IFabricAsyncOperationCallback * callback,
        /*[out, retval]*/ IFabricAsyncOperationContext **context)
    {
        if (context == NULL) { return E_INVALIDARG; }

        ComPointer<ComTestAsyncEnumOperationData> thisCPtr;
        thisCPtr.SetAndAddRef(this);

        ComPointer<ComGetNextOperation> getOperation = make_com<ComGetNextOperation>(move(thisCPtr));

        HRESULT hr = getOperation->Initialize(callback);
        if (FAILED(hr)) { return hr; }

        ComAsyncOperationContextCPtr baseOperation;
        baseOperation.SetNoAddRef(getOperation.DetachNoRelease());
        hr = baseOperation->Start(baseOperation);
        if (FAILED(hr)) { return hr; }

        *context = baseOperation.DetachNoRelease();
        return S_OK;
    }

    HRESULT ComTestStateProvider::ComTestAsyncEnumOperationData::EndGetNext(
        /*[in]*/ IFabricAsyncOperationContext * context,
        /*[out, retval]*/ IFabricOperationData ** operation)
    {
        if ((context == NULL) || (operation == NULL)) { return E_INVALIDARG; }

        return ComGetNextOperation::End(context, operation);
    }
}
