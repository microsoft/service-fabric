// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace Transport;

ComFabricServiceCommunicationMessage::ComFabricServiceCommunicationMessage()
:IFabricServiceCommunicationMessage(), ComUnknownBase(), heap_(), body_(), headers_(), bodyBytes_()
{
}  

ComFabricServiceCommunicationMessage::ComFabricServiceCommunicationMessage(MessageUPtr & transportMessage)
: ComFabricServiceCommunicationMessage()
{

    TcpServiceMessageHeader header;
    if (transportMessage->Headers.TryReadFirst(header))
    {
        SetHeaderBuffer(header.Headers);

    }
    vector<const_buffer> buffers;
    transportMessage->GetBody(buffers);
    SetBodyBuffer(buffers);
}
ComFabricServiceCommunicationMessage::~ComFabricServiceCommunicationMessage()
{
}

FABRIC_MESSAGE_BUFFER *   ComFabricServiceCommunicationMessage::Get_Body()
{
    return body_.GetRawPointer();
}

FABRIC_MESSAGE_BUFFER * ComFabricServiceCommunicationMessage::Get_Headers()
{
    return headers_.GetRawPointer();
}

HRESULT ComFabricServiceCommunicationMessage::ToNativeTransportMessage(
    IFabricServiceCommunicationMessage *  msg,
    /*[out, retval]*/ MessageUPtr & message)
{
    void * state = nullptr;

    FABRIC_MESSAGE_BUFFER * body = msg->Get_Body();
    FABRIC_MESSAGE_BUFFER * headers = msg->Get_Headers();

    vector<Common::const_buffer> buffers;

    if (body){
        buffers.push_back(Common::const_buffer(body->Buffer, body->BufferSize));
    }
    //
    // Addref the communication message, so that it is not cleaned up till the transport message is
    // destructed.
    //
    ComPointer<IFabricServiceCommunicationMessage> serviceCommunicationMsgCPtr;
    serviceCommunicationMsgCPtr.SetAndAddRef(msg);

    message = Common::make_unique<Message>(
        buffers,
        [serviceCommunicationMsgCPtr](vector<Common::const_buffer> const &, void *){},
        state);

    if (headers != nullptr &&  headers->BufferSize > 0){
        TcpServiceMessageHeader serviceHeader{ headers->Buffer, headers->BufferSize };
        message->Headers.Add(serviceHeader);
    }
    return S_OK;
}

void ComFabricServiceCommunicationMessage::SetHeaderBuffer(__in vector<byte> const & vector)
{
    headers_ = heap_.AddItem<FABRIC_MESSAGE_BUFFER>();
    ULONG const initDataSize = static_cast<const ULONG>(vector.size());
    auto initData = heap_.AddArray<BYTE>(initDataSize);
    for (size_t i = 0; i < initDataSize; i++)
    {
        initData[i] = vector[i];
    }

    headers_->Buffer = initData.GetRawArray();
    headers_->BufferSize = initDataSize;
}

void ComFabricServiceCommunicationMessage::SetBodyBuffer(__in vector<Common::const_buffer> const & body)
{
    size_t size = 0;
    for (auto const & buffer : body)
    {
        size += buffer.len;
    }
    bodyBytes_.reserve(size);
    for (auto const & buffer : body)
    {
        bodyBytes_.insert(bodyBytes_.end(), buffer.buf, buffer.buf + buffer.len);
    }

    body_ = heap_.AddItem<FABRIC_MESSAGE_BUFFER>();

    body_->Buffer = bodyBytes_.data();
    body_->BufferSize = static_cast<ULONG>(bodyBytes_.size());
}

