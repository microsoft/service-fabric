// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.IO;

namespace System.Fabric
{
    using System;
    using System.Globalization;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Threading;
    using System.Threading.Tasks;
    using BOOLEAN = System.SByte;
    using System.Reflection;

    // This class is used to expose the native V2 store stack using the managed KVS API.
    // Our internal APIs (e.g. used by Actor framework) are split into an internal 
    // Microsoft.ServiceFabric.Internal.dll separate from the public System.Fabric.dll and
    // this class is only available in Microsoft.ServiceFabric.Internal.dll. 
    //
    // If we decide to expose the native V2 store stack publicly using the managed KVS API, 
    // then this class will be refactored to remove usage of the OverrideNativeKeyValueStore_V2 hook.
    //
    // This class is currently used by the KVS actor state provider and Upgrade Service.
    //
    internal class KeyValueStoreReplica_V2 : KeyValueStoreReplica
    {
        // Used by FabricUS
        //
        public KeyValueStoreReplica_V2(
            string workingDirectory,
            string storeDirectory,
            string storeName,
            Guid partitionId)
            : this(
                storeName, 
                new ReplicatorSettings(), 
                new KeyValueStoreReplicaSettings_V2(
                    workingDirectory,
                    storeDirectory,
                    storeName,
                    partitionId)) 
        {
        }

        // Used by KvsActorStateProvider_V2
        //
        public KeyValueStoreReplica_V2(
            ReplicatorSettings replicatorSettings,
            KeyValueStoreReplicaSettings_V2 kvsSettings)
            : this(
                "KVS_V2", // Unused ESE storeName
                replicatorSettings,
                kvsSettings)
        {
        }

        private KeyValueStoreReplica_V2(
            string storeName,
            ReplicatorSettings replicatorSettings,
            KeyValueStoreReplicaSettings_V2 kvsSettings)
            : base(storeName, replicatorSettings)
        {
            Requires.Argument<KeyValueStoreReplicaSettings_V2>("kvsSettings", kvsSettings).NotNull();

            this.KeyValueStoreReplicaSettings_V2 = kvsSettings;
        }

        public KeyValueStoreReplicaSettings_V2 KeyValueStoreReplicaSettings_V2 { get; private set; }

        public override SecondaryNotificationMode NotificationMode 
        { 
            get 
            { 
                return this.KeyValueStoreReplicaSettings_V2.SecondaryNotificationMode;
            } 
        }

        public void Initialize_OverrideNativeKeyValueStore(StatefulServiceInitializationParameters initParams)
        {
            using (var pin = new PinCollection())
            {
                var kvsSettings = this.KeyValueStoreReplicaSettings_V2;
                var nativeKvsSettings = kvsSettings.ToNative(pin);

                // Microsoft.ServiceFabric.Internal does not have friend access to System.Fabric.
                // Use reflection to access hook and override underlying native store implementation.
                //
                var method = typeof(KeyValueStoreReplica).GetMethod(
                    "OverrideNativeKeyValueStore_V2", 
                    BindingFlags.NonPublic | BindingFlags.Instance); 

                // We are invoking a method on KeyValueStoreReplica in System.Fabric.dll from 
                // Microsoft.ServiceFabric.Internal.dll so the argument must be a type common to both dlls.
                //
                method.Invoke(this, new object[] { nativeKvsSettings });

                this.Initialize(initParams);
            }
        }
    }
}