// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace ServiceModel;
using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("ComFabricClient");

// ********************************************************************************************************************
// ComProxyAsyncOperation Classes
//

class ComProxyFaultAnalysisService::InvokeDataLossAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(InvokeDataLossAsyncOperation)

public:
    InvokeDataLossAsyncOperation(
        __in IFabricFaultAnalysisService & comImpl,
        __in FABRIC_START_PARTITION_DATA_LOSS_DESCRIPTION * invokeDataLossDescription,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , invokeDataLossDescription_(invokeDataLossDescription)
        , timeout_(timeout)
    {
    }

    virtual ~InvokeDataLossAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<InvokeDataLossAsyncOperation>(operation);

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        InvokeDataLossDescription internalDescription;
        auto error = internalDescription.FromPublicApi(*invokeDataLossDescription_);

            if (!error.IsSuccess())
            {
                Trace.WriteWarning(TraceComponent, "ProxyService/DataLoss/Could not convert back");
                return error.ToHResult();
            }

        return comImpl_.BeginStartPartitionDataLoss(
                invokeDataLossDescription_,
                this->GetDWORDTimeout(),
                callback,
                context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndStartPartitionDataLoss(context);
    }

    IFabricFaultAnalysisService & comImpl_;
    FABRIC_START_PARTITION_DATA_LOSS_DESCRIPTION * invokeDataLossDescription_;
    TimeSpan const timeout_;
};

class ComProxyFaultAnalysisService::GetInvokeDataLossProgressAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(GetInvokeDataLossProgressAsyncOperation)

public:
    GetInvokeDataLossProgressAsyncOperation(
        __in IFabricFaultAnalysisService & comImpl,
        __in FABRIC_TEST_COMMAND_OPERATION_ID operationId,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , operationId_(operationId)
        , timeout_(timeout)
    {
    }

    virtual ~GetInvokeDataLossProgressAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation, IFabricPartitionDataLossProgressResult ** reply)
    {
        auto casted = AsyncOperation::End<GetInvokeDataLossProgressAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            *reply = move(casted->result);
        }

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginGetPartitionDataLossProgress(
                operationId_,
                this->GetDWORDTimeout(),
                callback,
                context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndGetPartitionDataLossProgress(context, (IFabricPartitionDataLossProgressResult**)&result);

    }

    IFabricFaultAnalysisService & comImpl_;
    FABRIC_TEST_COMMAND_OPERATION_ID operationId_;
    IFabricPartitionDataLossProgressResult * result;
    TimeSpan const timeout_;
};

class ComProxyFaultAnalysisService::InvokeQuorumLossAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(InvokeQuorumLossAsyncOperation)

public:
    InvokeQuorumLossAsyncOperation(
        __in IFabricFaultAnalysisService & comImpl,
        __in FABRIC_START_PARTITION_QUORUM_LOSS_DESCRIPTION * invokeQuorumLossDescription,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , invokeQuorumLossDescription_(invokeQuorumLossDescription)
        , timeout_(timeout)
    {
    }

    virtual ~InvokeQuorumLossAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<InvokeQuorumLossAsyncOperation>(operation);

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        InvokeQuorumLossDescription internalDescription;
        auto error = internalDescription.FromPublicApi(*invokeQuorumLossDescription_);

        if (!error.IsSuccess())
        {
            Trace.WriteWarning(TraceComponent, "ProxyService/QuorumLoss/Could not convert back");
            return error.ToHResult();
        }

        return comImpl_.BeginStartPartitionQuorumLoss(
                invokeQuorumLossDescription_,
                this->GetDWORDTimeout(),
                callback,
                context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndStartPartitionQuorumLoss(context);
    }

    IFabricFaultAnalysisService & comImpl_;
    FABRIC_START_PARTITION_QUORUM_LOSS_DESCRIPTION * invokeQuorumLossDescription_;
    TimeSpan const timeout_;
};

class ComProxyFaultAnalysisService::GetInvokeQuorumLossProgressAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(GetInvokeQuorumLossProgressAsyncOperation)

