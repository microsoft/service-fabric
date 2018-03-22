// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Communication::TcpServiceCommunication;
using namespace std;
using namespace Transport;
using namespace Api;

static const StringLiteral TraceType("ServiceCommunicationClient.TryConnectAsyncOperation");

class ServiceCommunicationClient::TryConnectAsyncOperation::OnConnectAsyncOperation
    : public LinkableAsyncOperation
{
    DENY_COPY(OnConnectAsyncOperation)
public:

    OnConnectAsyncOperation
    (__in ServiceCommunicationClient& owner,
        TimeSpan const timeout,
        AsyncOperationSPtr const& primary,
        AsyncCallback const& callback,
        AsyncOperationSPtr const& parent)
        : LinkableAsyncOperation(primary, callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    OnConnectAsyncOperation(
        __in ServiceCommunicationClient& owner,
        TimeSpan const timeout,
        AsyncCallback const& callback,
        AsyncOperationSPtr const& parent)
        : LinkableAsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const& asyncOperation)
    {
        auto casted = AsyncOperation::End<OnConnectAsyncOperation>(asyncOperation);

        return casted->Error;
    }

protected:
    void OnResumeOutsideLockPrimary(AsyncOperationSPtr const& thisSPtr)
    {
        //Create Connect Request

        //Send Connect Request
        auto operation = this->owner_.requestReply_.BeginRequest(move(CreateConnectRequest()),
            this->owner_.serverSendTarget_,
            TransportPriority::Normal,
            this->timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const& operation)
        {
            this->OnConnectComplete(operation, false);
        },
            thisSPtr);

        this->OnConnectComplete(operation, true);
    }


private:

    void OnConnectComplete(AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;

        ErrorCode error = this->owner_.requestReply_.EndRequest(operation, reply);

        if (error.IsSuccess())
        {
            ServiceCommunicationErrorHeader errorHeader;
            if (reply->Headers.TryReadFirst(errorHeader))
            {
                error = ErrorCode(static_cast<Common::ErrorCodeValue::Enum>(errorHeader.ErrorCodeValue));

                if (!isConnectSucceded(error))
                {
                    error = DisconnectAndSetConnectError(error);
                }
                else
                { //This means we got a valid reply. Hence it is Connected.
                    error = ErrorCodeValue::Success;
                    WriteInfo(TraceType, this->owner_.clientId_, "Connect Succeeded :  for Service {0}", this->owner_.serviceLocation_);

                    if (this->owner_.TransitionToConnected().IsSuccess())
                    {
                        if (this->owner_.eventHandler_.get() != nullptr)
                        {
                            auto root = this->owner_.CreateComponentRoot();
                            auto &localowner = this->owner_;
                            Threadpool::Post([&localowner, root]()
                            {
                                localowner.eventHandler_->OnConnected(localowner.serverAddress_);
                            });
                        }
                    }
                }
            }
        }
        else
        {
            error = DisconnectAndSetConnectError(error);
        }

        TryComplete(operation->Parent, error);
    }

    bool isConnectSucceded(ErrorCode error)
    {
        //EndpointnotFound check  for backward Compatibilty
        if (error.IsSuccess())
        {
            return true;
        }
        if (error.IsError(ErrorCodeValue::EndpointNotFound) && !this->owner_.explicitConnect_)
        {
            return true;
        }
        return false;
    }

    ErrorCode DisconnectAndSetConnectError(ErrorCode const & error)
    {
        ErrorCode connectError = error;
        WriteWarning(TraceType, this->owner_.clientId_, "Error While Receiving Connect Reply : {0} from Service {1}", error, this->owner_.serviceLocation_);

        //If its Timeout for Connect Request , then we set error as Cannot Conenct
        if (ServiceCommunicationHelper::IsCannotConnectErrorDuringConnect(error))
        {
            connectError = ErrorCodeValue::ServiceCommunicationCannotConnect;
        }

        this->owner_.connectError_ = connectError;

        this->owner_.TransitionToConnectFailed();

        //This will make sure disconnect happens
        this->owner_.serverSendTarget_->Reset();

        return connectError;
    }

    MessageUPtr CreateConnectRequest()
    {
        MessageUPtr request = make_unique<Message>();
        this->AddConnectHeader(request);
        return request;
    }

    void AddConnectHeader(MessageUPtr& message)
    {
        message->Headers.Add(ActorHeader(Actor::ServiceCommunicationActor));
        wstring serviceLocation;
        StringWriter(serviceLocation).Write(owner_.serviceLocation_);
        StringWriter(serviceLocation).Write(Constants::ConnectActorDelimiter);
        //Needed for old service to reject Request
        StringWriter(serviceLocation).Write(Constants::ConnectAction);
        message->Headers.Add(ServiceLocationActorHeader(serviceLocation));
        message->Headers.Replace(TimeoutHeader(this->timeoutHelper_.GetRemainingTime()));
        message->Headers.Add(FabricActivityHeader(ActivityId()));
        message->Headers.Add(ActionHeader(Constants::ConnectAction));

        TcpClientIdHeader header{ this->owner_.clientId_ };
        message->Headers.Add(header);
    }

    ServiceCommunicationClient& owner_;
    TimeoutHelper timeoutHelper_;
};

