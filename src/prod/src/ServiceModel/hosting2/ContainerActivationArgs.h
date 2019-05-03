// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ContainerActivationArgs : public Serialization::FabricSerializable
    {
    public:
        ContainerActivationArgs();

        ContainerActivationArgs(
            bool isUserLocalSystem,
            std::wstring const & appHostId,
            std::wstring const & nodeId,
            ContainerDescription const & containerDesc,
            ProcessDescription const & processDesc,
            std::wstring const & fabricBinPath,
            std::vector<std::wstring> const & gatewayIpAddresses);

        ContainerActivationArgs(ContainerActivationArgs const & other) = default;
        ContainerActivationArgs(ContainerActivationArgs && other) = default;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_CONTAINER_ACTIVATION_ARGS & fabricActivationArgs) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_08(IsUserLocalSystem, AppHostId, NodeId, ContainerDescriptionObj, ProcessDescriptionObj, FabricBinPath, GatewayIpAddress, GatewayIpAddresses);

    public:
        bool IsUserLocalSystem;
        std::wstring AppHostId;
        std::wstring NodeId;
        ContainerDescription ContainerDescriptionObj;
        ProcessDescription ProcessDescriptionObj;
        std::wstring FabricBinPath;
        std::wstring GatewayIpAddress;
        std::vector<std::wstring> GatewayIpAddresses;
    };
}