// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IFaultManagementClient
    {
    public:
        virtual ~IFaultManagementClient() {};

        virtual Common::AsyncOperationSPtr BeginRestartNode(
            std::wstring const & nodeName,
            std::wstring const & instanceId,
            bool shouldCreateFabricDump,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndRestartNode(
            Common::AsyncOperationSPtr const &,
            std::wstring const & nodeName,
            std::wstring const & instanceId,
            __out IRestartNodeResultPtr &result) = 0;

        virtual Common::AsyncOperationSPtr BeginStartNode(
            Management::FaultAnalysisService::StartNodeDescriptionUsingNodeName const & description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndStartNode(
            Common::AsyncOperationSPtr const &,
            Management::FaultAnalysisService::StartNodeDescriptionUsingNodeName const & description,
            __out IStartNodeResultPtr &result) = 0;

        virtual Common::AsyncOperationSPtr BeginStopNode(
            Management::FaultAnalysisService::StopNodeDescriptionUsingNodeName const & description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndStopNode(
            Common::AsyncOperationSPtr const &,
            Management::FaultAnalysisService::StopNodeDescriptionUsingNodeName const & description,
            __out IStopNodeResultPtr &result) = 0;

        virtual Common::AsyncOperationSPtr BeginRestartDeployedCodePackage(
            Management::FaultAnalysisService::RestartDeployedCodePackageDescriptionUsingNodeName const & description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndRestartDeployedCodePackage(
            Common::AsyncOperationSPtr const &,
            Management::FaultAnalysisService::RestartDeployedCodePackageDescriptionUsingNodeName const & description,
            __out IRestartDeployedCodePackageResultPtr &result) = 0;

        virtual Common::AsyncOperationSPtr BeginMovePrimary(
            Management::FaultAnalysisService::MovePrimaryDescriptionUsingNodeName const & description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndMovePrimary(
            Common::AsyncOperationSPtr const &,
            Management::FaultAnalysisService::MovePrimaryDescriptionUsingNodeName const & description,
            __out IMovePrimaryResultPtr &result) = 0;

        virtual Common::AsyncOperationSPtr BeginMoveSecondary(
            Management::FaultAnalysisService::MoveSecondaryDescriptionUsingNodeName const & description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndMoveSecondary(
            Common::AsyncOperationSPtr const &,
            Management::FaultAnalysisService::MoveSecondaryDescriptionUsingNodeName const & description,
            __out IMoveSecondaryResultPtr &result) = 0;
        
        virtual Common::AsyncOperationSPtr BeginStopNodeInternal(
            std::wstring const & nodeName,
            std::wstring const & instanceId,
            DWORD durationInSeconds,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;        
        virtual Common::ErrorCode EndStopNodeInternal(
            Common::AsyncOperationSPtr const &) = 0;
    };
}
