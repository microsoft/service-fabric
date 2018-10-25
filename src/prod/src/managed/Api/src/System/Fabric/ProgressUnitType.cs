// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// Describes the unit of measurement for each <see cref="System.Fabric.IImageStoreProgressHandler.UpdateProgress"/> callback.
    /// </summary>
    public enum ProgressUnitType
    {
        /// <summary>
        /// Reserved for internal use. May indicate a mismatch between managed and native DLL versions.
        /// </summary>
        Invalid = NativeImageStore.FABRIC_PROGRESS_UNIT_TYPE.FABRIC_PROGRESS_UNIT_TYPE_INVALID,

        /// <summary>
        /// The unit of measurement is in bytes.
        /// </summary>
        Bytes = NativeImageStore.FABRIC_PROGRESS_UNIT_TYPE.FABRIC_PROGRESS_UNIT_TYPE_BYTES,
            
        /// <summary>
        /// The unit of measurement is in number of code, config, and data packages.
        /// </summary>
        ServiceSubPackages = NativeImageStore.FABRIC_PROGRESS_UNIT_TYPE.FABRIC_PROGRESS_UNIT_TYPE_SERVICE_SUB_PACKAGES,
            
        /// <summary>
        /// The unit of measurement is in number of files.
        /// </summary>
        Files = NativeImageStore.FABRIC_PROGRESS_UNIT_TYPE.FABRIC_PROGRESS_UNIT_TYPE_FILES 
    }

    internal class ProgressUnitTypeHelper
    {
        public static ProgressUnitType FromNative(NativeImageStore.FABRIC_PROGRESS_UNIT_TYPE native)
        {
            switch (native)
            {
                case NativeImageStore.FABRIC_PROGRESS_UNIT_TYPE.FABRIC_PROGRESS_UNIT_TYPE_BYTES:
                case NativeImageStore.FABRIC_PROGRESS_UNIT_TYPE.FABRIC_PROGRESS_UNIT_TYPE_SERVICE_SUB_PACKAGES:
                case NativeImageStore.FABRIC_PROGRESS_UNIT_TYPE.FABRIC_PROGRESS_UNIT_TYPE_FILES:
                    return (ProgressUnitType)native;

                default:
                    return ProgressUnitType.Invalid;
            }
        }
    }
}