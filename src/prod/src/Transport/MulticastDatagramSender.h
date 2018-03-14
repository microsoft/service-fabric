// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    typedef std::shared_ptr<MulticastDatagramSender> MulticastDatagramSenderSPtr;
    
    //
    // MulticastDatagramSender allows efficient transmission of message to a collection of transport targets.
    // 
    class MulticastDatagramSender
    {
        DENY_COPY(MulticastDatagramSender)

    public:
        //
        // Construct a MulticastDatagramSender that will use the specified unicast datagram 
        // transport to send message to a collection of transport targets.
        //
        MulticastDatagramSender(IDatagramTransportSPtr unicastTransport);
        virtual ~MulticastDatagramSender();

        //
        // Resolve a list of transport addresses to SendTargets. Requires a begin and end iterator
        // to a collection of NamedAddress.
        //
        template<class TAddressIterator>
        MulticastSendTargetSPtr Resolve(TAddressIterator begin, TAddressIterator end, size_t & failedResolves)
        {
            failedResolves = 0;

            vector<ISendTarget::SPtr> targets;

            for(TAddressIterator iter = begin; iter < end; iter++)
            {
                auto target = transport_->ResolveTarget(iter->Address, iter->Name);

                if (!target)
                {
                    ++failedResolves;
                }

                targets.push_back(std::move(target));
            }

            return make_shared<MulticastSendTarget>(targets.begin(), targets.end());
        }

        //
        // Send specified message to a collection of SendTargets. The specified message must not 
        // contain any target specific content. All target specific headers must be specified via
        // targetHeaders parameter. The TargetHeadersCollection must be created from 
        // SendTargetCollection. The iterator of TargetHeaderCollection has position equivalence to 
        // corresponding iterator in SendTargetCollections.
        void Send(
            MulticastSendTargetSPtr const & multicastTarget, 
            MessageUPtr && message, 
            MessageHeadersCollection && targetHeaders = std::move(MessageHeadersCollection(0)));

    private:
         IDatagramTransportSPtr transport_;
    };
}