public:
    GetInvokeQuorumLossProgressAsyncOperation(
        __in IFabricFaultAnalysisService & comImpl,
        __in FABRIC_TEST_COMMAND_OPERATION_ID operationId,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , operationId_(operationId)
        , timeout_(timeout)
    {
    }

    virtual ~GetInvokeQuorumLossProgressAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation, IFabricPartitionQuorumLossProgressResult ** reply)
    {
        auto casted = AsyncOperation::End<GetInvokeQuorumLossProgressAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            *reply = move(casted->result);
        }

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginGetPartitionQuorumLossProgress(
                operationId_,
                this->GetDWORDTimeout(),
                callback,
                context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndGetPartitionQuorumLossProgress(context, (IFabricPartitionQuorumLossProgressResult**)&result);
    }

    IFabricFaultAnalysisService & comImpl_;
    FABRIC_TEST_COMMAND_OPERATION_ID operationId_;
    IFabricPartitionQuorumLossProgressResult * result;
    TimeSpan const timeout_;
};

class ComProxyFaultAnalysisService::RestartPartitionAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(RestartPartitionAsyncOperation)

public:
    RestartPartitionAsyncOperation(
        __in IFabricFaultAnalysisService & comImpl,
        __in FABRIC_START_PARTITION_RESTART_DESCRIPTION * restartPartitionDescription,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , restartPartitionDescription_(restartPartitionDescription)
        , timeout_(timeout)
    {
    }

    virtual ~RestartPartitionAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<RestartPartitionAsyncOperation>(operation);

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        RestartPartitionDescription internalDescription;
        auto error = internalDescription.FromPublicApi(*restartPartitionDescription_);

        if (!error.IsSuccess())
        {
            Trace.WriteWarning(TraceComponent, "ProxyService/RestartPartition/Could not convert back");
            return error.ToHResult();
        }

        return comImpl_.BeginStartPartitionRestart(
            restartPartitionDescription_,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndStartPartitionRestart(context);
    }

    IFabricFaultAnalysisService & comImpl_;
    FABRIC_START_PARTITION_RESTART_DESCRIPTION * restartPartitionDescription_;
    TimeSpan const timeout_;
};

class ComProxyFaultAnalysisService::GetRestartPartitionProgressAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(GetRestartPartitionProgressAsyncOperation)

public:
    GetRestartPartitionProgressAsyncOperation(
        __in IFabricFaultAnalysisService & comImpl,
        __in FABRIC_TEST_COMMAND_OPERATION_ID operationId,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , operationId_(operationId)
        , timeout_(timeout)
    {
    }

    virtual ~GetRestartPartitionProgressAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation, IFabricPartitionRestartProgressResult ** reply)
    {
        auto casted = AsyncOperation::End<GetRestartPartitionProgressAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            *reply = move(casted->result);
        }

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginGetPartitionRestartProgress(
            operationId_,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndGetPartitionRestartProgress(context, (IFabricPartitionRestartProgressResult**)&result);
    }

    IFabricFaultAnalysisService & comImpl_;
    FABRIC_TEST_COMMAND_OPERATION_ID operationId_;
    IFabricPartitionRestartProgressResult * result;
    TimeSpan const timeout_;
};

class ComProxyFaultAnalysisService::GetTestCommandStatusAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(GetTestCommandStatusAsyncOperation)

public:
    GetTestCommandStatusAsyncOperation(
        __in IFabricFaultAnalysisService & comImpl,
        __in FABRIC_TEST_COMMAND_LIST_DESCRIPTION * description,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , description_(description)
        , timeout_(timeout)
    {
    }

    virtual ~GetTestCommandStatusAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation, IFabricTestCommandStatusResult ** reply)

    {
        auto casted = AsyncOperation::End<GetTestCommandStatusAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            *reply = move(casted->result_);
        }

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginGetTestCommandStatusList(
            description_,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndGetTestCommandStatusList(context, (IFabricTestCommandStatusResult**)&result_);
    }

    IFabricFaultAnalysisService & comImpl_;
    FABRIC_TEST_COMMAND_LIST_DESCRIPTION * description_;

    IFabricTestCommandStatusResult * result_;

    TimeSpan const timeout_;
};

