// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "Federation/PartnerNode.h"
#include "Federation/SiteNode.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Store;
using namespace ReliabilityUnitTest;
using namespace Transport;

void FederationWrapperStub::Reset()
{
    fmMessages_.clear();
    fmmMessages_.clear();
    otherNodeMessages_.clear();
}

void FederationWrapperStub::SendToFM(MessageUPtr&& msg)
{
    fmMessages_.push_back(move(msg));
}

void FederationWrapperStub::SendToFMM(MessageUPtr&& msg)
{
    fmmMessages_.push_back(move(msg));
}

void FederationWrapperStub::SendToNode(Transport::MessageUPtr && message, Federation::NodeInstance const & target)
{
    otherNodeMessages_[target.Id].push_back(move(message));
}

void FederationWrapperStub::VerifyNoMessagesSentToFMAndFMM()
{
    VerifyNoMessagesSentToFM();
    VerifyNoMessagesSentToFMM();
}

void FederationWrapperStub::VerifyNoMessagesSentToFM()
{
    Verify::IsTrueLogOnError(fmMessages_.empty(), L"Mesages were sent to FM when none were expected");
}

void FederationWrapperStub::VerifyNoMessagesSentToFMM()
{
    Verify::IsTrueLogOnError(fmmMessages_.empty(), L"Mesages were sent to FMM when none were expected");
}

void FederationWrapperStub::CompleteRequestReceiverContext(Federation::RequestReceiverContext & context, Transport::MessageUPtr && reply)
{
    if (validateRequestReplyContext_)
    {
        RequestReceiverContextState & obj = GetRequestReceiver(context);
        obj.Complete(move(reply));
    }
    else
    {
        context.Reply(move(reply));
    }
}

void FederationWrapperStub::CompleteRequestReceiverContext(Federation::RequestReceiverContext & context, Common::ErrorCode const & error)
{
    if (validateRequestReplyContext_)
    {
        RequestReceiverContextState & obj = GetRequestReceiver(context);
        obj.Reject(error);
    }
    else
    {
        context.Reject(error);
    }
}

class DummyRequestReceiveContext : public RequestReceiverContext
{
public:
    DummyRequestReceiveContext(MessageId const & id)
        : RequestReceiverContext(nullptr, nullptr, NodeInstance(), id)
    {
    }

    virtual void Reply(MessageUPtr &&) const
    {
    }
};

RequestReceiverContextUPtr FederationWrapperStub::CreateRequestReceiverContext(Transport::MessageId const & source, std::wstring const & tag)
{
    RequestReceiverContextUPtr rv = make_unique<DummyRequestReceiveContext>(source);

    RequestReceiverContextStateSPtr metadata = make_shared<RequestReceiverContextState>(tag);
    requestReceiverContexts_[source] = metadata;
    return rv;
}

Common::AsyncOperationSPtr FederationWrapperStub::BeginRequestToFMM(
    Transport::MessageUPtr && ,
    Common::TimeSpan const,
    Common::TimeSpan const,
    Common::AsyncCallback const &,
    Common::AsyncOperationSPtr const &) const 
{
    Assert::CodingError("Not used in RA test");
}
        
Common::ErrorCode FederationWrapperStub::EndRequestToFMM(Common::AsyncOperationSPtr const & , Transport::MessageUPtr & ) const 
{
    Assert::CodingError("Not used in RA test");
}

Common::AsyncOperationSPtr FederationWrapperStub::BeginRequestToFM(
    Transport::MessageUPtr && ,
    Common::TimeSpan const ,
    Common::TimeSpan const ,
    Common::AsyncCallback const & ,
    Common::AsyncOperationSPtr const & ) 
{
    Assert::CodingError("Not used in RA test");
}

Common::ErrorCode FederationWrapperStub::EndRequestToFM(Common::AsyncOperationSPtr const & , Transport::MessageUPtr & )
{
    Assert::CodingError("Not used in RA test");
}

RequestReceiverContextState & FederationWrapperStub::GetRequestReceiver(Federation::RequestReceiverContext & context)
{
    auto it = requestReceiverContexts_.find(context.RelatesToId);
    ASSERT_IF(it == requestReceiverContexts_.end(), "the context must have been created");
    
    return *it->second;
}

RequestReceiverContextState::RequestReceiverContextState(wstring const & tag)
: state_(Created),
  tag_(tag)
{
}

void RequestReceiverContextState::Complete(Transport::MessageUPtr && reply)
{
    ASSERT_IF(state_ != RequestReceiverContextState::Created, "must be in created");
    state_ = Completed;
    replyMessage_ = move(reply);
}

void RequestReceiverContextState::Reject(ErrorCode const & err)
{
    ASSERT_IF(state_ != RequestReceiverContextState::Created, "must be in created");
    state_ = Rejected;
    rejectError_ = err;
}

