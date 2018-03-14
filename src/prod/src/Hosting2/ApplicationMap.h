// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ApplicationMap :
        public Common::RootedObject
    {
        DENY_COPY(ApplicationMap)

    public:
        typedef std::function<bool(Application2SPtr const &)> Visitor;

        ApplicationMap(Common::ComponentRoot const & root);
        ~ApplicationMap();

        Common::ErrorCode Add(Application2SPtr const & application);
        Common::ErrorCode Get(ServiceModel::ApplicationIdentifier const & applicationId, __out Application2SPtr & application);
        Common::ErrorCode Contains(ServiceModel::ApplicationIdentifier const & applicationId, __out bool & contains);
        Common::ErrorCode Remove(ServiceModel::ApplicationIdentifier const & applicationId, int64 const instanceId, __out Application2SPtr & application);
        Common::ErrorCode VisitUnderLock(ApplicationMap::Visitor const & visitor);
        // Note: The continuation token is expected to be the application name
        Common::ErrorCode GetApplications(__out std::vector<Application2SPtr> & applications, std::wstring const & filterApplicationName = L"", std::wstring const & continuationToken = L"");

        void Open();
        std::vector<Application2SPtr> Close();

    private:
        void NotifyDcaAboutServicePackages();

    private:
        Common::RwLock lock_;
        std::map<ServiceModel::ApplicationIdentifier, Application2SPtr> map_;
        bool isClosed_;
        Common::TimerSPtr notifyDcaTimer_;
    };
}
