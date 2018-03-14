// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class MulticastDatagramSender;

    class MulticastSendTarget;
    typedef std::shared_ptr<MulticastSendTarget> MulticastSendTargetSPtr;
    
    // MulticastSendTarget represents a collection of SendTargets to which a multicast message
    // can be sent. It provides iterators traversing through the collection.
    //
    // TODO: implement iterator pattern?
    class MulticastSendTarget
    {
        DENY_COPY(MulticastSendTarget)

    public:
        template <class TSendTargetIterator>
        MulticastSendTarget(TSendTargetIterator begin, TSendTargetIterator end)
        : targets_(begin, end)
        {
        }
        virtual ~MulticastSendTarget();
        ISendTarget::SPtr const & operator[](size_t index);
        size_t size() const;

        
    private:
        std::vector<ISendTarget::SPtr> targets_;
    };
}
