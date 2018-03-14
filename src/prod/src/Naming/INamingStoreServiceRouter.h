// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class INamingStoreServiceRouter
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

        // wrappers for communicating with other store services
        virtual Common::AsyncOperationSPtr BeginRequestReplyToPeer(
            Transport::MessageUPtr &&,
            Federation::NodeInstance const &,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndRequestReplyToPeer(
            Common::AsyncOperationSPtr const &,
            __out Transport::MessageUPtr &) = 0;                

        virtual bool ShouldAbortProcessing() = 0;
    };
}