ServiceCommunicationClient::TryConnectAsyncOperation::TryConnectAsyncOperation(
    __in ServiceCommunicationClient& owner,
    TimeSpan const& timeout,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
    : AsyncOperation(callback, parent),
    owner_(owner),
    timeoutHelper_(timeout)
{
}

ErrorCode ServiceCommunicationClient::TryConnectAsyncOperation::End(AsyncOperationSPtr const& operation)
{
    auto casted = AsyncOperation::End<TryConnectAsyncOperation>(operation);

    return casted->Error;
}

void ServiceCommunicationClient::TryConnectAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    auto state = this->owner_.GetState();
    if (state == this->owner_.Connected)
    {
        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
    else if ((this->owner_.GetTerminalStatesMask() & state) != 0)
    {
        if (this->owner_.connectError_.IsSuccess())
        {
            //this will the case where connect completed/not started  but client got dicsconnected/aborted
            this->TryComplete(thisSPtr, ErrorCodeValue::ServiceCommunicationCannotConnect);
        }
        else
        {
            this->TryComplete(thisSPtr, this->owner_.connectError_);
        }

    }
    else
    {
        bool connectSucceded = false;

        AsyncOperationSPtr operation;
        {
            AcquireExclusiveLock lock(this->owner_.connectLock_);
            //This check is for raceCondition between checking state and Connect Operation Completion
            auto finalstate = this->owner_.GetState();
            if (finalstate == this->owner_.Connected)
            {
                connectSucceded = true;
            }
            else if ((this->owner_.GetTerminalStatesMask() & finalstate) != 0)
            {
                this->TryComplete(thisSPtr, ErrorCodeValue::ServiceCommunicationCannotConnect);
                return;
            }
            else
            {
                if (this->owner_.primaryConnectOperation_ == nullptr)
                {
                    //Primary Operation for OnConnect and on Callback it will call SendRequest if Succeeded
                    operation = AsyncOperation::CreateAndStart<OnConnectAsyncOperation>(
                        this->owner_,
                        this->timeoutHelper_.GetRemainingTime(),
                        [this](AsyncOperationSPtr const& operation)
                    {
                        this->OnConnected(operation, false);
                    },
                        thisSPtr);
                    this->owner_.primaryConnectOperation_ = operation;
                }
                else
                {
                    // Secondary operation for OnConnect and on Callback it will call SendRequest if Succeeded
                    operation = AsyncOperation::CreateAndStart<OnConnectAsyncOperation>(
                        this->owner_,
                        this->timeoutHelper_.GetRemainingTime(),
                        this->owner_.primaryConnectOperation_,
                        [this](AsyncOperationSPtr const& operation)
                    {
                        this->OnConnected(operation, false);
                    },
                        thisSPtr);
                }
            }
        }


        if (connectSucceded)
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        AsyncOperation::Get<OnConnectAsyncOperation>(operation)->ResumeOutsideLock(operation);
        this->OnConnected(operation, true);
    }
}

void ServiceCommunicationClient::TryConnectAsyncOperation::OnConnected(AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = OnConnectAsyncOperation::End(operation);
    //if connection succceded , Send actual message , else return error
    this->TryComplete(operation->Parent, error);


    {
        AcquireExclusiveLock lock(this->owner_.connectLock_);
        //So that primary operation gets freed.This has been done after making flag as Connected/Connect Failed
        this->owner_.primaryConnectOperation_ = nullptr;
    }

}



