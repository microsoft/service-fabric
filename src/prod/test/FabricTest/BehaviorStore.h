// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class BehaviorStore
    {
    public:
        typedef std::set<std::wstring> BehaviorSet;
        typedef std::map<std::wstring, BehaviorSet> ServiceBehaviorSet;
        typedef std::map<std::wstring, ServiceBehaviorSet> NodeServiceBehaviorSet;

        BehaviorStore();

        void AddBehavior(
            __in std::wstring const & nodeId,
            __in std::wstring const& serviceName,
            __in std::wstring const& strBehavior);

        void RemoveBehavior(
            __in std::wstring const & nodeId,
            __in std::wstring const& serviceName,
            __in std::wstring const& strBehavior);

        bool IsBehaviorSet(
            __in std::wstring const& nodeId,
            __in std::wstring const& serviceName,
            __in std::wstring const& strBehavior) const;

        bool IsBehaviorSetForNode(
            NodeServiceBehaviorSet::const_iterator const& iteratorNode,
            __in std::wstring const& serviceName,
            __in std::wstring const& strBehavior) const;

        bool IsBehaviorSetForService(
            ServiceBehaviorSet::const_iterator const& iteratorService,
            __in std::wstring const& strBehavior) const;

        bool IsSupportedNodeStr(
            __in std::wstring const& nodeNameStr) const;

        bool IsSupportedServiceStr(
            __in std::wstring const& serviceNameStr) const;
    private:
        NodeServiceBehaviorSet behaviorMap_;
        static Common::StringLiteral const TraceSource;
    };
};
