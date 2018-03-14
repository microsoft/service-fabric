// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class FabricRuntimeImpl :
        public Common::ComponentRoot,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(FabricRuntimeImpl)

    public:
        FabricRuntimeImpl(
            Common::ComponentRoot const & parent,
            ApplicationHost & host,
            FabricRuntimeContext && context,
            Common::ComPointer<IFabricProcessExitHandler> const & fabricExitHandler);
        virtual ~FabricRuntimeImpl();

        __declspec(property(get=get_Id)) std::wstring const & RuntimeId;
        inline std::wstring const & get_Id() const { return runtimeId_; };

        __declspec(property(get=get_Host)) ApplicationHost & Host;
        inline ApplicationHost & get_Host() { return host_;  };

        __declspec(property(get=get_FactoryManager)) ServiceFactoryManagerUPtr const & FactoryManager;
        inline ServiceFactoryManagerUPtr const & get_FactoryManager() { return serviceFactoryManager_;  };

        void OnFabricProcessExited();
        FabricRuntimeContext GetRuntimeContext();
        void UpdateFabricRuntimeContext(FabricRuntimeContext const& fabricRuntimeContext);

    private:
        void SetFabricNodeContextData();

    private:
        Common::RwLock runtimeContextLock_;
        FabricRuntimeContext context_;
        std::wstring runtimeId_;
        Common::ComponentRootSPtr const parent_;
        ApplicationHost & host_;
        ServiceFactoryManagerUPtr serviceFactoryManager_;
        Common::ComPointer<IFabricProcessExitHandler> fabricExitHandler_;
    };
}
