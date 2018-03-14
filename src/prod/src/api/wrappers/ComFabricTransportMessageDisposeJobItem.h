// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComFabricTransportMessageDisposeJobItem
    {
        DENY_COPY(ComFabricTransportMessageDisposeJobItem);
    public:
        ComFabricTransportMessageDisposeJobItem() {}
        ComFabricTransportMessageDisposeJobItem(ComPointer<IFabricTransportMessage> && message)
            : message_(move(message)) {

        }

        ComFabricTransportMessageDisposeJobItem(ComFabricTransportMessageDisposeJobItem && other)
            : message_(move(other.message_))
        {
        }


        ComFabricTransportMessageDisposeJobItem& operator=(ComFabricTransportMessageDisposeJobItem && other)
        {
            if (this != &other)
            {
                message_ = move(other.message_);
            }

            return *this;

        }

        bool ProcessJob(ComponentRoot &)
        {
            this->message_->Dispose();
            return true;
        }

    private:
        ComPointer<IFabricTransportMessage> message_;
    };
   
}
