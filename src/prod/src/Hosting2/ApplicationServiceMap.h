// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ApplicationServiceMap
    {
        DENY_COPY(ApplicationServiceMap)

    public:

        ApplicationServiceMap();
        ~ApplicationServiceMap();

        Common::ErrorCode Add(ApplicationServiceSPtr const & appProcess); 
        Common::ErrorCode AddContainer(ApplicationServiceSPtr const & appService);

        Common::ErrorCode Get(std::wstring const & parentId, std::wstring const & appInstanceId,  __out ApplicationServiceSPtr & appService);
        Common::ErrorCode Contains(std::wstring const & parentId, std::wstring const & appInstanceId,__out bool & contains);
        Common::ErrorCode Remove(std::wstring const & parentId, std::wstring const & appInstanceId);
        Common::ErrorCode Remove(std::wstring const & parentId, std::wstring const & appInstanceId, __out ApplicationServiceSPtr & appService);
        Common::ErrorCode ContainsParentId(std::wstring const & parentId, __out bool & contains);
        std::vector<ApplicationServiceSPtr> RemoveAllByParentId(std::wstring const & parentId);
        std::map<std::wstring, std::map<std::wstring, ApplicationServiceSPtr>> RemoveAllContainerServices();
        std::vector<ApplicationServiceSPtr> Close();
        std::map<std::wstring, std::map<std::wstring, ApplicationServiceSPtr>> GetAllServices(bool forContainer);
        void GetCount(__out size_t & containersCount, __out size_t & applicationsCount);

    private:
        Common::ErrorCode AddCallerHoldsLock(
            ApplicationServiceSPtr const & appService,
            std::map<std::wstring, std::map<std::wstring, ApplicationServiceSPtr>> & map);

        Common::ErrorCode GetCallerHoldsLock(
            std::wstring const & parentId,
            std::wstring const & appInstanceId,
            std::map<std::wstring, std::map<std::wstring, ApplicationServiceSPtr>> & map,
            __out ApplicationServiceSPtr & appService);

        Common::ErrorCode RemoveCallerHoldsLock(
            std::wstring const & parentId,
            std::wstring const & appInstanceId,
            std::map<std::wstring, std::map<std::wstring, ApplicationServiceSPtr>> & map,
            __out ApplicationServiceSPtr & appService);

    private:
        Common::RwLock lock_;
        Common::RwLock containersLock_;
        std::map<std::wstring, std::map<std::wstring, ApplicationServiceSPtr>> map_;
        std::map<std::wstring, std::map<std::wstring, ApplicationServiceSPtr>> containersMap_;
        bool isClosed_;
    };
}
