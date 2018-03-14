// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ContainerDeactivationArgs : public Serialization::FabricSerializable
    {
    public:
        ContainerDeactivationArgs();

        ContainerDeactivationArgs(
            std::wstring const & containerName,
            bool configuredForAutoRemove,
            bool isContainerRoot,
            std::wstring const & cgroupName,
            bool isGracefulDeactivation);

        ContainerDeactivationArgs(ContainerDeactivationArgs const & other) = default;
        ContainerDeactivationArgs(ContainerDeactivationArgs && other) = default;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_CONTAINER_DEACTIVATION_ARGS & fabricDeactivationArgs) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_05(ContainerName, ConfiguredForAutoRemove, IsContainerRoot, CgroupName, IsGracefulDeactivation);

    public:
        std::wstring ContainerName;
        bool ConfiguredForAutoRemove;
        bool IsContainerRoot;
        std::wstring CgroupName;
        bool IsGracefulDeactivation;
    };
}
