// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class ActivateProcessRequest : public Serialization::FabricSerializable
    {
    public:
        ActivateProcessRequest();
        ActivateProcessRequest(
            DWORD parentProcessId,
            std::wstring const & nodeId,
            std::wstring const & applicationId,
            std::wstring const & appServiceId,
            ProcessDescription const & processDescription,
            std::wstring const & runasName,
            int64 timeoutTicks,
            std::wstring const & fabricBinFolder,
            bool isContainerHost = false,
            ContainerDescription const & containerDescription = ContainerDescription());
            

        __declspec(property(get=get_ParentProcessId)) DWORD ParentProcessId;
        DWORD get_ParentProcessId() const { return parentProcessId_; }

        __declspec(property(get=get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return nodeId_; }
        
        __declspec(property(get=get_ApplicationId)) std::wstring const & ApplicationId;
        std::wstring const & get_ApplicationId() const { return applicationId_; }

        __declspec(property(get=get_AppServiceId)) std::wstring const & ApplicationServiceId;
        std::wstring const & get_AppServiceId() const { return appServiceId_; }

         __declspec(property(get=get_TimeoutTicks)) int64 TimeoutInTicks;
        int64 get_TimeoutTicks() const { return ticks_; }


        __declspec(property(get=get_ProcessDescription)) ProcessDescription const & ProcDescription;
        ProcessDescription const & get_ProcessDescription() const { return processDescription_; }

        __declspec(property(get=get_SecurityUserId)) std::wstring const & SecurityUserId;
        std::wstring const & get_SecurityUserId() const { return securityUserId_; }

        __declspec(property(get=get_FabricBinFolder)) std::wstring const & FabricBinFolder;
        std::wstring const & get_FabricBinFolder() const { return fabricBinFolder_; }


        __declspec(property(get = get_IsContainerHost)) bool IsContainerHost;
        bool get_IsContainerHost() const { return isContainerHost_; }

        __declspec(property(get = get_ContainerDescription)) ContainerDescription const & ContainerDescriptionObj;
        ContainerDescription get_ContainerDescription() const { return containerDescription_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_10( 
            parentProcessId_,
            nodeId_,
            applicationId_,
            appServiceId_,
            processDescription_,
            securityUserId_,
            ticks_,
            fabricBinFolder_,
            isContainerHost_,
            containerDescription_);

    private:
        DWORD parentProcessId_;
        std::wstring nodeId_;
        std::wstring applicationId_;
        std::wstring appServiceId_;
        ProcessDescription processDescription_;
        std::wstring securityUserId_;
        int64 ticks_;
        std::wstring fabricBinFolder_;
        bool isContainerHost_;
        ContainerDescription containerDescription_;
    };
}
