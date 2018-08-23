// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Globalization;
    using System.IO;
    using System.Fabric.Interop;

    using BOOLEAN = System.SByte;
    internal static class FileSignatureVerifier
    {
        public static bool IsSignatureValid(string filename)
        {
            using (var pin = new PinCollection())
            {
                BOOLEAN isValid;
                NativeCommon.VerifyFileSignature(pin.AddBlittable(filename), out isValid);
                return NativeTypes.FromBOOLEAN(isValid);
            }
        }       
    }
}