class ComProxyFaultAnalysisService::CancelTestCommandAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(CancelTestCommandAsyncOperation)

public:
    CancelTestCommandAsyncOperation(
        __in IFabricFaultAnalysisService & comImpl,
        __in FABRIC_CANCEL_TEST_COMMAND_DESCRIPTION* description,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , description_(description)
        , timeout_(timeout)
    {
    }

    virtual ~CancelTestCommandAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<CancelTestCommandAsyncOperation>(operation);

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginCancelTestCommand(
            description_,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndCancelTestCommand(context);
    }

    IFabricFaultAnalysisService & comImpl_;
    FABRIC_CANCEL_TEST_COMMAND_DESCRIPTION* description_;
    TimeSpan const timeout_;
};

// Chaos

class ComProxyFaultAnalysisService::StartChaosAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(StartChaosAsyncOperation)

public:
    StartChaosAsyncOperation(
        __in IFabricFaultAnalysisService & comImpl,
        __in FABRIC_START_CHAOS_DESCRIPTION * startChaosDescription,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , startChaosDescription_(startChaosDescription)
        , timeout_(timeout)
    {
    }

    virtual ~StartChaosAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<StartChaosAsyncOperation>(operation);
        return casted->Error;
    }

    protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        StartChaosDescription internalDescription;
        auto error = internalDescription.FromPublicApi(*startChaosDescription_);

        if (!error.IsSuccess())
        {
            Trace.WriteWarning(TraceComponent, "ProxyService/StartChaos/Could not convert back");
            return error.ToHResult();
        }

        return comImpl_.BeginStartChaos(
            startChaosDescription_,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndStartChaos(context);
    }

    IFabricFaultAnalysisService & comImpl_;
    FABRIC_START_CHAOS_DESCRIPTION * startChaosDescription_;
    TimeSpan const timeout_;
};

class ComProxyFaultAnalysisService::StopChaosAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(StopChaosAsyncOperation)

public:
    StopChaosAsyncOperation(
        __in IFabricFaultAnalysisService & comImpl,
                TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , timeout_(timeout)
    {
    }

    virtual ~StopChaosAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<StopChaosAsyncOperation>(operation);
        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginStopChaos(
            this->GetDWORDTimeout(),
                callback,
                context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        ;      return comImpl_.EndStopChaos(context);
    }

    IFabricFaultAnalysisService & comImpl_;
    TimeSpan const timeout_;
};

class ComProxyFaultAnalysisService::GetChaosReportAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(GetChaosReportAsyncOperation)

public:
    GetChaosReportAsyncOperation(
        __in IFabricFaultAnalysisService & comImpl,
        __in FABRIC_GET_CHAOS_REPORT_DESCRIPTION * getChaosReportDescription,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , getChaosReportDescription_(getChaosReportDescription)
        , timeout_(timeout)
    {
    }

    virtual ~GetChaosReportAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation, IFabricChaosReportResult ** reply)
    {
        auto casted = AsyncOperation::End<GetChaosReportAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            *reply = move(casted->result);
        }

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        GetChaosReportDescription internalDescription;
        auto error = internalDescription.FromPublicApi(*getChaosReportDescription_);

        if (!error.IsSuccess())
        {
            Trace.WriteWarning(TraceComponent, "ProxyService/GetChaosReport/Could not convert back");
            return error.ToHResult();
        }

        return comImpl_.BeginGetChaosReport(
            getChaosReportDescription_,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndGetChaosReport(context, (IFabricChaosReportResult**)&result);
    }

    IFabricFaultAnalysisService & comImpl_;
    FABRIC_GET_CHAOS_REPORT_DESCRIPTION * getChaosReportDescription_;
    IFabricChaosReportResult * result;
    TimeSpan const timeout_;
};

