// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class INodeWrapper
    {
    public:
        virtual ~INodeWrapper() { };

        virtual void GetRuntimeManagerUnderLock(std::function<void(Hosting2::FabricRuntimeManager &)> processor) const = 0;

        virtual void GetReliabilitySubsystemUnderLock(std::function<void(ReliabilityTestApi::ReliabilityTestHelper &)> processor) const = 0;

        virtual ReliabilityTestApi::FailoverManagerComponentTestApi::FailoverManagerTestHelperUPtr GetFailoverManager() const = 0;

        virtual ReliabilityTestApi::FailoverManagerComponentTestApi::FailoverManagerTestHelperUPtr GetFailoverManagerMaster() const = 0;

        __declspec (property(get=get_IsOpened)) bool IsOpened;
        virtual bool get_IsOpened() const = 0;

        __declspec (property(get=get_IsOpening)) bool IsOpening;
        virtual bool get_IsOpening() const = 0;

        __declspec (property(get=get_Instance)) Federation::NodeInstance Instance;
        virtual Federation::NodeInstance get_Instance() const = 0;

        __declspec (property(get=get_Id)) Federation::NodeId Id;
        virtual Federation::NodeId get_Id() const = 0;

        __declspec(property(get=get_NodeOpenTimestamp)) Common::StopwatchTime NodeOpenTimestamp;
        virtual Common::StopwatchTime get_NodeOpenTimestamp() const = 0;

        __declspec (property(get=get_RuntimeManager)) std::unique_ptr<FabricTestRuntimeManager> const& RuntimeManager;
        virtual std::unique_ptr<FabricTestRuntimeManager> const& get_RuntimeManager() const = 0;

        __declspec (property(get=get_Node)) Fabric::FabricNodeSPtr const & Node;
        virtual Fabric::FabricNodeSPtr const & get_Node() const = 0;

        __declspec (property(get=get_NodePropertyMap)) std::map<std::wstring, std::wstring> const& NodeProperties;
        virtual std::map<std::wstring, std::wstring> const& get_NodePropertyMap() const = 0;

        virtual void Open(
            Common::TimeSpan, 
            bool expectNodeFailure, 
            bool addRuntime, 
            Common::ErrorCodeValue::Enum openErrorCode,
            std::vector<Common::ErrorCodeValue::Enum> const& retryErrors) = 0;

        virtual void Close(bool removeData = false) = 0;
        virtual void Abort() = 0;

        virtual TestHostingSubsystem & GetTestHostingSubsystem() const = 0;

        virtual Hosting2::HostingSubsystem & GetHostingSubsystem() const = 0;
    };
};
