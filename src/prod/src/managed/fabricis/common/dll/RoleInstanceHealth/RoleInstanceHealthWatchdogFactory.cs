// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    internal class RoleInstanceHealthWatchdogFactory : IRoleInstanceHealthWatchdogFactory
    {
        private readonly IConfigSection configSection;
        private readonly IHealthClient healthClient;

        public RoleInstanceHealthWatchdogFactory(IConfigSection configSection, IHealthClient healthClient)
        {
            this.configSection = configSection.Validate("configSection");
            this.healthClient = healthClient.Validate("healthClient");
        }

        public IRoleInstanceHealthWatchdog Create(string configKeyPrefix)
        {
            var watchdog = new RoleInstanceHealthWatchdog(configSection, configKeyPrefix, healthClient);
            return watchdog;
        }
    }
}