class ComProxyFaultAnalysisService::GetNodeTransitionProgressAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(GetNodeTransitionProgressAsyncOperation)

public:
    GetNodeTransitionProgressAsyncOperation(
        __in IFabricFaultAnalysisService & comImpl,
        __in FABRIC_TEST_COMMAND_OPERATION_ID operationId,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , operationId_(operationId)
        , timeout_(timeout)
    {
    }

    virtual ~GetNodeTransitionProgressAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation, IFabricNodeTransitionProgressResult ** reply)
    {
        auto casted = AsyncOperation::End<GetNodeTransitionProgressAsyncOperation>(operation);

        Trace.WriteWarning(TraceComponent, "GetNodeTransitionProgressAsyncOperation::End ");

        if (casted->Error.IsSuccess())
        {
            *reply = move(casted->result);
        }

        Trace.WriteWarning(TraceComponent, "GetNodeTransitionProgressAsyncOperation::End returning error {0}", casted->Error);

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginGetNodeTransitionProgress(
            operationId_,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        HRESULT error = comImpl_.EndGetNodeTransitionProgress(context, (IFabricNodeTransitionProgressResult**)&result);
        Trace.WriteWarning(TraceComponent, "GetNodeTransitionProgressAsyncOperation::EndComAsyncOperation returning error {0}", error);

        return error;
    }

    IFabricFaultAnalysisService & comImpl_;
    FABRIC_TEST_COMMAND_OPERATION_ID operationId_;
    IFabricNodeTransitionProgressResult * result;
    TimeSpan const timeout_;
};


class ComProxyFaultAnalysisService::StartNodeTransitionAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(StartNodeTransitionAsyncOperation)

public:
    StartNodeTransitionAsyncOperation(
        __in IFabricFaultAnalysisService & comImpl,
        __in FABRIC_NODE_TRANSITION_DESCRIPTION* description,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , description_(description)
        , timeout_(timeout)
    {
    }

    virtual ~StartNodeTransitionAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<StartNodeTransitionAsyncOperation>(operation);

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginStartNodeTransition(
            description_,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndStartNodeTransition(context);
    }

    IFabricFaultAnalysisService & comImpl_;
    FABRIC_NODE_TRANSITION_DESCRIPTION* description_;
    TimeSpan const timeout_;
};


class ComProxyFaultAnalysisService::GetStoppedNodeListAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(GetStoppedNodeListAsyncOperation)

public:
    GetStoppedNodeListAsyncOperation(
        __in IFabricFaultAnalysisService & comImpl,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , timeout_(timeout)
    {
    }

    virtual ~GetStoppedNodeListAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation, wstring & reply)
    {
        auto casted = AsyncOperation::End<GetStoppedNodeListAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            reply = move(casted->result_);
        }

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        Trace.WriteInfo(TraceComponent, "GetStoppedNodeListAsyncOperation BeginComAsyncOperation");

        return comImpl_.BeginGetStoppedNodeList(
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        ComPointer<IFabricStringResult> result;
        auto hr = comImpl_.EndGetStoppedNodeList(context, result.InitializationAddress());

        if (SUCCEEDED(hr))
        {
            hr = StringUtility::LpcwstrToWstring(
                result->get_String(),
                true,
                0,
                ParameterValidator::MaxEndpointSize,
                result_);
        }

        return hr;
    }

    IFabricFaultAnalysisService & comImpl_;
    wstring result_;

    TimeSpan const timeout_;
};

// ********************************************************************************************************************
// ComProxyFaultAnalysisService Implementation
//
ComProxyFaultAnalysisService::ComProxyFaultAnalysisService(
    ComPointer<IFabricFaultAnalysisService> const & comImpl)
    : IFaultAnalysisService()
    , ComProxySystemServiceBase<IFabricFaultAnalysisService>(comImpl)
{
}

ComProxyFaultAnalysisService::~ComProxyFaultAnalysisService()
{
}

