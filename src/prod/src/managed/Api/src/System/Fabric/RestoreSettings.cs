// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Interop;
    using System.Threading;

    /// <summary>
    /// Represents the settings for a key/value store <see cref="System.Fabric.KeyValueStoreReplica.RestoreAsync(string, System.Fabric.RestoreSettings, System.Threading.CancellationToken)" /> operation.
    /// </summary>
    public sealed class RestoreSettings
    {
        private readonly bool inlineReopen;
        private readonly bool enableLsnCheck;
        
        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.RestoreSettings"/> class.
        /// </summary>
        public RestoreSettings()
        {
            this.inlineReopen = false;
            this.enableLsnCheck = false;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.RestoreSettings"/> class.
        /// </summary>
        /// <param name="inlineReopen">
        /// Indicates if the <see cref="System.Fabric.KeyValueStoreReplica"/> should re-open itself
        /// after it has restored successfully from the supplied backup. If false is
        /// specified, the replica reports transient fault after successful restore. 
        /// </param>
        public RestoreSettings(bool inlineReopen)
        {
            this.inlineReopen = inlineReopen;
            this.enableLsnCheck = false;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="System.Fabric.RestoreSettings"/> class. 
        /// </summary>
        /// <param name="inlineReopen">
        /// Indicates if the <see cref="System.Fabric.KeyValueStoreReplica"/> should re-open itself
        /// after it has restored successfully from the supplied backup. If false is
        /// specified, the replica reports transient fault after successful restore.  
        /// </param>
        /// <param name="enableLsnCheck">
        /// Indicates if the <see cref="System.Fabric.KeyValueStoreReplica"/> should check that  
        /// restore data is not older than data currently present in the service.
        /// This protects against accidental data loss. If false is specified, 
        /// <see cref="System.Fabric.KeyValueStoreReplica"/> will overwrite current service data with
        /// restore data even if data present in service in newer. 
        /// </param>
        public RestoreSettings(bool inlineReopen, bool enableLsnCheck)
        {
            this.inlineReopen = inlineReopen;
            this.enableLsnCheck = enableLsnCheck;
        }

        /// <summary>
        /// Indicates if the <see cref="System.Fabric.KeyValueStoreReplica"/> should re-open itself
        /// after it has restored successfully from the supplied backup. If false is
        /// specified, the replica reports transient fault after successful restore. 
        /// </summary>
        /// <value>
        /// A <see cref="bool"/> value indicating if <see cref="System.Fabric.KeyValueStoreReplica"/> will 
        /// re-open itself after the restore.
        /// </value>
        public bool InlineReopen
        {
            get { return this.inlineReopen; }
        }

        /// <summary>
        /// Indicates if the <see cref="System.Fabric.KeyValueStoreReplica"/> should check that  
        /// restore data is not older than data currently present in the service.
        /// This protects against accidental data loss. If false is specified, 
        /// <see cref="System.Fabric.KeyValueStoreReplica"/> will overwrite current service data with
        /// restore data even if data present in service in newer. 
        /// </summary>
        /// /// <value>
        /// A <see cref="bool"/> value indicating if <see cref="System.Fabric.KeyValueStoreReplica"/> will 
        /// check if restore data is not older than data currently present in the service.
        /// </value>
        public bool EnableLsnCheck
        {
            get { return this.enableLsnCheck; }
        }

        internal IntPtr ToNative(PinCollection pin)
        {
            var nativeSettings = new NativeTypes.FABRIC_KEY_VALUE_STORE_RESTORE_SETTINGS[1];
            nativeSettings[0].InlineReopen = NativeTypes.ToBOOLEAN(this.inlineReopen);

            var ex1Settings = new NativeTypes.FABRIC_KEY_VALUE_STORE_RESTORE_SETTINGS_EX1[1];
            ex1Settings[0].EnableLsnCheck = NativeTypes.ToBOOLEAN(this.enableLsnCheck);

            nativeSettings[0].Reserved = pin.AddBlittable(ex1Settings);

            return pin.AddBlittable(nativeSettings);
        }
    }
}