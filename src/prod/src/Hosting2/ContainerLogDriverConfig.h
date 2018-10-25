// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ContainerLogDriverConfig : public Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(ContainerLogDriverConfig, "ContainerLogDriver")

        /*
        Example of ClusterManifest section
        <ContainerLogDriver>

        </ContainerLogDriver>
        */

        // Names here match Log ServiceModel/LogConfigDescription.h
        // These match up to Docker's ContainerCreate REST API https://docs.docker.com/engine/api/v1.37/#operation/ContainerCreate
        // Type specifies the log driver to be used (json-file, syslog.. etc) https://docs.docker.com/config/containers/logging/configure/#supported-logging-drivers
        // Config is the configuration given to the log driver. Configuration requirements differ depending on the type of log driver used. For supported configuration values,
        // See the specific log driver documentation. For example, for json-file log driver, https://docs.docker.com/config/containers/logging/json-file/#usage
        // if the log driver is not specified then we will let docker use it's default log driver.
        INTERNAL_CONFIG_ENTRY(std::wstring, L"ContainerLogDriver", Type, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(std::wstring, L"ContainerLogDriver", Config, L"{\"Config\":{}}", Common::ConfigEntryUpgradePolicy::Dynamic);
    };
}
