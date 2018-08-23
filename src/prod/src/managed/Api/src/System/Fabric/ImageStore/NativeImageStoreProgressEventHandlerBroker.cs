// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageStore
{
    using System.Fabric.Interop;

    internal class NativeImageStoreProgressEventHandlerBroker : NativeImageStore.IFabricNativeImageStoreProgressEventHandler
    {
        private readonly IImageStoreProgressHandler progressHandler;

        public NativeImageStoreProgressEventHandlerBroker(IImageStoreProgressHandler progressHandler)
        {
            this.progressHandler = progressHandler;
        }

        void NativeImageStore.IFabricNativeImageStoreProgressEventHandler.GetUpdateInterval(IntPtr milliseconds)
        {
            if (this.progressHandler == null)
            {
                unsafe
                {
                    *((UInt32*)milliseconds) = 0;
                }
            }
            else
            {
                var interval = this.progressHandler.GetUpdateInterval();

                unsafe
                {
                    *((UInt32*)milliseconds) = (uint)interval.TotalMilliseconds;
                }
            }
        }

        void NativeImageStore.IFabricNativeImageStoreProgressEventHandler.OnUpdateProgress(
            UInt64 completedItems, 
            UInt64 totalItems,
            NativeImageStore.FABRIC_PROGRESS_UNIT_TYPE itemType)
        {
            if (this.progressHandler != null)
            {
                // unsigned integers are not CLS-compliant
                //
                this.progressHandler.UpdateProgress((long)completedItems, (long)totalItems, ProgressUnitTypeHelper.FromNative(itemType));
            }
        }
    }
}