// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace Federation;
using namespace Hosting2;
using namespace Query;
using namespace ServiceModel;
using namespace Store;
using namespace SystemServices;
using namespace Transport;
using namespace Management;
using namespace Management::FaultAnalysisService;
using namespace ClientServerTransport;

StringLiteral const TraceComponent("FaultAnalysisServiceAgent");

//------------------------------------------------------------------------------
// Message Dispatch Helpers
//

class FaultAnalysisServiceAgent::DispatchMessageAsyncOperationBase
    : public TimedAsyncOperation
    , protected NodeActivityTraceComponent<TraceTaskCodes::FaultAnalysisService>
{
public:
    DispatchMessageAsyncOperationBase(
        Federation::NodeInstance const & nodeInstance,
        __in IFaultAnalysisService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , NodeActivityTraceComponent(nodeInstance, FabricActivityHeader::FromMessage(*message).ActivityId)
        , service_(service)
        , request_(move(message))
        , receiverContext_(move(receiverContext))
    {
    }

    virtual ~DispatchMessageAsyncOperationBase() { }

    __declspec(property(get = get_Service)) IFaultAnalysisService & Service;
    IFaultAnalysisService & get_Service() { return service_; }

    __declspec(property(get = get_Request)) Message & Request;
    Message & get_Request() { return *request_; }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out MessageUPtr & reply, __out IpcReceiverContextUPtr & receiverContext)
    {
        auto casted = AsyncOperation::End<DispatchMessageAsyncOperationBase>(operation);

        if (casted->Error.IsSuccess())
        {
            reply = move(casted->reply_);
        }

        // Always return receiver context for caller to send IPC failure
        receiverContext = move(casted->receiverContext_);

        return casted->Error;
    }

protected:

	void OnStart(AsyncOperationSPtr const & thisSPtr)
	{
		SystemServiceMessageBody requestBody;
		if (this->Request.SerializedBodySize() > 0)
		{
			if (!this->Request.GetBody(requestBody))
			{
				WriteWarning(TraceComponent, "{0} failed to get process incoming request: {1}", this->TraceId, this->Request.MessageId);
				this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
				return;
			}
		}

		auto operation = this->Service.BeginCallSystemService(
			std::move(this->Request.Action),
			std::move(requestBody.Content),
			this->RemainingTime,
			[this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
			thisSPtr);
		this->OnDispatchComplete(operation, true);
	}

	virtual ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
	{
		wstring result;

		ErrorCode error = this->Service.EndCallSystemService(operation, result);

		if (!error.IsSuccess())
		{
			Trace.WriteWarning(TraceComponent, "EndCallSystemServiceCall failed");
			return error;
		}

		SystemServiceReplyMessageBody replyBody(move(result));
		reply = FaultAnalysisServiceMessage::GetClientOperationSuccess(replyBody);
		return error;
	}

    void OnDispatchComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        ErrorCode error = this->EndDispatch(operation, reply_);

        this->TryComplete(operation->Parent, error); 
    }

private:

    IFaultAnalysisService & service_;
    MessageUPtr request_;
    IpcReceiverContextUPtr receiverContext_;
    MessageUPtr reply_;
};


class FaultAnalysisServiceAgent::InvokeDataLossAsyncOperation : public DispatchMessageAsyncOperationBase
{
public:
    InvokeDataLossAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IFaultAnalysisService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~InvokeDataLossAsyncOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        InvokeDataLossMessageBody body;
        if (this->Request.GetBody(body))
        {
            ScopedHeap heap;
            FABRIC_START_PARTITION_DATA_LOSS_DESCRIPTION publicDescription = { 0 };

            body.DataLossDescription.ToPublicApi(heap, publicDescription);

            InvokeDataLossDescription internalDescription;
            auto error = internalDescription.FromPublicApi(publicDescription);

            if (!error.IsSuccess())
            {
                Trace.WriteWarning(TraceComponent, "Agent/DataLoss/Could not convert back");
            }

            auto operation = this->Service.BeginStartPartitionDataLoss(
                &publicDescription,
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
                thisSPtr);
            this->OnDispatchComplete(operation, true);
        }
        else
        {
            WriteInfo(TraceComponent, "{0} failed to get process incoming request: {1}", this->TraceId, this->Request.MessageId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        Trace.WriteInfo(TraceComponent, "Inside EndDispatch of InvokeDataLoss in Agent");
        auto error = this->Service.EndStartPartitionDataLoss(operation);

        if (error.IsSuccess())
        {
            reply = FaultAnalysisServiceMessage::GetClientOperationSuccess();
        }

        return error;
    }
};


class FaultAnalysisServiceAgent::CancelTestCommandAsyncOperation : public DispatchMessageAsyncOperationBase
{
public:
    CancelTestCommandAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IFaultAnalysisService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~CancelTestCommandAsyncOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        CancelTestCommandMessageBody body;
        if (this->Request.GetBody(body))
        {
            FABRIC_CANCEL_TEST_COMMAND_DESCRIPTION publicDescription = {0};
            publicDescription.OperationId = body.Description.get_OperationId().AsGUID();
            publicDescription.Force = body.Description.get_Force() ? TRUE : FALSE;

            auto operation = this->Service.BeginCancelTestCommand(
                &publicDescription,
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
                thisSPtr);
            this->OnDispatchComplete(operation, true);
        }
        else
        {
            WriteInfo(TraceComponent, "{0} failed to get process incoming request: {1}", this->TraceId, this->Request.MessageId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        auto error = this->Service.EndCancelTestCommand(operation);

        if (error.IsSuccess())
        {
            reply = FaultAnalysisServiceMessage::GetClientOperationSuccess();
        }

        return error;
    }
};


class FaultAnalysisServiceAgent::StartNodeTransitionAsyncOperation : public DispatchMessageAsyncOperationBase
{
public:
    StartNodeTransitionAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IFaultAnalysisService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~StartNodeTransitionAsyncOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(TraceComponent, "FaultAnalysisServiceAagent::StartNodeTransitionAsyncOperation::OnStart ");

