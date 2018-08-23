// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Fabric.Interop;
    using System.Security.Cryptography.X509Certificates;

    internal class CertificateManager
    {
        //DateTime expected in local time
        public static void GenerateSelfSignedCertAndImportToStore(string subName, string store, StoreLocation storeLocation, string DNS, DateTime expirationDate)
        {
            using (var pin = new PinCollection())
            {
                string profile;
                if (storeLocation == StoreLocation.CurrentUser)
                    profile = "U";
                else
                    profile = "M";
                NativeCommon.GenerateSelfSignedCertAndImportToStore(
                    pin.AddBlittable(subName),
                    pin.AddBlittable(store),
                    pin.AddBlittable(profile),
                    pin.AddBlittable(DNS),
                    NativeTypes.ToNativeFILETIME(expirationDate));
            }
        }

        public static void GenerateSelfSignedCertAndSaveAsPFX(string subName, string fileName,string password, string DNS, DateTime expirationDate)
        {
             using (var pin = new PinCollection())
            {
                NativeCommon.GenerateSelfSignedCertAndSaveAsPFX(
                    pin.AddBlittable(subName),
                    pin.AddBlittable(fileName),
                    pin.AddBlittable(password),
                    pin.AddBlittable(DNS),
                    NativeTypes.ToNativeFILETIME(expirationDate));
            }
        }

        /*Set isExactMatch to true to delete all certificates which "exactly" match certName.
          Set isExactMatch to false to delete all certificates with prefix as certName*/

        public static void DeleteCertificateFromStore(string certName, string store, StoreLocation storeLocation, bool isExactMatch)
        {
            using (var pin = new PinCollection())
            {
                string profile;
                if (storeLocation == StoreLocation.CurrentUser)
                    profile = "U";
                else
                    profile = "M";
                NativeCommon.DeleteCertificateFromStore(
                    pin.AddBlittable(certName),
                    pin.AddBlittable(store),
                    pin.AddBlittable(profile),
                    NativeTypes.ToBOOLEAN(isExactMatch));
            }
        }
    }
}