// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    typedef std::function<void(ComPointer<IFabricTransportMessage>)> DeleteCallback;

    class ComFabricTransportMessage : public IFabricTransportMessage,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComFabricTransportMessage);

        COM_INTERFACE_LIST1(
            ComFabricTransportMessage,
            IID_IFabricTransportMessage,
            IFabricTransportMessage)

    public:
        static IID iid;
       
        static HRESULT ComFabricTransportMessage::ToNativeTransportMessage(
            IFabricTransportMessage *  msg,
            DeleteCallback const & callback,
            Transport::MessageUPtr & message);

        virtual ~ComFabricTransportMessage();
        ComFabricTransportMessage();
        ComFabricTransportMessage(Transport::MessageUPtr  && transportMessage);

        virtual void GetHeaderAndBodyBuffer(
		const FABRIC_TRANSPORT_MESSAGE_BUFFER ** headerBuffer,
		ULONG * msgBuffercount,
		const FABRIC_TRANSPORT_MESSAGE_BUFFER ** messageBuffers);

        virtual void Dispose();
       
    private:
        Common::ReferenceArray<FABRIC_TRANSPORT_MESSAGE_BUFFER> bodybuffer_;
        Common::ReferencePointer<FABRIC_TRANSPORT_MESSAGE_BUFFER>  header_;
        Common::ScopedHeap heap_;
        std::vector<byte> headerBytes_;
        Transport::MessageUPtr transportMessage_;
        FabricTransportMessageHeader messageHeader_;
        void SetBodyBuffer(__in vector<Common::const_buffer> const & body, size_t bufferIndex, ULONG bufferSize);        
        void SetBuffers(ULONG headerSize);
    };
}