        StartNodeTransitionMessageBody body;
        if (this->Request.GetBody(body))
        {
            ScopedHeap heap;

            FABRIC_NODE_TRANSITION_DESCRIPTION description = {};

            WriteInfo(
               TraceComponent, 
               "FaultAnalysisServiceAagent::StartNodeTransitionAsyncOperation::OnStart NodeTransitionType:{0}, OperationId:{1}, nodeName:{2}, nodeInstance:{3}", 
               (int)body.Description.NodeTransitionType,
               body.Description.OperationId,
               body.Description.NodeName,
               body.Description.NodeInstanceId);

            body.Description.ToPublicApi(heap, description);            

            WriteNoise(TraceComponent, "FaultAnalysisServiceAagent::StartNodeTransitionAsyncOperation::OnStart after ToPublic");
           
            auto operation = this->Service.BeginStartNodeTransition(
                &description,
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
                thisSPtr);
            this->OnDispatchComplete(operation, true);
        }
        else
        {
            WriteInfo(TraceComponent, "{0} failed to get process incoming request: {1}", this->TraceId, this->Request.MessageId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        auto error = this->Service.EndStartNodeTransition(operation);

        if (error.IsSuccess())
        {
            reply = FaultAnalysisServiceMessage::GetClientOperationSuccess();
        }

        return error;
    }
};

class FaultAnalysisServiceAgent::GetInvokeDataLossProgressAsyncOperation : public DispatchMessageAsyncOperationBase
{
public:
    GetInvokeDataLossProgressAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IFaultAnalysisService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~GetInvokeDataLossProgressAsyncOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        GetInvokeDataLossProgressMessageBody body;
        if (this->Request.GetBody(body))
        {
            auto operation = this->Service.BeginGetPartitionDataLossProgress(
                body.OperationId.AsGUID(),
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
                thisSPtr);

            this->OnDispatchComplete(operation, true);
        }
        else
        {
            WriteInfo(TraceComponent, "GetInvokeDataLossProgress/ {0} failed to get process incoming request: {1}", this->TraceId, this->Request.MessageId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        ErrorCode error = this->Service.EndGetPartitionDataLossProgress(operation, &progressResult_);

        if (!error.IsSuccess())
        {
            Trace.WriteWarning(TraceComponent, "GetInvokeDataLossProgress in Agent/Service.EndGetInvokeDataLossProgress failed");
            return error;
        }

        InvokeDataLossProgress progress;

        const FABRIC_PARTITION_DATA_LOSS_PROGRESS * publicProgress = progressResult_->get_Progress();

        error = progress.FromPublicApi(*publicProgress);

        if (!error.IsSuccess())
        {
            return error;
        }
        else
        {
            InvokeDataLossProgress invokeDataLossProgress(move(progress));
            GetInvokeDataLossProgressReplyMessageBody replyBody(move(invokeDataLossProgress));
            reply = FaultAnalysisServiceMessage::GetClientOperationSuccess(replyBody);
        }

        return error;
    }

    IFabricPartitionDataLossProgressResult* progressResult_;
};

class FaultAnalysisServiceAgent::InvokeQuorumLossAsyncOperation : public DispatchMessageAsyncOperationBase
{
public:
    InvokeQuorumLossAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IFaultAnalysisService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~InvokeQuorumLossAsyncOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        InvokeQuorumLossMessageBody body;
        if (this->Request.GetBody(body))
        {
            ScopedHeap heap;
            FABRIC_START_PARTITION_QUORUM_LOSS_DESCRIPTION publicDescription = { 0 };

            body.QuorumLossDescription.ToPublicApi(heap, publicDescription);

            InvokeQuorumLossDescription internalDescription;
            auto error = internalDescription.FromPublicApi(publicDescription);

            if (!error.IsSuccess())
            {
                Trace.WriteWarning(TraceComponent, "Agent/QuorumLoss/Could not convert back");
            }

            auto operation = this->Service.BeginStartPartitionQuorumLoss(
                &publicDescription,
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
                thisSPtr);
            this->OnDispatchComplete(operation, true);
        }
        else
        {
            WriteInfo(TraceComponent, "{0} failed to get process incoming request: {1}", this->TraceId, this->Request.MessageId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        ErrorCode error = this->Service.EndStartPartitionQuorumLoss(operation);

        if (error.IsSuccess())
        {
            reply = FaultAnalysisServiceMessage::GetClientOperationSuccess();
        }

        return error;
    }
};

class FaultAnalysisServiceAgent::GetInvokeQuorumLossProgressAsyncOperation : public DispatchMessageAsyncOperationBase
{
public:
    GetInvokeQuorumLossProgressAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IFaultAnalysisService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~GetInvokeQuorumLossProgressAsyncOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        GetInvokeQuorumLossProgressMessageBody body;
        if (this->Request.GetBody(body))
        {
            auto operation = this->Service.BeginGetPartitionQuorumLossProgress(
                body.OperationId.AsGUID(),
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
                thisSPtr);

            this->OnDispatchComplete(operation, true);
        }
        else
        {
            WriteInfo(TraceComponent, "GetInvokeQuorumLossProgress/ {0} failed to get process incoming request: {1}", this->TraceId, this->Request.MessageId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        ErrorCode error = this->Service.EndGetPartitionQuorumLossProgress(operation, &progressResult_);

        if (!error.IsSuccess())
        {
            return error;
        }

        const FABRIC_PARTITION_QUORUM_LOSS_PROGRESS * publicProgress = progressResult_->get_Progress();
        InvokeQuorumLossProgress progress;
        error = progress.FromPublicApi(*publicProgress);

        if (!error.IsSuccess())
        {
            return error;
        }
        else
        {
            InvokeQuorumLossProgress invokeQuorumLossProgress(move(progress));
            GetInvokeQuorumLossProgressReplyMessageBody replyBody(move(invokeQuorumLossProgress));
            reply = FaultAnalysisServiceMessage::GetClientOperationSuccess(replyBody);
        }

        return error;
    }

    IFabricPartitionQuorumLossProgressResult* progressResult_;
};

// RestartPartition
class FaultAnalysisServiceAgent::RestartPartitionAsyncOperation : public DispatchMessageAsyncOperationBase
{
public:
    RestartPartitionAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IFaultAnalysisService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~RestartPartitionAsyncOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        RestartPartitionMessageBody body;
        if (this->Request.GetBody(body))
        {
            ScopedHeap heap;
            FABRIC_START_PARTITION_RESTART_DESCRIPTION publicDescription = { 0 };

            body.Description.ToPublicApi(heap, publicDescription);

            RestartPartitionDescription internalDescription;
            auto error = internalDescription.FromPublicApi(publicDescription);

            if (!error.IsSuccess())
            {
                Trace.WriteWarning(TraceComponent, "Agent/RestartPartition/Could not convert back");
            }

            auto operation = this->Service.BeginStartPartitionRestart(
                &publicDescription,
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
                thisSPtr);
            this->OnDispatchComplete(operation, true);
        }
        else
        {
            WriteInfo(TraceComponent, "{0} failed to get process incoming request: {1}", this->TraceId, this->Request.MessageId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        ErrorCode error = this->Service.EndStartPartitionRestart(operation);

        if (error.IsSuccess())
        {
            reply = FaultAnalysisServiceMessage::GetClientOperationSuccess();
        }

        return error;
    }
};

class FaultAnalysisServiceAgent::GetRestartPartitionProgressAsyncOperation : public DispatchMessageAsyncOperationBase
{
public:
    GetRestartPartitionProgressAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IFaultAnalysisService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~GetRestartPartitionProgressAsyncOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        GetRestartPartitionProgressMessageBody body;
        if (this->Request.GetBody(body))
        {
            auto operation = this->Service.BeginGetPartitionRestartProgress(
                body.OperationId.AsGUID(),
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
                thisSPtr);

            this->OnDispatchComplete(operation, true);
        }
        else
        {
            WriteInfo(TraceComponent, "GetRestartPartitionProgress/ {0} failed to get process incoming request: {1}", this->TraceId, this->Request.MessageId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        ErrorCode error = this->Service.EndGetPartitionRestartProgress(operation, &progressResult_);

        if (!error.IsSuccess())
        {
            return error;
        }

        const FABRIC_PARTITION_RESTART_PROGRESS * publicProgress = progressResult_->get_Progress();
        RestartPartitionProgress progress;
        error = progress.FromPublicApi(*publicProgress);

        if (!error.IsSuccess())
        {
            return error;
        }
        else
        {
            RestartPartitionProgress restartPartitionProgress(move(progress));
            GetRestartPartitionProgressReplyMessageBody replyBody(move(restartPartitionProgress));
            reply = FaultAnalysisServiceMessage::GetClientOperationSuccess(replyBody);
        }

        return error;
    }

    IFabricPartitionRestartProgressResult* progressResult_;
};



class FaultAnalysisServiceAgent::GetNodeTransitionProgressAsyncOperation : public DispatchMessageAsyncOperationBase
{
public:
    GetNodeTransitionProgressAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IFaultAnalysisService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~GetNodeTransitionProgressAsyncOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        Trace.WriteInfo(
            TraceComponent,
            "FaultAnalysisServiceAgent::GetNodeTransitionProgressAsyncOperation::OnStart");

