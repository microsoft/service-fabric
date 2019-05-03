// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Threading.Tasks;

    internal interface IContainerActivatorService
    {
        void StartEventMonitoring(bool isContainerServiceManaged, UInt64 sinceTime);

        Task<string> ActivateContainerAsync(ContainerActivationArgs activationArgs, TimeSpan timeout);

        Task ExecuteUpdateRoutesAsync(ContainerUpdateRouteArgs updateRouteArgs, TimeSpan timeout);

        Task DeactivateContainerAsync(ContainerDeactivationArgs deactivationArgs, TimeSpan timeout);

        Task DownloadImagesAsync(List<ContainerImageDescription> imageDescriptions, TimeSpan timeout);

        Task DeleteImagesAsync(List<string> imagesNames, TimeSpan timeout);

        Task<ContainerApiExecutionResponse> InvokeContainerApiAsync(ContainerApiExecutionArgs apiExecArgs, TimeSpan timeout);
    }
}