// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Specifies the optional settings to describe the behavior of transactions supported by <see cref="System.Fabric.KeyValueStoreReplica" />.</para>
    /// </summary>
    public class KeyValueStoreTransactionSettings
    {
        /// <summary>
        /// <para>Creates anew instance of the <see cref="System.Fabric.KeyValueStoreTransactionSettings" /> class..</para>
        /// </summary>
        public KeyValueStoreTransactionSettings()
        {
            this.SerializationBlockSize = 0;
        }

        private int serializationBlockSize;
        /// <summary>
        /// <para>Specifies the block size (in bytes) to use when allocating memory for replication operations.</para>
        /// </summary>
        /// <value>
        /// <para>The block size (in bytes) to use when allocating memory for replication operations.</para>
        /// </value>
        public int SerializationBlockSize 
        { 
            get { return this.serializationBlockSize; } 
            set 
            {
                if (value < 0)
                {
                    throw new ArgumentException(StringResources.Error_Positive_Value_Required);
                }

                this.serializationBlockSize = value;
            } 
        }

        internal IntPtr ToNative(PinCollection pin)
        {
            var nativeSettings = new NativeTypes.FABRIC_KEY_VALUE_STORE_TRANSACTION_SETTINGS[1];
            nativeSettings[0].SerializationBlockSize = (uint)this.SerializationBlockSize;

            return pin.AddBlittable(nativeSettings);
        }
    }
}