        GetNodeTransitionProgressMessageBody body;
        if (this->Request.GetBody(body))
        {
            auto operation = this->Service.BeginGetNodeTransitionProgress(
                body.OperationId.AsGUID(),
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
                thisSPtr);

            this->OnDispatchComplete(operation, true);
        }
        else
        {
            WriteInfo(TraceComponent, "BeginGetNodeTransitionProgress {0} failed to get process incoming request: {1}", this->TraceId, this->Request.MessageId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        Trace.WriteInfo(
            TraceComponent,
            "FaultAnalysisServiceAgent::GetNodeTransitionProgressAsyncOperation::EndDispatch");

        ErrorCode error = this->Service.EndGetNodeTransitionProgress(operation, &progressResult_);

        Trace.WriteInfo(
            TraceComponent,
            "FaultAnalysisServiceAgent::GetNodeSTransitionProgressAsyncOperation::EndDispatch error is {0}",
            error);

        Trace.WriteInfo(
            TraceComponent,
            "FaultAnalysisServiceAgent::GetNodeTransitionProgressAsyncOperation::EndDispatch progressResult_ is null? '{0}'",
            progressResult_ == NULL ? true : false);

        if (!error.IsSuccess())
        {
            return error;
        }

        const FABRIC_NODE_TRANSITION_PROGRESS * publicProgress = progressResult_->get_Progress();

        Trace.WriteInfo(
            TraceComponent,
            "FaultAnalysisServiceAgent::GetNodeTransitionProgressAsyncOperation::EndDispatch after progressResult_->get_Progress"
            );
        NodeTransitionProgress progress;
        error = progress.FromPublicApi(*publicProgress);

        Trace.WriteInfo(
            TraceComponent,
            "FaultAnalysisServiceAgent::GetNodeTransitionProgressAsyncOperation::EndDispatch FromPublicApi returned {0}",
            error);

        if (!error.IsSuccess())
        {
            return error;
        }
        else
        {
            NodeTransitionProgress nodeOperationProgress(move(progress));
            GetNodeTransitionProgressReplyMessageBody replyBody(move(nodeOperationProgress));
            reply = FaultAnalysisServiceMessage::GetClientOperationSuccess(replyBody);
        }

        return error;
    }

    IFabricNodeTransitionProgressResult* progressResult_;
};


// Chaos

class FaultAnalysisServiceAgent::StartChaosAsyncOperation : public DispatchMessageAsyncOperationBase
{
public:
    StartChaosAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IFaultAnalysisService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~StartChaosAsyncOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        StartChaosMessageBody body;
        if (this->Request.GetBody(body))
        {
            ScopedHeap heap;
            FABRIC_START_CHAOS_DESCRIPTION publicDescription = { 0 };

            body.ChaosDescription.ToPublicApi(heap, publicDescription);

            StartChaosDescription internalDescription;
            auto error = internalDescription.FromPublicApi(publicDescription);

            if (!error.IsSuccess())
            {
                Trace.WriteWarning(TraceComponent, "Agent/StartChaos/Could not convert back");
                this->TryComplete(thisSPtr, ErrorCodeValue::InvalidArgument);
            }

            auto operation = this->Service.BeginStartChaos(
                &publicDescription,
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
                thisSPtr);
            this->OnDispatchComplete(operation, true);
        }
        else
        {
            WriteInfo(TraceComponent, "{0} failed to get process incoming request: {1}", this->TraceId, this->Request.MessageId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        auto error = this->Service.EndStartChaos(operation);

        if (error.IsSuccess())
        {
            reply = FaultAnalysisServiceMessage::GetClientOperationSuccess();
        }

        return error;
    }
};

class FaultAnalysisServiceAgent::StopChaosAsyncOperation : public DispatchMessageAsyncOperationBase
{
public:
    StopChaosAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IFaultAnalysisService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~StopChaosAsyncOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = this->Service.BeginStopChaos(
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
            thisSPtr);
        this->OnDispatchComplete(operation, true);
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        auto error = this->Service.EndStopChaos(operation);

