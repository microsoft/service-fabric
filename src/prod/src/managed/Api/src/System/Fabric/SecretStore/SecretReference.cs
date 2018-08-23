// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.SecretStore
{
    using System.Fabric.Interop;

    /// <summary>
    /// Represent a reference to a secret type.
    /// </summary>
    public sealed class SecretReference
    {
        /// <summary>
        /// The full qualified name of the secret referenced.
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// The version of the secret referenced.
        /// </summary>
        public string Version { get; set; }

        internal static unsafe SecretReference FromNative(NativeTypes.FABRIC_SECRET_REFERENCE nativeSecretReference)
        {
            var secretRef = new SecretReference();

            secretRef.Name = NativeTypes.FromNativeString(nativeSecretReference.Name);
            secretRef.Version = NativeTypes.FromNativeString(nativeSecretReference.Version);

            return secretRef;
        }

        internal static unsafe SecretReference[] FromNativeArray(IntPtr nativeListPtr)
        {
            if (nativeListPtr == IntPtr.Zero)
            {
                throw new ArgumentNullException(nameof(nativeListPtr));
            }

            var nativeList = (NativeTypes.FABRIC_SECRET_REFERENCE_LIST*)nativeListPtr;

            if (nativeList->Count < 0)
            {
                throw new ArgumentOutOfRangeException("nativeList.Count", "Count of the list must be equal to or greater than zero.");
            }

            var managedArray = new SecretReference[nativeList->Count];
            var nativeItems = (NativeTypes.FABRIC_SECRET_REFERENCE*)nativeList->Items;

            for (int itemIndex = 0; itemIndex < nativeList->Count; itemIndex++)
            {
                managedArray[itemIndex] = FromNative(nativeItems[itemIndex]);
            }

            return managedArray;
        }

        internal static unsafe IntPtr ToNativeArray(PinCollection pinCollection, SecretReference[] secretRefs)
        {
            if (secretRefs == null)
            {
                throw new ArgumentNullException(nameof(secretRefs));
            }

            var nativeArray = new NativeTypes.FABRIC_SECRET_REFERENCE[secretRefs.Length];

            for (int itemIndex = 0; itemIndex < secretRefs.Length; itemIndex++)
            {
                nativeArray[itemIndex] = secretRefs[itemIndex].ToNative(pinCollection);
            }

            var nativeList = new NativeTypes.FABRIC_SECRET_REFERENCE_LIST();

            nativeList.Count = (uint)secretRefs.Length;
            nativeList.Items = pinCollection.AddBlittable(nativeArray);

            return pinCollection.AddBlittable(nativeList);
        }

        internal NativeTypes.FABRIC_SECRET_REFERENCE ToNative(PinCollection pinCollection)
        {
            var nativeSecretRef = new NativeTypes.FABRIC_SECRET_REFERENCE();

            nativeSecretRef.Name = pinCollection.AddObject(this.Name);
            nativeSecretRef.Version = pinCollection.AddObject(this.Version);

            return nativeSecretRef;
        }
    }
}
