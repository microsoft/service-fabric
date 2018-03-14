// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Federation/SeedNodeProxy.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Federation;
using namespace LeaseWrapper;

StringLiteral const TraceTicketFile("TicketFile");

SeedNodeProxy::SeedNodeProxy(NodeConfig const & seedConfig, NodeId proxyId, bool shouldZeroTicketOnAbsentFile)
    :   VoteProxy(seedConfig.Id, proxyId, false),
        seedConfig_(seedConfig),
        shouldZeroTicketOnAbsentFile_(shouldZeroTicketOnAbsentFile), 
        store_()
{
}

SeedNodeProxy::SeedNodeProxy(NodeConfig const & seedConfig, NodeId proxyId)
    :   VoteProxy(seedConfig.Id, proxyId, false),
        seedConfig_(seedConfig),
        shouldZeroTicketOnAbsentFile_(true),
        store_()
{
}

SeedNodeProxy::~SeedNodeProxy()
{
    Abort();
}

ErrorCode SeedNodeProxy::OnAcquire(__inout NodeConfig & ownerConfig,
    Common::TimeSpan,
    bool,
    __out VoteOwnerState & state)
{
    if (ProxyId != VoteId)
    {
        ownerConfig = seedConfig_;
        return ErrorCodeValue::OwnerExists;
    }

    wstring filePath = Path::Combine(ownerConfig.WorkingDir, VoteId.ToString() + L".tkt");

    auto error = store_.TryOpen(filePath, FileMode::OpenOrCreate, FileAccess::ReadWrite);

    if (!error.IsSuccess())
    {
        WriteError(TraceTicketFile,
            "{0} failed to open file {1}: {2}",
            VoteId, filePath, error);
        return ErrorCodeValue::VoteStoreAccessError;
    }

    error = LoadTickets(state);
    if (!error.IsSuccess())
    {
        store_.Close();
    }

    return error;
}

ErrorCode SeedNodeProxy::LoadTickets(__out VoteOwnerState & state)
{
    FederationConfig & config = FederationConfig::GetConfig();
    DateTime now = DateTime::Now();

    store_.Seek(0, SeekOrigin::Begin);

    const int ByteCount = 2 * sizeof(DateTime);
    DateTime dateTimes[2];

    // Read in two DateTimes
    int bytesRead = store_.TryRead(dateTimes, ByteCount);
    if (bytesRead == ByteCount)
    {
        state.setGlobalTicket(dateTimes[0]);
        state.setSuperTicket(dateTimes[1]);

        WriteInfo(TraceTicketFile,
            "Ticket file for {0} loaded: {1}",
            VoteId, state);
        return ErrorCodeValue::Success;
    }

    if (bytesRead == 0 && shouldZeroTicketOnAbsentFile_)
    {
        state.GlobalTicket = DateTime::Zero;
        state.SuperTicket = DateTime::Zero;

        WriteInfo(TraceTicketFile,
            "Tickets not found for {0}, tickets set to zero",
            VoteId);
    }
    else
    {
        if (bytesRead == 0)
        {
            WriteError(TraceTicketFile,
                "Ticket file not found for {0}, tickets set to lease interval",
                VoteId);
        }
        else
        {
            WriteError(TraceTicketFile,
                "Invalid data in ticket file for {0} ignored",
                VoteId);
        }
        state.GlobalTicket = now + config.GlobalTicketLeaseDuration;
        state.SuperTicket = now + config.BootstrapTicketLeaseDuration;
    }

    ErrorCode error = OnSetGlobalTicket(state.GlobalTicket);
    if (error.IsSuccess())
    {
        error = OnSetSuperTicket(state.SuperTicket);
    }

    if (!error.IsSuccess())
    {
        store_.Close();
    }

    return error;
}

Common::ErrorCode SeedNodeProxy::WriteData(int offset, void const* buffer, int size)
{
    try
    {
        store_.Seek(offset, Common::SeekOrigin::Begin);
        store_.Write(buffer, size);
        store_.Flush();
        return ErrorCodeValue::Success;
    }
    catch(system_error const & e)
    {
        WriteError(TraceTicketFile,
            "{0} failed to update the vote store: {1}",
            VoteId, e);

        return ErrorCodeValue::VoteStoreAccessError;
    }
}

ErrorCode SeedNodeProxy::OnSetGlobalTicket(Common::DateTime globalTicket)
{
    WriteInfo(TraceTicketFile, "{0} Writing GlobalTicket to file: {1}", VoteId, globalTicket);
    return WriteData(0, &globalTicket, sizeof(DateTime));
}

ErrorCode SeedNodeProxy::OnSetSuperTicket(Common::DateTime superTicket)
{
    WriteInfo(TraceTicketFile, "{0} Writing SuperTicket to file: {1}", VoteId, superTicket);
    return WriteData(sizeof(DateTime), &superTicket, sizeof(DateTime));
}

ErrorCode SeedNodeProxy::OnOpen()
{
    return ErrorCodeValue::Success;
}

ErrorCode SeedNodeProxy::OnClose()
{
    Cleanup();
    return ErrorCodeValue::Success;
}

void SeedNodeProxy::OnAbort()
{
    Cleanup();
}

void SeedNodeProxy::Cleanup()
{
    if (isAcquired_)
    {
        try
        {
            store_.Close();
        }
        catch(system_error const & e)
        {
            WriteError(TraceTicketFile,
                "{0} failed to close the vote store: {1}",
                VoteId, e);
        }

        isAcquired_ = false;
    }
}

NodeConfig SeedNodeProxy::GetConfig()
{
    return this->seedConfig_;
}

AsyncOperationSPtr SeedNodeProxy::BeginArbitrate(
    SiteNode & siteNode,
    ArbitrationRequestBody const & request,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    MessageUPtr message;

    if (request.Type == ArbitrationType::TwoWaySimple ||
        request.Type == ArbitrationType::TwoWayExtended ||
        request.Type == ArbitrationType::RevertToReject)
    {
        message = FederationMessage::GetArbitrateRequest().CreateMessage(request);
    }
    else if (request.Type == ArbitrationType::KeepAlive)
    {
        message = FederationMessage::GetArbitrateKeepAlive().CreateMessage(request);
    }
    else
    {
        message = FederationMessage::GetExtendedArbitrateRequest().CreateMessage(request);
    }

    PartnerNodeSPtr seedNode = make_shared<PartnerNode>(this->GetConfig(), siteNode);

    // TODO: use one-way & extend request for RevertToReject after arbitrators all move to new version
    if (request.Type == ArbitrationType::RevertToNeutral ||
        request.Type == ArbitrationType::KeepAlive)
    {
        siteNode.Send(move(message), seedNode);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCode::Success(),
            OperationContextCallback<bool>(callback, true), 
            parent);
    }
    else
    {
        return siteNode.BeginSendRequest(move(message), seedNode, timeout, callback, parent);
    }
}

ErrorCode SeedNodeProxy::EndArbitrate(AsyncOperationSPtr const & operation,
    SiteNode & siteNode,
    __out ArbitrationReplyBody & result)
{
    if (operation->PopOperationContext<bool>())
    {
        return CompletedAsyncOperation::End(operation);
    }

    MessageUPtr reply;
    ErrorCode error = siteNode.EndSendRequest(operation, reply);
    if (error.IsSuccess())
    {
        if (!reply->GetBody<ArbitrationReplyBody>(result))
        {
            error = reply->FaultErrorCodeValue;
        }
    }

    return error;
}
