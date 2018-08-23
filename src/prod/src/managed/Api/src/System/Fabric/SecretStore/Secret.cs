// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.SecretStore
{
    using System.Fabric.Interop;
    using System.Security;

    /// <summary>
    /// Represent a secret type.
    /// </summary>
    public sealed class Secret
    {
        /// <summary>
        /// The fully qualfied name of the secret.
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// The value of the secret.
        /// </summary>
        public SecureString Value { get; set; }

        /// <summary>
        /// The version of the secret.
        /// </summary>
        public string Version { get; set; }

        internal static unsafe Secret FromNative(NativeTypes.FABRIC_SECRET nativeSecret)
        {
            var secret = new Secret();

            secret.Name = NativeTypes.FromNativeString(nativeSecret.Name);
            secret.Version = NativeTypes.FromNativeString(nativeSecret.Version);

            if (nativeSecret.Value != IntPtr.Zero)
            {
                secret.Value = NativeTypes.FromNativeToSecureString(nativeSecret.Value);
            }

            return secret;
        }

        internal static unsafe Secret[] FromNativeArray(IntPtr nativeListPtr)
        {
            if (nativeListPtr == IntPtr.Zero)
            {
                throw new ArgumentNullException(nameof(nativeListPtr));
            }

            var nativeList = (NativeTypes.FABRIC_SECRET_LIST*)nativeListPtr;

            if (nativeList->Count < 0)
            {
                throw new ArgumentOutOfRangeException("nativeList.Count", "Count of the list must be equal to or greater than zero.");
            }

            var managedArray = new Secret[nativeList->Count];
            var nativeItems = (NativeTypes.FABRIC_SECRET*)nativeList->Items;

            for (int itemIndex = 0; itemIndex < nativeList->Count; itemIndex++)
            {
                managedArray[itemIndex] = FromNative(nativeItems[itemIndex]);
            }

            return managedArray;
        }

        internal static unsafe IntPtr ToNativeArray(PinCollection pinCollection, Secret[] secretRefs)
        {
            if (secretRefs == null)
            {
                throw new ArgumentNullException(nameof(secretRefs));
            }

            var nativeArray = new NativeTypes.FABRIC_SECRET[secretRefs.Length];

            for (int itemIndex = 0; itemIndex < secretRefs.Length; itemIndex++)
            {
                nativeArray[itemIndex] = secretRefs[itemIndex].ToNative(pinCollection);
            }

            var nativeList = new NativeTypes.FABRIC_SECRET_LIST();

            nativeList.Count = (uint)secretRefs.Length;
            nativeList.Items = pinCollection.AddBlittable(nativeArray);

            return pinCollection.AddBlittable(nativeList);
        }

        internal NativeTypes.FABRIC_SECRET ToNative(PinCollection pinCollection)
        {
            var nativeSecret = new NativeTypes.FABRIC_SECRET();

            nativeSecret.Name = pinCollection.AddObject(this.Name);
            nativeSecret.Version = pinCollection.AddObject(this.Version);
            nativeSecret.Value = pinCollection.AddObject(this.Value);

            return nativeSecret;
        }
    }
}