        if (error.IsSuccess())
        {
            reply = FaultAnalysisServiceMessage::GetClientOperationSuccess();
        }

        return error;
    }
};

class FaultAnalysisServiceAgent::GetChaosReportAsyncOperation : public DispatchMessageAsyncOperationBase
{
public:
    GetChaosReportAsyncOperation(
        Federation::NodeInstance const & nodeInstance,
        __in IFaultAnalysisService & service,
        MessageUPtr && message,
        IpcReceiverContextUPtr && receiverContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DispatchMessageAsyncOperationBase(nodeInstance, service, move(message), move(receiverContext), timeout, callback, parent)
    {
    }

    virtual ~GetChaosReportAsyncOperation() { }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        GetChaosReportMessageBody body;
        if (this->Request.GetBody(body))
        {
            ScopedHeap heap;
            FABRIC_GET_CHAOS_REPORT_DESCRIPTION publicDescription = {0};
            body.ReportDescription.ToPublicApi(heap, publicDescription);

            auto operation = this->Service.BeginGetChaosReport(
                &publicDescription,
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnDispatchComplete(operation, false); },
                thisSPtr);
            this->OnDispatchComplete(operation, true);
        }
        else
        {
            WriteInfo(TraceComponent, "GetChaosReport/ {0} failed to get process incoming request: {1}", this->TraceId, this->Request.MessageId);
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        }
    }

