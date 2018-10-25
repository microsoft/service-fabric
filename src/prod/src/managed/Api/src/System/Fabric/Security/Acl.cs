// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Collections.Generic;

   internal class Acl
   {
        public Acl()
            : this(new List<Ace>())
        {
        }

        public Acl(IList<Ace> aceItems)
        {
            Requires.Argument<IList<Ace>>("aceItems", aceItems).NotNull();
            this.AceItems = aceItems;
        }

        public IList<Ace> AceItems 
        { 
            get; 
            private set; 
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeAceArray = new NativeTypes.FABRIC_SECURITY_ACE[this.AceItems.Count];
            for (int i = 0; i < this.AceItems.Count; ++i)
            {
                this.AceItems[i].ToNative(pinCollection, out nativeAceArray[i]);
            }

            var nativeAcl = new NativeTypes.FABRIC_SECURITY_ACL();
            nativeAcl.AceCount = (uint)nativeAceArray.Length;
            nativeAcl.AceItems = pinCollection.AddBlittable(nativeAceArray);
            nativeAcl.Reserved = IntPtr.Zero;

            return pinCollection.AddBlittable(nativeAcl);
        }
       
        internal static unsafe Acl FromNative(NativeTypes.FABRIC_SECURITY_ACL* nativePtr)
        {
            var retval = new Acl();

            if (nativePtr->AceCount > 0)
            {
                var nativeAceArray = (NativeTypes.FABRIC_SECURITY_ACE*)nativePtr->AceItems;
                for (int i = 0; i < nativePtr->AceCount; ++i)
                {
                    var nativeAce = *(nativeAceArray + i);
                    retval.AceItems.Add(Ace.FromNative(nativeAce));
                }
            }

            return retval;
        }

        internal static unsafe Acl FromNativeResult(NativeClient.IFabricGetAclResult nativeResult)
        {
            var retval = Acl.FromNative((NativeTypes.FABRIC_SECURITY_ACL*)nativeResult.get_Acl());
            GC.KeepAlive(nativeResult);

            return retval;
        }
    }
}