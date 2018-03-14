// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ServiceLocationChangeClientManager
    {
        DENY_COPY(ServiceLocationChangeClientManager);

        friend class ServiceLocationChangeClient;
        
    public:
        ServiceLocationChangeClientManager();
        
        __declspec (property(get=get_DefaultNamedClient)) ServiceLocationChangeClientSPtr const& DefaultNamedClient;
        

        bool AddClient(std::wstring const & clientName, Common::ComPointer<IFabricServiceManagementClient> && client);
        
        bool DeleteClient(std::wstring const & clientName);

        bool FindClient(std::wstring const & clientName, ServiceLocationChangeClientSPtr & client);

        void GetDefaultNamedClient(__out ServiceLocationChangeClientSPtr & client);
        void ResetDefaultNamedClient();

    private:
        std::map<std::wstring, ServiceLocationChangeClientSPtr> namedClients_;
        ServiceLocationChangeClientSPtr defaultNamedClient_;
        Common::ExclusiveLock lock_;

        static const Common::StringLiteral TraceSource;
    };
}
