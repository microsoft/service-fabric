// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace Communication::TcpServiceCommunication;
using namespace std;

static const StringLiteral TraceType("ServiceCommunicationListener");

class ServiceCommunicationListener::OpenAsyncOperation
    : public AsyncOperation
{
    DENY_COPY(OpenAsyncOperation);

public:
    OpenAsyncOperation(
        ServiceCommunicationListener & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
    {
    }

    virtual ~OpenAsyncOperation() {}

    static ErrorCode End(AsyncOperationSPtr const & operation, __out wstring & reply)
    {
        auto casted = AsyncOperation::End<OpenAsyncOperation>(operation);
        if (casted->Error.IsSuccess())
        {
            reply = casted->owner_.endpointAddress_;
        }
        return casted->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        //Intention is to not allow multple open calls
        if (!owner_.TransitionToOpened().IsSuccess())
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::AlreadyExists);
            return;
        }

        auto error = owner_.transport_->Open();


        if (error.IsSuccess())
        {
            error = owner_.transport_->RegisterCommunicationListener(
                owner_.dispatcher_,
                owner_.endpointAddress_);

            if (!error.IsSuccess())
            {
                //For Clean Up.
                this->owner_.Abort();
            }
        }
        else
        {  //For Clean Up.
            this->owner_.Abort();
        }

        this->TryComplete(thisSPtr, error);
    }

private:
    ServiceCommunicationListener & owner_;
};

class ServiceCommunicationListener::CloseAsyncOperation
    : public AsyncOperation
{
    DENY_COPY(CloseAsyncOperation);

public:
    CloseAsyncOperation(
        ServiceCommunicationListener & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        :AsyncOperation(callback, parent)
        , owner_(owner)
    {
    }

    virtual ~CloseAsyncOperation() {}


    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<CloseAsyncOperation>(operation);
        return casted->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.transport_.get() == nullptr)
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
            return;
        }
        if (!this->owner_.TransitionToClosed().IsSuccess())
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::ObjectClosed);
            return;
        }

        auto operation = owner_.transport_->BeginUnregisterCommunicationListener(owner_.dispatcher_->ServiceInfo,
            [this](AsyncOperationSPtr const & operation) { this->OnUnRegisterComplete(operation, false); },
            thisSPtr);

        this->OnUnRegisterComplete(operation, true);
    }

    void OnUnRegisterComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.transport_->EndUnregisterCommunicationListener(operation);
        this->owner_.Cleanup();
        this->TryComplete(operation->Parent, error);
    }

private:
    ServiceCommunicationListener & owner_;
};

AsyncOperationSPtr ServiceCommunicationListener::BeginOpen(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return  AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        callback,
        parent);
}

ErrorCode ServiceCommunicationListener::EndOpen(
    AsyncOperationSPtr const & asyncOperation,
    __out wstring & serviceAddress)
{
    return OpenAsyncOperation::End(asyncOperation, serviceAddress);
}

AsyncOperationSPtr ServiceCommunicationListener::BeginClose(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return  AsyncOperation::CreateAndStart<CloseAsyncOperation>(
        *this,
        callback,
        parent);
}

ErrorCode ServiceCommunicationListener::EndClose(
    AsyncOperationSPtr const & asyncOperation)
{
    return CloseAsyncOperation::End(asyncOperation);
}

void ServiceCommunicationListener::AbortListener()
{
    this->Abort();
}

void ServiceCommunicationListener::Cleanup()
{
    this->transport_->Close();
    this->transport_.reset();
}


void ServiceCommunicationListener::OnAbort()
{
    if (transport_.get() != nullptr)
    {
        //To not unregister if not got registered.
        if (!endpointAddress_.empty())
        {
            AsyncOperationWaiter waiter;

            auto operation = this->transport_->BeginUnregisterCommunicationListener(
                this->dispatcher_->ServiceInfo,
                [this, &waiter](AsyncOperationSPtr const & operation)
            {
                this->transport_->EndUnregisterCommunicationListener(operation);
                waiter.Set();
            },
                AsyncOperationSPtr());

            waiter.WaitOne();
        }

        this->Cleanup();
    }
}


ServiceCommunicationListener::~ServiceCommunicationListener()
{
    WriteInfo(TraceType, "destructing ServiceCommunicationListener {0} ", this->endpointAddress_);
}
