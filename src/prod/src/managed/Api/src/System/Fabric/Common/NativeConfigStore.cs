// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Reflection;
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Security;
    using System.Security.Cryptography.X509Certificates;
    using BOOLEAN = System.SByte;
    using System.IO;
    using System.Fabric.Common.Tracing;

    internal class NativeConfigStore : IConfigStore
    {
        private NativeCommon.IFabricConfigStore2 nativeConfigStore;

        private NativeConfigStore(NativeCommon.IFabricConfigStore2 nativeConfigStore)
        {
            this.nativeConfigStore = nativeConfigStore;
        }

        public static NativeConfigStore FabricGetConfigStore()
        {
            return NativeConfigStore.FabricGetConfigStore(null);
        }

        public static NativeConfigStore FabricGetConfigStore(IConfigStoreUpdateHandler updateHandler)
        {
            NativeCommon.IFabricConfigStoreUpdateHandler nativeUpdateHandler = null;
            if (updateHandler != null)
            {
                nativeUpdateHandler = new ConfigStoreUpdateHandlerBroker(updateHandler);
            }

            return Utility.WrapNativeSyncInvokeInMTA(() => NativeConfigStore.CreateHelper(nativeUpdateHandler), "NativeConfigStore.FabricGetConfigStore");
        }

        public static SecureString DecryptText(string encryptedValue)
        {
            return DecryptText(encryptedValue, StoreLocation.LocalMachine);
        }

        public static SecureString DecryptText(string encryptedValue, StoreLocation storeLocation)
        {
            if (encryptedValue == null)
            {
                throw new ArgumentException(StringResources.Error_NullEncryptedValue);
            }

            return Utility.WrapNativeSyncInvokeInMTA(() => DecryptTextHelper(encryptedValue, storeLocation), "NativeConfigStore.FabricDecryptText");
        }

        public static string EncryptText(
            string text,
            string certThumbPrint,
            string certStoreName)
        {
            return EncryptText(text, certThumbPrint, certStoreName, StoreLocation.LocalMachine, null);
        }

        public static string EncryptText(
            string text,
            string certThumbPrint,
            string certStoreName,
            StoreLocation certStoreLocation,
            string algorithmOid)
        {
            if (string.IsNullOrEmpty(text))
            {
                throw new ArgumentException(StringResources.Error_NullEncryptedText);
            }

            if (string.IsNullOrEmpty(certThumbPrint))
            {
                throw new ArgumentException(StringResources.Error_InvalidX509Thumbprint);
            }

            if (string.IsNullOrEmpty(certStoreName))
            {
                throw new ArgumentException(StringResources.Error_InvalidX509StoreLocation);
            }

            return Utility.WrapNativeSyncInvokeInMTA(() => EncryptTextHelper(text, certThumbPrint, certStoreName, certStoreLocation, algorithmOid), "NativeConfigStore.FabricEncryptText");
        }

        public static string EncryptTextByCertFile(
            string text,
            string certFilePath,
            string algorithmOid)
        {
            if (string.IsNullOrEmpty(text))
            {
                throw new ArgumentException(StringResources.Error_NullEncryptedText);
            }

            if (string.IsNullOrEmpty(certFilePath))
            {
                throw new ArgumentException(StringResources.Error_InvalidX509Thumbprint);
            }

            return Utility.WrapNativeSyncInvokeInMTA(() => EncryptValueHelper(text, certFilePath, algorithmOid), "NativeConfigStore.FabricEncryptText2");
        }

        public ICollection<string> GetSections(string partialSectionName)
        {
            Requires.Argument("partialSectionName", partialSectionName).NotNullOrEmpty();

            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetSectionsHelper(partialSectionName), "ConfigStore.GetSections");
        }

        public ICollection<string> GetKeys(string sectionName, string partialKeyName)
        {
            Requires.Argument("sectionName", sectionName).NotNullOrEmpty();

            return Utility.WrapNativeSyncInvokeInMTA(() => this.GetKeysHelper(sectionName, partialKeyName), "ConfigStore.GetKeys");
        }

        public string ReadUnencryptedString(string sectionName, string keyName)
        {
            bool isEncrypted;
            string value = this.ReadString(sectionName, keyName, out isEncrypted);

            if (isEncrypted)
            {
                throw new InvalidOperationException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_EncryptedValue_Formatted,
                        sectionName,
                        keyName));
            }

            return value;
        }

        public bool ReadUnencryptedBool(string sectionName, string keyName, FabricEvents.ExtensionsEvents traceSource, string traceType, bool throwIfInvalid)
        {
            NativeConfigStore configStore = NativeConfigStore.FabricGetConfigStore();
            string propertyValueStr = configStore.ReadUnencryptedString(sectionName, keyName);
            bool propertyValue = false;
            traceSource.WriteInfo(traceType, "{0} {1}", keyName, propertyValueStr);
            if (!bool.TryParse(propertyValueStr, out propertyValue))
            {
                string message = string.Format("{0} could not parse: {1}", keyName, propertyValueStr);
                traceSource.WriteError(traceType, message);
                if (throwIfInvalid)
                {
                    throw new InvalidDataException(message);
                }
                else
                {
                    propertyValue = false;
                }
            }

            return propertyValue;
        }

        public string ReadString(string sectionName, string keyName, out bool isEncrypted)
        {
            Requires.Argument("sectionName", sectionName);
            Requires.Argument("keyName", keyName);

            bool isEncryptedOut = false;
            var retval = Utility.WrapNativeSyncInvokeInMTA(() => this.ReadStringHelper(sectionName, keyName, out isEncryptedOut), "ConfigStore.ReadString");

            isEncrypted = isEncryptedOut;
            return retval;
        }

        public bool IgnoreUpdateFailures
        {
            get
            {
                return Utility.WrapNativeSyncInvokeInMTA(() => this.get_IgnoreUpdateFailuresHelper(), "ConfigStore.get_IgnoreUpdateFailuresHelper");
            }

            set
            {
                Utility.WrapNativeSyncInvokeInMTA(() => this.set_IgnoreUpdateFailuresHelper(value), "ConfigStore.set_IgnoreUpdateFailuresHelper");
            }
        }

        #region Helper Methods

        private static NativeConfigStore CreateHelper(NativeCommon.IFabricConfigStoreUpdateHandler updateHandler)
        {
            Guid riid = typeof(NativeCommon.IFabricConfigStore2).GetTypeInfo().GUID;
            return new NativeConfigStore(NativeCommon.FabricGetConfigStore(ref riid, updateHandler));
        }

        private static SecureString DecryptTextHelper(string encryptedValue, StoreLocation storeLocation)
        {
            using (var pin = new PinCollection())
            {
                return StringResult.FromNativeToSecureString(NativeCommon.FabricDecryptText(
                    pin.AddBlittable(encryptedValue),
                    (NativeTypes.FABRIC_X509_STORE_LOCATION)storeLocation));
            }
        }

        private static string EncryptTextHelper(
            string text,
            string certThumbPrint,
            string certStoreName,
            StoreLocation storeLocation,
            string algorithmOid)
        {
            using (var pin = new PinCollection())
            {
                return StringResult.FromNative(NativeCommon.FabricEncryptText(
                    pin.AddBlittable(text),
                    pin.AddBlittable(certThumbPrint),
                    pin.AddBlittable(certStoreName),
                    (NativeTypes.FABRIC_X509_STORE_LOCATION)storeLocation,
                    pin.AddBlittable(algorithmOid)));
            }
        }

        private static string EncryptValueHelper(
            string text,
            string certFilePath,
            string algorithmOid)
        {
            using (var pin = new PinCollection())
            {
                return StringResult.FromNative(NativeCommon.FabricEncryptText2(
                    pin.AddBlittable(text),
                    pin.AddBlittable(certFilePath),
                    pin.AddBlittable(algorithmOid)));
            }
        }

        private string ReadStringHelper(string sectionName, string keyName, out bool isEncrypted)
        {
            using (var pin = new PinCollection())
            {
                BOOLEAN isEncryptedValue;
                string value = StringResult.FromNative(
                    this.nativeConfigStore.ReadString(
                        pin.AddBlittable(sectionName),
                        pin.AddBlittable(keyName),
                        out isEncryptedValue));
                isEncrypted = NativeTypes.FromBOOLEAN(isEncryptedValue);
                return value;
            }
        }

        private ICollection<string> GetKeysHelper(string sectionName, string partialKeyName)
        {
            using (var pin = new PinCollection())
            {
                return StringCollectionResult.FromNative(
                    this.nativeConfigStore.GetKeys(
                        pin.AddBlittable(sectionName),
                        pin.AddBlittable(partialKeyName)));
            }
        }

        private ICollection<string> GetSectionsHelper(string partialSectionName)
        {
            using (var pin = new PinCollection())
            {
                return StringCollectionResult.FromNative(
                    this.nativeConfigStore.GetSections(
                        pin.AddBlittable(partialSectionName)));
            }
        }

        private bool get_IgnoreUpdateFailuresHelper()
        {
            return NativeTypes.FromBOOLEAN(this.nativeConfigStore.get_IgnoreUpdateFailures());
        }

        private void set_IgnoreUpdateFailuresHelper(bool value)
        {
            this.nativeConfigStore.set_IgnoreUpdateFailures(NativeTypes.ToBOOLEAN(value));
        }

	 #endregion

        private sealed class ConfigStoreUpdateHandlerBroker : NativeCommon.IFabricConfigStoreUpdateHandler
        {
            private IConfigStoreUpdateHandler updateHandler;

            public ConfigStoreUpdateHandlerBroker(IConfigStoreUpdateHandler updateHandler)
            {
                this.updateHandler = updateHandler;
            }

            public BOOLEAN OnUpdate(IntPtr sectionName, IntPtr keyName)
            {
                return Utility.WrapNativeSyncMethodImplementation(() => this.OnUpdateHelper(sectionName, keyName), "ConfigStoreUpdateHandlerBroker.OnUpdate");
            }

            private BOOLEAN OnUpdateHelper(IntPtr sectionName, IntPtr keyName)
            {
                return NativeTypes.ToBOOLEAN(
                    this.updateHandler.OnUpdate(
                        NativeTypes.FromNativeString(sectionName),
                        NativeTypes.FromNativeString(keyName)));
            }

            public BOOLEAN CheckUpdate(IntPtr sectionName, IntPtr keyName, IntPtr value, BOOLEAN isEncrypted)
            {
                return Utility.WrapNativeSyncInvoke(() => this.CheckUpdateHelper(sectionName, keyName, value, isEncrypted), "ConfigStoreUpdateHandlerBroker.CheckUpdate");
            }

            private BOOLEAN CheckUpdateHelper(IntPtr sectionName, IntPtr keyName, IntPtr value, BOOLEAN isEncrypted)
            {
                return NativeTypes.ToBOOLEAN(
                    this.updateHandler.CheckUpdate(
                        NativeTypes.FromNativeString(sectionName),
                        NativeTypes.FromNativeString(keyName),
                        NativeTypes.FromNativeString(value),
                        NativeTypes.FromBOOLEAN(isEncrypted)));
            }
        }
    }
}