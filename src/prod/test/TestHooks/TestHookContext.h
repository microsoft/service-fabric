// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestHooks
{
    class TestHookContext
    {
    public:
        TestHookContext()
            : nodeId_()
            , serviceName_()
        {
        }

        TestHookContext(
            Federation::NodeId const & nodeId,
            std::wstring const & serviceName)
            : nodeId_(nodeId)
            , serviceName_(serviceName)
        {
        }

        __declspec(property(get=get_NodeId)) Federation::NodeId const & NodeId;
        __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;

        Federation::NodeId const & get_NodeId() const { return nodeId_; }
        std::wstring const & get_ServiceName() const { return serviceName_; }

    private:
        Federation::NodeId nodeId_;
        std::wstring serviceName_;
    };
}
