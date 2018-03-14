// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ApplicationHostRegistrationTable :
        public Common::RootedObject
    {
        DENY_COPY(ApplicationHostRegistrationTable)

        public:
            ApplicationHostRegistrationTable(Common::ComponentRoot const & root);
            virtual ~ApplicationHostRegistrationTable();

            Common::ErrorCode Add(ApplicationHostRegistrationSPtr const & registration);
            Common::ErrorCode Remove(std::wstring const & hostId, __out ApplicationHostRegistrationSPtr & registration);
            Common::ErrorCode Find(std::wstring const & hostId, __out ApplicationHostRegistrationSPtr & registration);
            std::vector<ApplicationHostRegistrationSPtr> Close();

        private:
            bool isClosed_;
            Common::RwLock lock_;
            std::map<std::wstring, ApplicationHostRegistrationSPtr, Common::IsLessCaseInsensitiveComparer<std::wstring>> map_;
    };
}
