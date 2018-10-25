// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;
    using System.Fabric.Result;
    using System.Text;

    internal class OrchestrationUpgradeResult : NativeClient.IFabricOrchestrationUpgradeStatusResult
    {
        private FabricOrchestrationUpgradeProgress progress;

        public OrchestrationUpgradeResult(FabricOrchestrationUpgradeProgress progress)
        {
            this.progress = progress;
        }

        public unsafe IntPtr get_Progress()
        {
            using (PinCollection pin = new PinCollection())
            {
                var nativeResult = this.progress.ToNative(pin);
                return nativeResult;
            }
        }
    }
}