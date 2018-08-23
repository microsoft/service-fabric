// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal class WindowsPrincipalIdentifier : PrincipalIdentifier
    {
        public WindowsPrincipalIdentifier(string accountName)
            : base(PrincipalIdentifierKind.Windows)
        {
            Requires.Argument<string>("accountName", accountName).NotNullOrEmpty();
            this.AccountName = accountName;
        }

        public string AccountName 
        { 
            get; 
            private set; 
        }

        internal override IntPtr ToNativeValue(PinCollection pinCollection)
        {
            var nativeWindowsPrincipalIdentifier = new NativeTypes.FABRIC_SECURITY_WINDOWS_PRINCIPAL_IDENTIFIER();
            nativeWindowsPrincipalIdentifier.AccountName = pinCollection.AddObject(this.AccountName);

            return pinCollection.AddBlittable(nativeWindowsPrincipalIdentifier);
        }

        internal static unsafe WindowsPrincipalIdentifier FromNative(NativeTypes.FABRIC_SECURITY_WINDOWS_PRINCIPAL_IDENTIFIER* nativePtr)
        {
            var accountName = NativeTypes.FromNativeString(nativePtr->AccountName);
            return new WindowsPrincipalIdentifier(accountName);
        }
    }
}