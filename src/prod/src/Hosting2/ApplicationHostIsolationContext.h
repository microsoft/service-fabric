// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ApplicationHostIsolationContext
    {
        public:
            enum ProcessSharingLevel
            {
                ProcessSharingLevel_Application,
                ProcessSharingLevel_CodePackage
            };
            
        public:
            ApplicationHostIsolationContext(std::wstring const & processContext, std::wstring const & userContext);
            ApplicationHostIsolationContext(ApplicationHostIsolationContext const & other);
            ApplicationHostIsolationContext(ApplicationHostIsolationContext && other);

            ApplicationHostIsolationContext const & operator = (ApplicationHostIsolationContext const & other);
            ApplicationHostIsolationContext const & operator = (ApplicationHostIsolationContext && other);

            bool operator == (ApplicationHostIsolationContext const & other) const;
            bool operator != (ApplicationHostIsolationContext const & other) const;

            int compare(ApplicationHostIsolationContext const & other) const;
            bool operator < (ApplicationHostIsolationContext const & other) const;

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            ProcessSharingLevel GetProcessSharingLevel() const;

            static ApplicationHostIsolationContext Create(
                CodePackageInstanceIdentifier const & codePackageInstanceId,
                std::wstring const & runAs,
                uint64 instanceId,
                ServiceModel::CodePackageIsolationPolicyType::Enum const & codeIsolationPolicy);

            static Common::GlobalWString CodePackageSharing;
            static Common::GlobalWString ApplicationSharing;

        private:
            std::wstring processContext_;
            std::wstring userContext_;
    };
}
