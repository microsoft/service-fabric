// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using SecureString = System.Security.SecureString;

    // TODO: evaluate whether we should have an explicit callback to Dispose, or if we should copy to HGLOBAL?
    [SuppressMessage(FxCop.Category.Design, FxCop.Rule.TypesThatOwnDisposableFieldsShouldBeDisposable,
            Justification = "Interop class to be obsoleted after Windows Fabric transition.")]
    internal class StringResult : NativeCommon.IFabricStringResult
    {
        private IPinNode pin;

        public StringResult(string s)
        {
            this.pin = new PinBlittable(s);
        }

        public static string FromNative(NativeCommon.IFabricStringResult nativeResult)
        {
            return NativeTypes.FromNativeString(nativeResult);
        }

        public static SecureString FromNativeToSecureString(NativeCommon.IFabricStringResult nativeResult)
        {
            return NativeTypes.FromNativeToSecureString(nativeResult);
        }

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
        public IntPtr get_String()
        {
            return this.pin.AddrOfPinnedObject();
        }
    }
}