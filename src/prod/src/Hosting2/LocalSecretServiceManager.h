//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // downloads and removes secrets for applications from LocalSecretStore
    class LocalSecretServiceManager :
        public Common::RootedObject,
        public Common::FabricComponent,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(LocalSecretServiceManager)

    public:
        LocalSecretServiceManager(
            Common::ComponentRoot const & root,
            __in HostingSubsystem & hosting);
        virtual ~LocalSecretServiceManager();

        __declspec(property(get = get_Hosting)) HostingSubsystem const & Hosting;
        HostingSubsystem const & get_Hosting() { return this->hosting_; }

        Common::AsyncOperationSPtr BeginGetSecrets(
            Management::CentralSecretService::GetSecretsDescription & getSecretsDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndGetSecrets(
            AsyncOperationSPtr const & operation,
            __out Management::CentralSecretService::SecretsDescription & secretValues);

        Common::ErrorCode ParseSecretStoreRef(
            std::vector<wstring> const & secretStoreRefs,
            __out Management::CentralSecretService::GetSecretsDescription & getSecretsDescription);
        
    protected:
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

    private:
        class GetSecretsAsyncOperation;
        HostingSubsystem & hosting_;
        
    };
}