AsyncOperationSPtr ComProxyFaultAnalysisService::BeginStartPartitionDataLoss(
    FABRIC_START_PARTITION_DATA_LOSS_DESCRIPTION * invokeDataLossDescription,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<InvokeDataLossAsyncOperation>(
        *comImpl_.GetRawPointer(),
        invokeDataLossDescription,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyFaultAnalysisService::EndStartPartitionDataLoss(
    AsyncOperationSPtr const & asyncOperation)
{
    return InvokeDataLossAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr ComProxyFaultAnalysisService::BeginCancelTestCommand(
    FABRIC_CANCEL_TEST_COMMAND_DESCRIPTION* description,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<CancelTestCommandAsyncOperation>(
        *comImpl_.GetRawPointer(),
        description,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyFaultAnalysisService::EndCancelTestCommand(
    AsyncOperationSPtr const & asyncOperation)
{
    return CancelTestCommandAsyncOperation::End(asyncOperation);
}


AsyncOperationSPtr ComProxyFaultAnalysisService::BeginGetPartitionDataLossProgress(
    FABRIC_TEST_COMMAND_OPERATION_ID operationId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<GetInvokeDataLossProgressAsyncOperation>(
        *comImpl_.GetRawPointer(),
        operationId,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyFaultAnalysisService::EndGetPartitionDataLossProgress(
    AsyncOperationSPtr const & asyncOperation,
    IFabricPartitionDataLossProgressResult ** reply)
{
    return GetInvokeDataLossProgressAsyncOperation::End(asyncOperation, reply);
}

AsyncOperationSPtr ComProxyFaultAnalysisService::BeginStartPartitionQuorumLoss(
    FABRIC_START_PARTITION_QUORUM_LOSS_DESCRIPTION * invokeQuorumLossDescription,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<InvokeQuorumLossAsyncOperation>(
        *comImpl_.GetRawPointer(),
        invokeQuorumLossDescription,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyFaultAnalysisService::EndStartPartitionQuorumLoss(
    AsyncOperationSPtr const & asyncOperation)
{
    return InvokeQuorumLossAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr ComProxyFaultAnalysisService::BeginGetPartitionQuorumLossProgress(
    FABRIC_TEST_COMMAND_OPERATION_ID operationId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<GetInvokeQuorumLossProgressAsyncOperation>(
        *comImpl_.GetRawPointer(),
        operationId,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyFaultAnalysisService::EndGetPartitionQuorumLossProgress(
    AsyncOperationSPtr const & asyncOperation,
    IFabricPartitionQuorumLossProgressResult ** reply)
{
    return GetInvokeQuorumLossProgressAsyncOperation::End(asyncOperation, reply);
}

AsyncOperationSPtr ComProxyFaultAnalysisService::BeginStartPartitionRestart(
    FABRIC_START_PARTITION_RESTART_DESCRIPTION * restartPartitionDescription,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<RestartPartitionAsyncOperation>(
        *comImpl_.GetRawPointer(),
        restartPartitionDescription,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyFaultAnalysisService::EndStartPartitionRestart(
    AsyncOperationSPtr const & asyncOperation)
{
    return RestartPartitionAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr ComProxyFaultAnalysisService::BeginGetPartitionRestartProgress(
    FABRIC_TEST_COMMAND_OPERATION_ID operationId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<GetRestartPartitionProgressAsyncOperation>(
        *comImpl_.GetRawPointer(),
        operationId,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyFaultAnalysisService::EndGetPartitionRestartProgress(
    AsyncOperationSPtr const & asyncOperation,
    IFabricPartitionRestartProgressResult ** reply)
{
    return GetRestartPartitionProgressAsyncOperation::End(asyncOperation, reply);
}

AsyncOperationSPtr ComProxyFaultAnalysisService::BeginGetTestCommandStatusList(
    FABRIC_TEST_COMMAND_LIST_DESCRIPTION * description,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<GetTestCommandStatusAsyncOperation>(
        *comImpl_.GetRawPointer(),
        description,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyFaultAnalysisService::EndGetTestCommandStatusList(
    AsyncOperationSPtr const & asyncOperation,
    IFabricTestCommandStatusResult **reply)

{
    return GetTestCommandStatusAsyncOperation::End(asyncOperation, reply);
}

// Chaos

AsyncOperationSPtr ComProxyFaultAnalysisService::BeginStartChaos(
    FABRIC_START_CHAOS_DESCRIPTION * startChaosDescription,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<StartChaosAsyncOperation>(
        *comImpl_.GetRawPointer(),
        startChaosDescription,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyFaultAnalysisService::EndStartChaos(
    AsyncOperationSPtr const & asyncOperation)
{
    return StartChaosAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr ComProxyFaultAnalysisService::BeginStopChaos(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<StopChaosAsyncOperation>(
        *comImpl_.GetRawPointer(),
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyFaultAnalysisService::EndStopChaos(
    AsyncOperationSPtr const & asyncOperation)
{
    return StopChaosAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr ComProxyFaultAnalysisService::BeginGetChaosReport(
    FABRIC_GET_CHAOS_REPORT_DESCRIPTION * description,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<GetChaosReportAsyncOperation>(
        *comImpl_.GetRawPointer(),
        description,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyFaultAnalysisService::EndGetChaosReport(
    AsyncOperationSPtr const & asyncOperation,
    IFabricChaosReportResult ** reply)
{
    return GetChaosReportAsyncOperation::End(asyncOperation, reply);
}

AsyncOperationSPtr ComProxyFaultAnalysisService::BeginGetStoppedNodeList(
    //    FABRIC_TEST_COMMAND_LIST_DESCRIPTION * description,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    Trace.WriteInfo(TraceComponent, "ComProxyFaultAnalysisService::BeginGetStoppedNodeList ");

    auto operation = AsyncOperation::CreateAndStart<GetStoppedNodeListAsyncOperation>(
        *comImpl_.GetRawPointer(),
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyFaultAnalysisService::EndGetStoppedNodeList(
    AsyncOperationSPtr const & asyncOperation,
    wstring & reply)
{
    return GetStoppedNodeListAsyncOperation::End(asyncOperation, reply);
};

AsyncOperationSPtr ComProxyFaultAnalysisService::BeginStartNodeTransition(
    FABRIC_NODE_TRANSITION_DESCRIPTION* description,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<StartNodeTransitionAsyncOperation>(
        *comImpl_.GetRawPointer(),
        description,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyFaultAnalysisService::EndStartNodeTransition(
    AsyncOperationSPtr const & asyncOperation)
{
    return StartNodeTransitionAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr ComProxyFaultAnalysisService::BeginGetNodeTransitionProgress(
    FABRIC_TEST_COMMAND_OPERATION_ID operationId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<GetNodeTransitionProgressAsyncOperation>(
        *comImpl_.GetRawPointer(),
        operationId,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyFaultAnalysisService::EndGetNodeTransitionProgress(
    AsyncOperationSPtr const & asyncOperation,
    IFabricNodeTransitionProgressResult ** reply)
{
    ErrorCode error = GetNodeTransitionProgressAsyncOperation::End(asyncOperation, reply);
    Trace.WriteWarning(TraceComponent, "ComProxyFaultAnalysisService::EndGetNodeTransitionProgress returning error {0}", error);

    return error;
}

// SystemServiceCall
AsyncOperationSPtr ComProxyFaultAnalysisService::BeginCallSystemService(
    std::wstring const & action,
    std::wstring const & inputBlob,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return this->BeginCallSystemServiceInternal(
        action,
        inputBlob,
        timeout,
        callback,
        parent);
}

ErrorCode ComProxyFaultAnalysisService::EndCallSystemService(
    Common::AsyncOperationSPtr const & asyncOperation,
    __inout std::wstring & outputBlob)
{
    return this->EndCallSystemServiceInternal(
        asyncOperation,
        outputBlob);
}
