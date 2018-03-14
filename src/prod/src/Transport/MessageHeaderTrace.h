// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    typedef std::function<void (MessageHeaders::HeaderReference&, Common::TextWriter & w)> MessageHeaderTraceFunc;
    typedef std::unordered_map<MessageHeaderId::Enum, MessageHeaderTraceFunc, std::hash<unsigned short>> MessageHeaderTraceFuncMap;

    class MessageHeaderTrace;
    typedef std::unique_ptr<MessageHeaderTrace> MessageHeaderTraceUPtr;

    class MessageHeaderTrace
    {
    public:
        static Common::Global<MessageHeaderTrace> GetSingleton();
        static void Trace(Common::TextWriter & w, MessageHeaders::HeaderReference & headerReference);

        MessageHeaderTrace();

        template <class TMessageHeader> void RegisterHeader();

    private:
        static Common::Global<MessageHeaderTrace> StaticInit();

    private:
        static Common::Global<MessageHeaderTrace> singleton_;

        Common::RwLock typeMapLock_;

        MessageHeaderTraceFuncMap headerTraceFuncMap_;
    };

    template <class TMessageHeader>
    void MessageHeaderTrace::RegisterHeader()
    {
        __if_exists(TMessageHeader::Id)
        {
            __if_exists(TMessageHeader::WriteTo)
            {
                AcquireWriteLock lock(typeMapLock_);

                headerTraceFuncMap_.insert(
                    std::pair<MessageHeaderId::Enum, MessageHeaderTraceFunc>(
                        TMessageHeader::Id, 
                        [](MessageHeaders::HeaderReference& headerReference, Common::TextWriter & w)
                        { 
                            TMessageHeader header;
                            headerReference.TryDeserialize<TMessageHeader>(header);
                            if (headerReference.IsValid)
                            {
                                w << L'(' << headerReference.Id() << L':' << headerReference.SerializedHeaderObjectSize() << L':' << header << L')';
                            }
                            else
                            {
                                w << L"(FailedToDeserialize: " << headerReference.Status << L')';
                            }
                        }));
            }
        }
    }

    template <class TMessageHeader>
    struct MessageHeaderTraceFuncRegister
    {
        MessageHeaderTraceFuncRegister()
        {
            MessageHeaderTrace::GetSingleton()->RegisterHeader<TMessageHeader>();
        }
    };

    #define REGISTER_MESSAGE_HEADER(Header) { const static Transport::MessageHeaderTraceFuncRegister<Header> traceFuncRegisterFor##Header; }
}
