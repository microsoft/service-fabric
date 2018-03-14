// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Utility2
        {
            class SystemServiceHelper
            {
            public:
                class HostType
                {
                public:
                    enum Enum
                    {
                        Fabric = 0,
                        Managed = 1,
                        AdHoc = 2,
                    };
                };

                class ServiceType
                {
                public:
                    enum Enum
                    {
                        System,
                        User
                    };
                };

                static HostType::Enum GetHostType(
                    FailoverUnit const & ft);

                static HostType::Enum GetHostType(
                    Reliability::FailoverUnitDescription const & ftDesc,
                    std::wstring const & applicationName);

                static ServiceType::Enum GetServiceType(
                    FailoverUnit const & ft);

                static ServiceType::Enum GetServiceType(
                    std::wstring const & serviceName);
            };
        }
    }
}


