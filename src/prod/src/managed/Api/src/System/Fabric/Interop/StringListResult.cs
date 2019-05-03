// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using SecureString = System.Security.SecureString;

    [SuppressMessage(FxCop.Category.Design, FxCop.Rule.TypesThatOwnDisposableFieldsShouldBeDisposable,
            Justification = "Interop class to be obsoleted after Windows Fabric transition.")]
    internal class StringListResult : NativeCommon.IFabricStringListResult
    {
        private PinCollection pinCollection;
        private UInt32 count;
        private IntPtr items;

        public StringListResult(IList<string> list)
        {
            pinCollection = new PinCollection();
            items = NativeTypes.ToNativeStringPointerArray(pinCollection, list);
            count = (UInt32)list.Count;
        }

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
        public IntPtr GetStrings(out UInt32 itemCount)
        {
            itemCount = count;
            return items;
        }
    }
}