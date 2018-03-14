// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Transport/Message.h"
#include "Common/AsyncOperation.h"
#include "ServiceModel/federation/NodeId.h"

namespace Naming
{
    class INamingMessageProcessor
    {
    public:
        __declspec(property(get=get_Id)) Federation::NodeId Id;
        virtual Federation::NodeId get_Id() const = 0;

        virtual Common::AsyncOperationSPtr BeginProcessRequest(
            Transport::MessageUPtr &&, 
            Common::TimeSpan const,
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const & = Common::AsyncOperationSPtr()) = 0;
        virtual Common::ErrorCode EndProcessRequest(
            Common::AsyncOperationSPtr const &,
            __out Transport::MessageUPtr &) = 0;
    };

    typedef std::shared_ptr<INamingMessageProcessor> INamingMessageProcessorSPtr;
}
