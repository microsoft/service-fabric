// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace Transport;

using namespace Transport;

ComFabricTransportMessage::ComFabricTransportMessage()
    :IFabricTransportMessage(), ComUnknownBase(), heap_(), bodybuffer_(), header_(), headerBytes_()
{
}

ComFabricTransportMessage::ComFabricTransportMessage(MessageUPtr && transportMessage)
    : ComFabricTransportMessage()
{
    this->transportMessage_ = move(transportMessage);
    if (this->transportMessage_->Headers.TryReadFirst(messageHeader_))
    {
        SetBuffers(messageHeader_.HeaderSize);
    }
    else 
    {
        SetBuffers(0);
    }
}

ComFabricTransportMessage::~ComFabricTransportMessage()
{
}

void ComFabricTransportMessage:: GetHeaderAndBodyBuffer(
	const FABRIC_TRANSPORT_MESSAGE_BUFFER ** headerBuffer,
	ULONG * msgBufferCount,
	const FABRIC_TRANSPORT_MESSAGE_BUFFER ** msgBuffers) 
{
	*headerBuffer = header_.GetRawPointer();
	*msgBufferCount = static_cast<ULONG>(this->bodybuffer_.GetCount());
	*msgBuffers = this->bodybuffer_.GetRawArray();
}

void ComFabricTransportMessage::Dispose() {
    //no-op
}

HRESULT ComFabricTransportMessage::ToNativeTransportMessage(
    IFabricTransportMessage *  msg,
    DeleteCallback const & callback,
    /*[out, retval]*/ MessageUPtr & message)
{
    void * state = nullptr;
    vector<Common::const_buffer> buffers;
  
    //Add Headers to Transport Message
    FABRIC_TRANSPORT_MESSAGE_BUFFER const *headers = NULL;
	ULONG bufferCount = 0;

	FABRIC_TRANSPORT_MESSAGE_BUFFER  const * bodybuffers = NULL;
     msg->GetHeaderAndBodyBuffer(&headers,&bufferCount, &bodybuffers);

    if (headers->BufferSize > 0) {
        //add header to buffers
        auto constbuffer = Common::const_buffer(headers->Buffer, headers->BufferSize);
        buffers.push_back(constbuffer);
    }

    //Add Body to Transport Message

       
        for (ULONG i = 0; i < bufferCount; i++)
        {
            auto constbuffer = Common::const_buffer(bodybuffers[i].Buffer, bodybuffers[i].BufferSize);
            buffers.push_back(constbuffer);
        }
    
    //
    // Addref the communication message, so that it is not cleaned up till the transport message is
    // destructed.
    //
    ComPointer<IFabricTransportMessage> fabricTransportMsgCPtr;
    fabricTransportMsgCPtr.SetAndAddRef(msg);

    if (buffers.size() > 0)
    {
        message = Common::make_unique<Message>(
            buffers,
            [fabricTransportMsgCPtr, callback](vector<Common::const_buffer> const & , void *) {
            if (callback)
            {
                callback(fabricTransportMsgCPtr);
            }

        },
            state);
    }
    else
    {
        message = Common::make_unique<Message>(
            buffers,
            [fabricTransportMsgCPtr](vector<Common::const_buffer> const &, void *) {},
            state);
     }
    
    if (headers->BufferSize > 0) 
    {
        FabricTransportMessageHeader messageHeader{ headers->BufferSize };
        message->Headers.Add(messageHeader);
    };
    return S_OK;
}

void ComFabricTransportMessage::SetBuffers(ULONG headerSize)
{
    //Read Headers
    vector<const_buffer> bodybuffers;
    this->transportMessage_->GetBody(bodybuffers);
    auto i = 0;
    auto bytesRead = 0;
    if (headerSize > 0)
    {
        header_ = heap_.AddItem<FABRIC_TRANSPORT_MESSAGE_BUFFER>();
        ULONG bufferSize = static_cast<ULONG>(bodybuffers[0].len);
        //Case where complete header is in single buffer.
        if(headerSize <= bufferSize)
        {
            header_->Buffer = (BYTE*)bodybuffers[0].buf;
            header_->BufferSize = headerSize;
            if (headerSize < bufferSize)
            {
                bytesRead = headerSize;
            }
            else 
            {
                i++;
            }
          }
        else 
        {//where header is spread into multiple buffers
            headerBytes_.reserve(headerSize);
            while (headerSize > 0) 
            {
                bufferSize = static_cast<ULONG>(bodybuffers[i].len);
                if (headerSize >= bufferSize) 
                {
                    headerBytes_.insert(headerBytes_.end(),
                        bodybuffers[i].buf, bodybuffers[i].buf + bufferSize);
                    headerSize -= bufferSize;                    
                    i++;
                }
                else
                {
                    headerBytes_.insert(headerBytes_.end(),
                        bodybuffers[i].buf, bodybuffers[i].buf + headerSize);
                    bytesRead = headerSize;
                    headerSize = 0;                    
                }
                
            }
            header_->Buffer = headerBytes_.data();
            header_->BufferSize = static_cast<ULONG>(headerBytes_.size());           
        }
    }
    //Read Body
    SetBodyBuffer(bodybuffers,i, bytesRead);
    
}

void ComFabricTransportMessage::SetBodyBuffer(__in vector<Common::const_buffer> const & body,size_t bufferIndex,ULONG bytesReadFromFirstBuffer)
{
    auto bodyBufferSize = body.size() - bufferIndex;
    this->bodybuffer_ = heap_.AddArray<FABRIC_TRANSPORT_MESSAGE_BUFFER>(bodyBufferSize);
    auto index = 0;
    for (size_t i = bufferIndex; i < body.size(); i++)
    {
        auto currentbuffer = body[i];
        if (i == bufferIndex)
        {
            this->bodybuffer_[index].Buffer = (BYTE*)(currentbuffer.buf + bytesReadFromFirstBuffer);
           this->bodybuffer_[index].BufferSize = currentbuffer.len-bytesReadFromFirstBuffer;
        }
        else
        {
            this->bodybuffer_[index].Buffer = (BYTE*)currentbuffer.buf;
            this->bodybuffer_[index].BufferSize = currentbuffer.len;
        }
        index++;
    }
}

