// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComFabricServiceCommunicationMessage : public IFabricServiceCommunicationMessage,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComFabricServiceCommunicationMessage);

        COM_INTERFACE_LIST1(
            ComFabricServiceCommunicationMessage,
            IID_IFabricServiceCommunicationMessage,
            IFabricServiceCommunicationMessage)

    public:

        static HRESULT ComFabricServiceCommunicationMessage::ToNativeTransportMessage(
            IFabricServiceCommunicationMessage *  msg,
            Transport::MessageUPtr & message);


        virtual ~ComFabricServiceCommunicationMessage();
        ComFabricServiceCommunicationMessage();
        ComFabricServiceCommunicationMessage::ComFabricServiceCommunicationMessage(Transport::MessageUPtr  & transportMessage);

        virtual FABRIC_MESSAGE_BUFFER * Get_Body();

        virtual FABRIC_MESSAGE_BUFFER * Get_Headers();

    private:
        Common::ReferencePointer<FABRIC_MESSAGE_BUFFER> body_;
        Common::ReferencePointer<FABRIC_MESSAGE_BUFFER>  headers_;
        std::vector<byte> bodyBytes_;
        Common::ScopedHeap heap_;
        void SetBodyBuffer(__in std::vector<Common::const_buffer> const & vector);
        void SetHeaderBuffer(__in std::vector<byte> const & vector);
    };
}
