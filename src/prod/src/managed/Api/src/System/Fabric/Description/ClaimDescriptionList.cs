// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Interop;

    internal sealed class ClaimDescriptionList 
    {
        private ClaimDescription[] claims;
        private Int32 count;
        public ClaimDescriptionList()
        {
            this.claims = null;
            this.count = 0;
        }

        public void AddClaims(List<ClaimDescription> claims)
        {
            this.count = claims.Count;
            this.claims = new ClaimDescription[this.count];
            for (int i = 0; i < claims.Count; i++)
            {
                this.claims[i] = claims[i];
            }
        }

        internal unsafe IntPtr ToNative(PinCollection pin)
        {
            var nativeArray = new NativeTypes.FABRIC_TOKEN_CLAIM[this.count];
            for (int i = 0; i < this.count; ++i)
            {
                this.claims[i].ToNative(pin, out nativeArray[i]);
            }

            var nativeList = new NativeTypes.FABRIC_TOKEN_CLAIM_RESULT_LIST();
            nativeList.Count = nativeArray.Length;
            nativeList.Items = pin.AddBlittable(nativeArray);

            return pin.AddBlittable(nativeList);
        }
    }
}