// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class GetContainerInfoRequest : public Serialization::FabricSerializable
    {
    public:
        GetContainerInfoRequest();
        GetContainerInfoRequest(
            std::wstring const & nodeId,
            std::wstring const & appServiceId,
            bool isServicePackageActivationModeExclusive,
            std::wstring const & containerInfoType,
            std::wstring const & containerInfoArgs);

        __declspec(property(get = get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return nodeId_; }

        __declspec(property(get = get_AppServiceId)) std::wstring const & ApplicationServiceId;
        std::wstring const & get_AppServiceId() const { return appServiceId_; }

        __declspec(property(get = get_IsServicePackageActivationModeExclusive)) bool IsServicePackageActivationModeExclusive;
        bool get_IsServicePackageActivationModeExclusive() { return isServicePackageActivationModeExclusive_; }

        __declspec(property(get = get_ContainerInfoType)) std::wstring const & ContainerInfoType;
        std::wstring const & get_ContainerInfoType() const { return containerInfoType_; }
    
        __declspec(property(get = get_ContainerInfoArgs)) std::wstring const & ContainerInfoArgs;
        std::wstring const & get_ContainerInfoArgs() const { return containerInfoArgs_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_05(nodeId_, appServiceId_, isServicePackageActivationModeExclusive_, containerInfoType_, containerInfoArgs_);

    private:
        std::wstring nodeId_;
        std::wstring appServiceId_;
        bool isServicePackageActivationModeExclusive_;
        std::wstring containerInfoType_;
        std::wstring containerInfoArgs_;
    };
}