    ErrorCode EndDispatch(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        ErrorCode error = this->Service.EndGetChaosReport(operation, &reportResult_);

        if (!error.IsSuccess())
        {
            WriteWarning(TraceComponent, "Failed at EndGetChaosReport.");
            return error;
        }

        const FABRIC_CHAOS_REPORT * publicReport = reportResult_->get_ChaosReportResult();

        ChaosReport report;
        error = report.FromPublicApi(*publicReport);

        if (!error.IsSuccess())
        {
            WriteWarning(TraceComponent, "Failed to execute FromPublicApi on ChaosReport.");
            return error;
        }
        else
        {
            ChaosReport chaosReport(move(report));
            GetChaosReportReplyMessageBody replyBody(move(chaosReport));
            reply = FaultAnalysisServiceMessage::GetClientOperationSuccess(replyBody);
        }

        return error;
    }

    IFabricChaosReportResult* reportResult_;
};

//------------------------------------------------------------------------------
// FaultAnalysisServiceAgent
//

FaultAnalysisServiceAgent::FaultAnalysisServiceAgent(
    Federation::NodeInstance const & nodeInstance,
    __in IpcClient & ipcClient,
    ComPointer<IFabricRuntime> const & runtimeCPtr)
    : IFaultAnalysisServiceAgent()
    , ComponentRoot()
    , NodeTraceComponent(nodeInstance)
    , routingAgentProxyUPtr_()
    , registeredServiceLocations_()
    , lock_()
    , runtimeCPtr_(runtimeCPtr)
    , queryMessageHandler_(nullptr)
    , servicePtr_()
{
    WriteInfo(TraceComponent, "{0} ctor", this->TraceId);

    auto temp = ServiceRoutingAgentProxy::Create(nodeInstance, ipcClient, *this);
    routingAgentProxyUPtr_.swap(temp);
}

FaultAnalysisServiceAgent::~FaultAnalysisServiceAgent()
{
    WriteInfo(TraceComponent, "{0} ~dtor", this->TraceId);
}

ErrorCode FaultAnalysisServiceAgent::Create(__out shared_ptr<FaultAnalysisServiceAgent> & result)
{
    ComPointer<IFabricRuntime> runtimeCPtr;

    HRESULT hr = ::FabricCreateRuntime(
        IID_IFabricRuntime,
        runtimeCPtr.VoidInitializationAddress());
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    ComFabricRuntime * castedRuntimePtr;
    try
    {
#if !defined(PLATFORM_UNIX)
        castedRuntimePtr = dynamic_cast<ComFabricRuntime*>(runtimeCPtr.GetRawPointer());
#else        
        castedRuntimePtr = (ComFabricRuntime*)(runtimeCPtr.GetRawPointer());
#endif
    }
    catch (...)
    {
        Assert::TestAssert("TSAgent unable to get ComfabricRuntime");
        return ErrorCodeValue::OperationFailed;
    }

    FabricNodeContextSPtr fabricNodeContext;
    auto error = castedRuntimePtr->Runtime->Host.GetFabricNodeContext(fabricNodeContext);
    if (!error.IsSuccess())
    {
        return error;
    }

    NodeId nodeId;
    if (!NodeId::TryParse(fabricNodeContext->NodeId, nodeId))
    {
        Assert::TestAssert("Could not parse NodeId {0}", fabricNodeContext->NodeId);
        return ErrorCodeValue::OperationFailed;
    }

    uint64 nodeInstanceId = fabricNodeContext->NodeInstanceId;
    Federation::NodeInstance nodeInstance(nodeId, nodeInstanceId);

    IpcClient & ipcClient = castedRuntimePtr->Runtime->Host.Client;

    shared_ptr<FaultAnalysisServiceAgent> agentSPtr(new FaultAnalysisServiceAgent(nodeInstance, ipcClient, runtimeCPtr));

    error = agentSPtr->Initialize();

    if (error.IsSuccess())
    {
        Trace.WriteInfo(TraceComponent, "Successfully Created FaultAnalysisServiceAgent");
        result.swap(agentSPtr);
    }

    return error;
}

ErrorCode FaultAnalysisServiceAgent::Initialize()
{
    return routingAgentProxyUPtr_->Open();
}

void FaultAnalysisServiceAgent::Release()
{
    routingAgentProxyUPtr_->Close().ReadValue();
}

void FaultAnalysisServiceAgent::RegisterFaultAnalysisService(
    ::FABRIC_PARTITION_ID partitionId,
    ::FABRIC_REPLICA_ID replicaId,
    IFaultAnalysisServicePtr const & rootedService,
    __out wstring & serviceAddress)
{
    SystemServiceLocation serviceLocation(
        routingAgentProxyUPtr_->NodeInstance,
        Guid(partitionId),
        replicaId,
        DateTime::Now().Ticks);
    this->ReplaceServiceLocation(serviceLocation);

    
    auto selfRoot = this->CreateComponentRoot();

    // Create and register the query handlers
    queryMessageHandler_ = make_unique<QueryMessageHandler>(*selfRoot, QueryAddresses::TSAddressSegment);
    queryMessageHandler_->RegisterQueryHandler(
     [this](QueryNames::Enum queryName, QueryArgumentMap const & queryArgs, Common::ActivityId const & activityId, TimeSpan timeout, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
    {
     return this->BeginProcessQuery(queryName, queryArgs, activityId, timeout, callback, parent);
    },
     [this](Common::AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
     return this->EndProcessQuery(operation, reply);
    });

    auto error = queryMessageHandler_->Open();
    if (!error.IsSuccess())
    {
     WriteWarning(
         TraceComponent,
         "{0} QueryMessageHandler failed to open with error {1}",
         this->TraceId,
         error);
    }

    servicePtr_ = rootedService;
    
    routingAgentProxyUPtr_->RegisterMessageHandler(
        serviceLocation,
        [this, rootedService](MessageUPtr & message, IpcReceiverContextUPtr & receiverContext)
    {
        this->DispatchMessage(rootedService, message, receiverContext);
    });

    serviceAddress = wformatString("{0}", serviceLocation);
}

void FaultAnalysisServiceAgent::UnregisterFaultAnalysisService(
    ::FABRIC_PARTITION_ID partitionId,
    ::FABRIC_REPLICA_ID replicaId)
{
    SystemServiceLocation serviceLocation(
        routingAgentProxyUPtr_->NodeInstance,
        Guid(partitionId),
        replicaId,
        DateTime::Now().Ticks);

    {
        AcquireExclusiveLock lock(lock_);

        auto findIt = find_if(registeredServiceLocations_.begin(), registeredServiceLocations_.end(),
            [&serviceLocation](SystemServiceLocation const & it) { return serviceLocation.EqualsIgnoreInstances(it); });

        if (findIt != registeredServiceLocations_.end())
        {
            serviceLocation = *findIt;

            registeredServiceLocations_.erase(findIt);
        }
    }

    // Expect to find location in our own list, but calling UnregisterMessageHandler()
    // is harmless even if we did not find it
    //
    routingAgentProxyUPtr_->UnregisterMessageHandler(serviceLocation);

    
    if (queryMessageHandler_)
    {
      queryMessageHandler_->Close();
    }
}

void FaultAnalysisServiceAgent::ReplaceServiceLocation(SystemServiceLocation const & serviceLocation)
{
    // If there is already a handler registration for this partition+replica, make sure its handler is
    // unregistered from the routing agent proxy to be more robust to re-opened replicas
    // that may not have cleanly unregistered (e.g. rudely aborted).
    //
    bool foundExisting = false;
    SystemServiceLocation existingLocation;
    {
        AcquireExclusiveLock lock(lock_);

        auto findIt = find_if(registeredServiceLocations_.begin(), registeredServiceLocations_.end(),
            [&serviceLocation](SystemServiceLocation const & it) { return serviceLocation.EqualsIgnoreInstances(it); });

        if (findIt != registeredServiceLocations_.end())
        {
            foundExisting = true;
            existingLocation = *findIt;

            registeredServiceLocations_.erase(findIt);
        }

        registeredServiceLocations_.push_back(serviceLocation);
    }

    if (foundExisting)
    {
        routingAgentProxyUPtr_->UnregisterMessageHandler(existingLocation);
    }
}


TimeSpan FaultAnalysisServiceAgent::GetRandomizedOperationRetryDelay(ErrorCode const & error) const
{
    return StoreTransaction::GetRandomizedOperationRetryDelay(
        error,
        ManagementConfig::GetConfig().MaxOperationRetryDelay);
}

AsyncOperationSPtr FaultAnalysisServiceAgent::BeginProcessQuery(
    QueryNames::Enum queryName,
    QueryArgumentMap const & queryArgs,
    ActivityId const & activityId,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    Trace.WriteInfo(TraceComponent, "Inside agent/BeginProcessQuery");
    return AsyncOperation::CreateAndStart<ProcessQueryAsyncOperation>(
        *this,
        queryName,
        queryArgs,
        activityId,
        timeout,
        callback,
        parent);
}

ErrorCode FaultAnalysisServiceAgent::EndProcessQuery(
    AsyncOperationSPtr const & operation,
    Transport::MessageUPtr & reply)
{
    return FaultAnalysisService::ProcessQueryAsyncOperation::End(operation, reply);
}

AsyncOperationSPtr FaultAnalysisServiceAgent::BeginSendQueryToService(
    FABRIC_TEST_COMMAND_STATE_FILTER stateFilter,
    FABRIC_TEST_COMMAND_TYPE_FILTER typeFilter,
    ActivityId const &,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    FABRIC_TEST_COMMAND_LIST_DESCRIPTION commandListDescription = {};

    commandListDescription.TestCommandStateFilter = stateFilter;
    commandListDescription.TestCommandTypeFilter = typeFilter;
    
    return (*servicePtr_.get()).BeginGetTestCommandStatusList(
        &commandListDescription,
        timeout,
        callback,
        parent);
}

ErrorCode FaultAnalysisServiceAgent::EndSendQueryToService(
    AsyncOperationSPtr const & operation,
    MessageUPtr & realReply)
{
    IFabricTestCommandStatusResult * commandQueryResult;
    ErrorCode error = (*servicePtr_.get()).EndGetTestCommandStatusList(operation, &commandQueryResult); 
    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "FaultAnalysisServiceAgent::EndSendQueryToService failed with {0}", error);
        return error;
    }

