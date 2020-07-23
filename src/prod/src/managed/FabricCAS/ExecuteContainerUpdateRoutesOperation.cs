// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Hosting.ContainerActivatorService
{
    using System;
    using System.Fabric;
    using System.Threading.Tasks;

    internal class ExecuteContainerUpdateRoutesOperation
    {
        #region Private Members

        private const string TraceType = "ExecuteUpdateRoutesOperation";

        private readonly UpdateRoutesContainerOperation updateRoutesOperation;
        private readonly ContainerUpdateRouteArgs updateRouteArgs;
        private readonly TimeoutHelper timeoutHelper;

        #endregion

        public ExecuteContainerUpdateRoutesOperation(
            ContainerActivatorService activator,
            ContainerUpdateRouteArgs updateRouteArgs,
            TimeSpan timeout)
        {
            this.updateRouteArgs = updateRouteArgs;
            this.timeoutHelper = new TimeoutHelper(timeout);

            updateRoutesOperation = new UpdateRoutesContainerOperation(
                TraceType,
                updateRouteArgs.ContainerId,
                updateRouteArgs.ContainerName,
                updateRouteArgs.ApplicationId,
                updateRouteArgs.ApplicationName,
                updateRouteArgs.CgroupName,
                updateRouteArgs.GatewayIpAddresses,
                updateRouteArgs.AutoRemove,
                updateRouteArgs.IsContainerRoot,
                updateRouteArgs.NetworkType,
                activator,
                timeoutHelper);
        }

        public async Task ExecuteAsync()
        {
#if !DotNetCoreClrLinux
            await this.ExecuteUpdateRouteCommandPrivateAsync();
#else
            await Task.CompletedTask;
#endif
        }

#if !DotNetCoreClrLinux
        private async Task ExecuteUpdateRouteCommandPrivateAsync()
        {
            if ((this.updateRouteArgs.NetworkType & ContainerNetworkType.Isolated) == ContainerNetworkType.Isolated)
            {
                await this.updateRoutesOperation.ExecuteUpdateRouteCommandAsync();
            }
        }
#endif
    }
}