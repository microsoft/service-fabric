// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    using System;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Cryptography.X509Certificates;

    /// <summary>
    /// <para>Represents the encryption utility.</para>
    /// </summary>
    public sealed class EncryptionUtility
    {
        /// <summary>
        /// <para>Instantiates a new <see cref="System.Fabric.Security.EncryptionUtility"/> class.</para>
        /// </summary>
        public EncryptionUtility()
        {
        }

        #region API

        /// <summary>
        /// <para>Encrypt text string with an installed X509 certificate. Certificate store location is LocalMachine and the encryption algorithm is AES256 CBC.</para>
        /// </summary>
        /// <param name="textToEncrypt">
        /// <para>The text string to encrypt.</para>
        /// </param>
        /// <param name="thumbprint">
        /// <para>The thumbprint of encryption certificate.</para>
        /// </param>
        /// <param name="storeName">
        /// <para>The name of certificate store, from which encryption certificate is retrieved.</para>
        /// </param>
        /// <returns>
        /// <para>The encrypted text as <see cref="System.String" />.</para>
        /// </returns>
        public static string EncryptText(
            string textToEncrypt,
            string thumbprint,
            string storeName)
        {
            return EncryptText(
                textToEncrypt,
                thumbprint,
                storeName,
                StoreLocation.LocalMachine,
                null);
        }

        /// <summary>
        /// <para>Encrypt text string with an installed X509 certificate.</para>
        /// </summary>
        /// <param name="textToEncrypt">
        /// <para>The text to encrypt.</para>
        /// </param>
        /// <param name="thumbprint">
        /// <para>The thumbprint of encryption certificate.</para>
        /// </param>
        /// <param name="storeName">
        /// <para>The name of certificate store, from which encryption certificate is retrieved.</para>
        /// </param>
        /// <param name="storeLocation">
        /// <para>The certificate store location to retrieve encryption certificate.</para>
        /// </param>
        /// <param name="algorithmOid">
        /// <para>The encryption algorithm object identifier (OID).</para>
        /// </param>
        /// <returns>
        /// <para>The encrypted text as <see cref="System.String" />.</para>
        /// </returns>
        public static string EncryptText(
            string textToEncrypt,
            string thumbprint,
            string storeName,
            StoreLocation storeLocation,
            string algorithmOid)
        {
            using (PinCollection pin = new PinCollection())
            {
                try
                {
                    return NativeTypes.FromNativeString(NativeCommon.FabricEncryptText(
                        pin.AddObject(textToEncrypt),
                        pin.AddObject(thumbprint),
                        pin.AddObject(storeName),
                        (NativeTypes.FABRIC_X509_STORE_LOCATION)storeLocation,
                        pin.AddObject(algorithmOid)));
                }
                catch (Exception ex)
                {
                    COMException comEx = Utility.TryTranslateExceptionToCOM(ex);
                    if (comEx != null)
                    {
                        throw comEx;
                    }
                    throw;
                }
            }
        }

        /// <summary>
        /// <para>Encrypt text string with an X509 certificate in a file.</para>
        /// </summary>
        /// <param name="textToEncrypt">
        /// <para>The text to encrypt.</para>
        /// </param>
        /// <param name="certFileName">
        /// <para>The encryption certificate file path.</para>
        /// </param>
        /// <param name="algorithmOid">
        /// <para>The encryption algorithm object identifier (OID).</para>
        /// </param>
        /// <returns>
        /// <para>The encrypted text as <see cref="System.String" />.</para>
        /// </returns>
        public static string EncryptTextByCertFile(
            string textToEncrypt,
            string certFileName,
            string algorithmOid)
        {
            using (PinCollection pin = new PinCollection())
            {
                try
                {
                    return NativeTypes.FromNativeString(NativeCommon.FabricEncryptText2(
                        pin.AddObject(textToEncrypt),
                        pin.AddObject(certFileName),
                        pin.AddObject(algorithmOid)));

                }
                catch (Exception ex)
                {
                    COMException comEx = Utility.TryTranslateExceptionToCOM(ex);
                    if (comEx != null)
                    {
                        throw comEx;
                    }
                    throw;
                }
            }
        }

        /// <summary>
        /// <para>Decrypt a text string encrypted by EncryptText methods of <see cref="System.Fabric.Security.EncryptionUtility" />, it is assumed that the store location of decryption certificate is LocalMachine.</para>
        /// </summary>
        /// <param name="textToDecrypt">
        /// <para>The encrypted text to decrypt.</para>
        /// </param>
        /// <returns>
        /// <para>The decrypted text as <see cref="System.Security.SecureString " />.</para>
        /// </returns>
        public static SecureString DecryptText(string textToDecrypt)
        {
            return DecryptText(textToDecrypt, StoreLocation.LocalMachine);
        }

        /// <summary>
        /// <para>Decrypt a text string encrypted by EncryptText methods of <see cref="System.Fabric.Security.EncryptionUtility" />.</para>
        /// </summary>
        /// <param name="textToDecrypt">
        /// <para>The encrypted text to decrypt.</para>
        /// </param>
        /// <param name="storeLocation">
        /// <para>The certificate store location to retrieve decryption certificate.</para>
        /// </param>
        /// <returns>
        /// <para>The decrypted text as <see cref="System.Security.SecureString " />.</para>
        /// </returns>
        public static SecureString DecryptText(string textToDecrypt, StoreLocation storeLocation)
        {
            using (PinCollection pin = new PinCollection())
            {
                try
                {
                    return StringResult.FromNativeToSecureString(NativeCommon.FabricDecryptText(
                        pin.AddObject(textToDecrypt),
                        (NativeTypes.FABRIC_X509_STORE_LOCATION)storeLocation));
                }
                catch (Exception ex)
                {
                    COMException comEx = Utility.TryTranslateExceptionToCOM(ex);
                    if (comEx != null)
                    {
                        throw comEx;
                    }
                    throw;
                }
            }
        }

        /// <summary>
        /// <para>Encrypts string value using specified certificate. This method is deprecated; EncryptText method should be used instead.</para>
        /// </summary>
        /// <param name="thumbprint">
        /// <para>The thumbprint of certificate used to encrypt text.</para>
        /// </param>
        /// <param name="storeLocation">
        /// <para>The StoreName for the certificate. Defaults to "My" store.</para>
        /// </param>
        /// <param name="textToEncrypt">
        /// <para>The text value that needs to be encrypted.</para>
        /// </param>
        /// <returns>
        /// <para>The encrypted text as <see cref="System.String" />.</para>
        /// </returns>
        [Obsolete("Deprecated by EncryptText", false)]
        public static string EncryptValue(string thumbprint, string storeLocation, string textToEncrypt)
        {
            using (PinCollection pin = new PinCollection())
            {
                return NativeTypes.FromNativeString(NativeCommon.FabricEncryptText(
                    pin.AddObject(textToEncrypt),
                    pin.AddObject(thumbprint),
                    pin.AddObject(storeLocation),
                    NativeTypes.FABRIC_X509_STORE_LOCATION.FABRIC_X509_STORE_LOCATION_LOCALMACHINE,
                    IntPtr.Zero));
            }
        }

        /// <summary>
        /// <para>Decrypts string values that were encrypted by calling EncryptValue. This method is deprecated; EncryptText method should be used instead.</para>
        /// </summary>
        /// <param name="textToDecrypt">
        /// <para>The string value to decrypt.</para>
        /// </param>
        /// <returns>
        /// <para>The decrypted value.</para>
        /// </returns>
        [Obsolete("Deprecated DecryptText", false)]
        public static string DecryptValue(string textToDecrypt)
        {
            using (PinCollection pin = new PinCollection())
            {
                return NativeTypes.FromNativeString(NativeCommon.FabricDecryptText(
                    pin.AddObject(textToDecrypt), NativeTypes.FABRIC_X509_STORE_LOCATION.FABRIC_X509_STORE_LOCATION_LOCALMACHINE));
            }
        }

        #endregion
    }
}