    const TEST_COMMAND_QUERY_RESULT_LIST * resultObj = commandQueryResult->get_Result();

    vector<TestCommandListQueryResult> resultList;
    TEST_COMMAND_QUERY_RESULT_ITEM * item = (TEST_COMMAND_QUERY_RESULT_ITEM *)resultObj->Items;
    for (uint i = 0; i < resultObj->Count; i++, ++item)
    {  
        resultList.push_back(TestCommandListQueryResult(Guid(item->OperationId), (FABRIC_TEST_COMMAND_PROGRESS_STATE)item->TestCommandState, (FABRIC_TEST_COMMAND_TYPE)item->TestCommandType));
    }   
    
    QueryResult queryResult = ServiceModel::QueryResult(move(resultList));

    realReply = FaultAnalysisServiceMessage::GetQueryReply(move(queryResult));
    return queryResult.QueryProcessingError;
}

AsyncOperationSPtr FaultAnalysisServiceAgent::BeginGetStoppedNodeList(
    ActivityId const & activityId,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(activityId);

    return (*servicePtr_.get()).BeginGetStoppedNodeList(
        //        &commandListDescription,
        timeout,
        callback,
        parent);
}

ErrorCode FaultAnalysisServiceAgent::EndGetStoppedNodeList(
    AsyncOperationSPtr const & operation,
    MessageUPtr & realReply)
{
    wstring result;

    Trace.WriteInfo(TraceComponent, "FaultAnalysisServiceAgent - Calling EndGetStoppedNodeList");

    ErrorCode error = (*servicePtr_.get()).EndGetStoppedNodeList(operation, result);

    Trace.WriteInfo(TraceComponent, "FaultAnalysisServiceAgent - EndGetStoppedNodeList return error='{0}'", error);

    Trace.WriteInfo(TraceComponent, "string returned from FAS='{0}'", result);
    ListPager<NodeQueryResult> nodeQueryResultList;
    Federation::NodeId nodeId;

    StringCollection stoppedNodes;
    StringUtility::Split<wstring>(result, stoppedNodes, L";");
    for (int i = 0; i < (int)stoppedNodes.size(); i++)
    {
        Trace.WriteInfo(TraceComponent, "  string parsed '{0}'", stoppedNodes[i]);
        error = nodeQueryResultList.TryAdd(NodeQueryResult(stoppedNodes[i], L"", L"", false, L"", L"", NodeId()));

        if (!error.IsSuccess())
        {
            Trace.WriteWarning(TraceComponent, "Error getting nodeId for nodeName {0}", error);
            return error;
        }
    }

    QueryResult queryResult = ServiceModel::QueryResult(move(nodeQueryResultList));

    realReply = FaultAnalysisServiceMessage::GetQueryReply(move(queryResult));
    return queryResult.QueryProcessingError;
}

void FaultAnalysisServiceAgent::DispatchMessage(
    IFaultAnalysisServicePtr const & servicePtr,
    __in Transport::MessageUPtr & message,
    __in Transport::IpcReceiverContextUPtr & receiverContext)
{
     if (message->Actor == Actor::FAS)
     {
        TimeoutHeader timeoutHeader;
        if (!message->Headers.TryReadFirst(timeoutHeader))
        {
            WriteInfo(TraceComponent, "{0} missing TimeoutHeader: message Id = {1}", this->TraceId, message->MessageId);
            routingAgentProxyUPtr_->OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext);
            return;
        }

        vector<ComponentRootSPtr> roots;
        roots.push_back(servicePtr.get_Root());
        roots.push_back(this->CreateComponentRoot());

        if (message->Action == FaultAnalysisServiceTcpMessage::StartPartitionDataLossAction)
        {
            auto operation = AsyncOperation::CreateAndStart<InvokeDataLossAsyncOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == FaultAnalysisServiceTcpMessage::GetPartitionDataLossProgressAction)
        {
            auto operation = AsyncOperation::CreateAndStart<GetInvokeDataLossProgressAsyncOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == FaultAnalysisServiceTcpMessage::StartPartitionQuorumLossAction)
        {
            auto operation = AsyncOperation::CreateAndStart<InvokeQuorumLossAsyncOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == FaultAnalysisServiceTcpMessage::GetPartitionQuorumLossProgressAction)
        {
            auto operation = AsyncOperation::CreateAndStart<GetInvokeQuorumLossProgressAsyncOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == FaultAnalysisServiceTcpMessage::StartPartitionRestartAction)
        {
            auto operation = AsyncOperation::CreateAndStart<RestartPartitionAsyncOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == FaultAnalysisServiceTcpMessage::GetPartitionRestartProgressAction)
        {
            auto operation = AsyncOperation::CreateAndStart<GetRestartPartitionProgressAsyncOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }       
        else if (message->Action == FaultAnalysisServiceTcpMessage::CancelTestCommandAction)
        {
            auto operation = AsyncOperation::CreateAndStart<CancelTestCommandAsyncOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == QueryTcpMessage::QueryAction)
        {                  
            auto operation = AsyncOperation::CreateAndStart<FaultAnalysisServiceAgentQueryMessageHandler>(
              *this,
              queryMessageHandler_,
              move(message),
              move(receiverContext),
              timeoutHeader.Timeout,
              [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete2(operation, false); },
              ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));

            this->OnDispatchMessageComplete2(operation, true);
        }
        else if (message->Action == FaultAnalysisServiceTcpMessage::StartChaosAction)
        {
            auto operation = AsyncOperation::CreateAndStart<StartChaosAsyncOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == FaultAnalysisServiceTcpMessage::StopChaosAction)
        {
            auto operation = AsyncOperation::CreateAndStart<StopChaosAsyncOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == FaultAnalysisServiceTcpMessage::GetChaosReportAction)
        {
            auto operation = AsyncOperation::CreateAndStart<GetChaosReportAsyncOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == FaultAnalysisServiceTcpMessage::StartNodeTransitionAction)
        {
            auto operation = AsyncOperation::CreateAndStart<StartNodeTransitionAsyncOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
        else if (message->Action == FaultAnalysisServiceTcpMessage::GetNodeTransitionProgressAction)
        {
            auto operation = AsyncOperation::CreateAndStart<GetNodeTransitionProgressAsyncOperation>(
                routingAgentProxyUPtr_->NodeInstance,
                *servicePtr.get(),
                move(message),
                move(receiverContext),
                timeoutHeader.Timeout,
                [this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
                ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
            this->OnDispatchMessageComplete(operation, true);
        }
		else
		{
			auto operation = AsyncOperation::CreateAndStart<DispatchMessageAsyncOperationBase>(
				routingAgentProxyUPtr_->NodeInstance,
				*servicePtr.get(),
				move(message),
				move(receiverContext),
				timeoutHeader.Timeout,
				[this](AsyncOperationSPtr const & operation) { return this->OnDispatchMessageComplete(operation, false); },
				ComponentRoot::CreateAsyncOperationMultiRoot(move(roots)));
			this->OnDispatchMessageComplete(operation, true);
		}
    }
    else
    {
        Trace.WriteWarning(TraceComponent, "{0} invalid Actor {1}: message Id = {2}", this->TraceId, message->Actor, message->MessageId);
        routingAgentProxyUPtr_->OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext);
    }
}

void FaultAnalysisServiceAgent::OnDispatchMessageComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    MessageUPtr reply;
    IpcReceiverContextUPtr receiverContext;

    ErrorCode error = DispatchMessageAsyncOperationBase::End(operation, reply, receiverContext);

    if (error.IsSuccess())
    {
        routingAgentProxyUPtr_->SendIpcReply(move(reply), *receiverContext);
    }
    else
    {
        routingAgentProxyUPtr_->OnIpcFailure(error, *receiverContext);
    }
}

void FaultAnalysisServiceAgent::OnDispatchMessageComplete2(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    MessageUPtr reply;
    IpcReceiverContextUPtr receiverContext;
    ErrorCode error = ClientRequestAsyncOperation::End(operation, reply, receiverContext);

    if (error.IsSuccess())
    {
        routingAgentProxyUPtr_->SendIpcReply(move(reply), *receiverContext);
    }
    else
    {
        routingAgentProxyUPtr_->OnIpcFailure(error, *receiverContext);
    }
}


