// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System;
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Fabric.Common;
    using System.Fabric.Security;
    using System.Linq;
    using System.Security.Cryptography.X509Certificates;

    [TestClass]
    public class X509Test
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("leikong")]
        public unsafe void X509CredentialsRoundTripTest0()
        {
            var x509Credentials = new X509Credentials
            {
                FindType = X509FindType.FindByThumbprint,
                FindValue = "ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff",
            };

            var pinCollection = new PinCollection();
            NativeX509CredentialConverter converter = new NativeX509CredentialConverter(x509Credentials);
            var nativeIntPtr = converter.ToNative(pinCollection);
            Assert.AreNotEqual(nativeIntPtr, IntPtr.Zero);

            var fromNative = X509Credentials.CreateFromNative((NativeTypes.FABRIC_X509_CREDENTIALS*) nativeIntPtr);

            Assert.AreEqual(x509Credentials.ProtectionLevel, fromNative.ProtectionLevel);
            Assert.AreEqual(x509Credentials.StoreName, fromNative.StoreName);
            Assert.AreEqual(x509Credentials.StoreLocation, fromNative.StoreLocation);
            Assert.AreEqual(x509Credentials.FindType, fromNative.FindType);
            Assert.AreEqual(x509Credentials.FindValue, fromNative.FindValue);
            Assert.AreEqual(x509Credentials.FindValueSecondary, fromNative.FindValueSecondary);
            Assert.IsTrue(x509Credentials.RemoteCommonNames.SequenceEqual(fromNative.RemoteCommonNames));
            Assert.IsTrue(x509Credentials.IssuerThumbprints.SequenceEqual(fromNative.IssuerThumbprints));
            Assert.IsTrue(x509Credentials.RemoteCertThumbprints.SequenceEqual(fromNative.RemoteCertThumbprints));
            Assert.IsTrue(x509Credentials.RemoteX509Names.SequenceEqual(fromNative.RemoteX509Names));
            Assert.IsTrue(x509Credentials.RemoteCertIssuers.SequenceEqual(fromNative.RemoteCertIssuers));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("leikong")]
        public unsafe void X509CredentialsRoundTripTest1()
        {
            var x509Credentials = new X509Credentials
            {
                RemoteCommonNames = {"AnyStirngIsFine"},
                FindType = X509FindType.FindByThumbprint,
                FindValue = "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf"
            };

            var pinCollection = new PinCollection();
            NativeX509CredentialConverter converter = new NativeX509CredentialConverter(x509Credentials);
            var nativeIntPtr = converter.ToNative(pinCollection);
            Assert.AreNotEqual(nativeIntPtr, IntPtr.Zero);

            var fromNative = X509Credentials.CreateFromNative((NativeTypes.FABRIC_X509_CREDENTIALS*) nativeIntPtr);

            Assert.AreEqual(x509Credentials.ProtectionLevel, fromNative.ProtectionLevel);
            Assert.AreEqual(x509Credentials.StoreName, fromNative.StoreName);
            Assert.AreEqual(x509Credentials.StoreLocation, fromNative.StoreLocation);
            Assert.AreEqual(x509Credentials.FindType, fromNative.FindType);
            Assert.AreEqual(x509Credentials.FindValue, fromNative.FindValue);
            Assert.AreEqual(x509Credentials.FindValueSecondary, fromNative.FindValueSecondary);
            Assert.IsTrue(x509Credentials.RemoteCommonNames.SequenceEqual(fromNative.RemoteCommonNames));
            Assert.IsTrue(x509Credentials.IssuerThumbprints.SequenceEqual(fromNative.IssuerThumbprints));
            Assert.IsTrue(x509Credentials.RemoteCertThumbprints.SequenceEqual(fromNative.RemoteCertThumbprints));
            Assert.IsTrue(x509Credentials.RemoteX509Names.SequenceEqual(fromNative.RemoteX509Names));
            Assert.IsTrue(x509Credentials.RemoteCertIssuers.SequenceEqual(fromNative.RemoteCertIssuers));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("leikong")]
        public unsafe void X509CredentialsRoundTripTest2()
        {
            var x509Credentials = new X509Credentials
            {
                IssuerThumbprints = {"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"},
                FindType = X509FindType.FindByThumbprint,
                FindValue = "ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff",
            };

            var pinCollection = new PinCollection();
            NativeX509CredentialConverter converter = new NativeX509CredentialConverter(x509Credentials);
            var nativeIntPtr = converter.ToNative(pinCollection);
            Assert.AreNotEqual(nativeIntPtr, IntPtr.Zero);

            var fromNative = X509Credentials.CreateFromNative((NativeTypes.FABRIC_X509_CREDENTIALS*) nativeIntPtr);

            Assert.AreEqual(x509Credentials.ProtectionLevel, fromNative.ProtectionLevel);
            Assert.AreEqual(x509Credentials.StoreName, fromNative.StoreName);
            Assert.AreEqual(x509Credentials.StoreLocation, fromNative.StoreLocation);
            Assert.AreEqual(x509Credentials.FindType, fromNative.FindType);
            Assert.AreEqual(x509Credentials.FindValue, fromNative.FindValue);
            Assert.AreEqual(x509Credentials.FindValueSecondary, fromNative.FindValueSecondary);
            Assert.IsTrue(x509Credentials.RemoteCommonNames.SequenceEqual(fromNative.RemoteCommonNames));
            Assert.IsTrue(x509Credentials.IssuerThumbprints.SequenceEqual(fromNative.IssuerThumbprints));
            Assert.IsTrue(x509Credentials.RemoteCertThumbprints.SequenceEqual(fromNative.RemoteCertThumbprints));
            Assert.IsTrue(x509Credentials.RemoteX509Names.SequenceEqual(fromNative.RemoteX509Names));
            Assert.IsTrue(x509Credentials.RemoteCertIssuers.SequenceEqual(fromNative.RemoteCertIssuers));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("leikong")]
        public unsafe void X509CredentialsRoundTripTest3()
        {
            var x509Credentials = new X509Credentials
            {
                RemoteCommonNames = {"cn1"},
                IssuerThumbprints =
                {
                    "ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f1",
                    "ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f2",
                    "ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f3"
                },
                FindType = X509FindType.FindByThumbprint,
                FindValue = "ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff",
            };

            var pinCollection = new PinCollection();
            NativeX509CredentialConverter converter = new NativeX509CredentialConverter(x509Credentials);
            var nativeIntPtr = converter.ToNative(pinCollection);
            Assert.AreNotEqual(nativeIntPtr, IntPtr.Zero);

            var fromNative = X509Credentials.CreateFromNative((NativeTypes.FABRIC_X509_CREDENTIALS*) nativeIntPtr);

            Assert.AreEqual(x509Credentials.ProtectionLevel, fromNative.ProtectionLevel);
            Assert.AreEqual(x509Credentials.StoreName, fromNative.StoreName);
            Assert.AreEqual(x509Credentials.StoreLocation, fromNative.StoreLocation);
            Assert.AreEqual(x509Credentials.FindType, fromNative.FindType);
            Assert.AreEqual(x509Credentials.FindValue, fromNative.FindValue);
            Assert.AreEqual(x509Credentials.FindValueSecondary, fromNative.FindValueSecondary);
            Assert.IsTrue(x509Credentials.RemoteCommonNames.SequenceEqual(fromNative.RemoteCommonNames));
            Assert.IsTrue(x509Credentials.IssuerThumbprints.SequenceEqual(fromNative.IssuerThumbprints));
            Assert.IsTrue(x509Credentials.RemoteCertThumbprints.SequenceEqual(fromNative.RemoteCertThumbprints));
            Assert.IsTrue(x509Credentials.RemoteX509Names.SequenceEqual(fromNative.RemoteX509Names));
            Assert.IsTrue(x509Credentials.RemoteCertIssuers.SequenceEqual(fromNative.RemoteCertIssuers));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("leikong")]
        public unsafe void X509CredentialsRoundTripTest4()
        {
            var x509Credentials = new X509Credentials
            {
                RemoteCertThumbprints =
                {
                    "10 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff",
                    "20 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff",
                    "30 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"
                },
                FindType = X509FindType.FindByThumbprint,
                FindValue = "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf"
            };

            var pinCollection = new PinCollection();
            NativeX509CredentialConverter converter = new NativeX509CredentialConverter(x509Credentials);
            var nativeIntPtr = converter.ToNative(pinCollection);
            Assert.AreNotEqual(nativeIntPtr, IntPtr.Zero);

            var fromNative = X509Credentials.CreateFromNative((NativeTypes.FABRIC_X509_CREDENTIALS*) nativeIntPtr);

            Assert.AreEqual(x509Credentials.ProtectionLevel, fromNative.ProtectionLevel);
            Assert.AreEqual(x509Credentials.StoreName, fromNative.StoreName);
            Assert.AreEqual(x509Credentials.StoreLocation, fromNative.StoreLocation);
            Assert.AreEqual(x509Credentials.FindType, fromNative.FindType);
            Assert.AreEqual(x509Credentials.FindValue, fromNative.FindValue);
            Assert.AreEqual(x509Credentials.FindValueSecondary, fromNative.FindValueSecondary);
            Assert.IsTrue(x509Credentials.RemoteCommonNames.SequenceEqual(fromNative.RemoteCommonNames));
            Assert.IsTrue(x509Credentials.IssuerThumbprints.SequenceEqual(fromNative.IssuerThumbprints));
            Assert.IsTrue(x509Credentials.RemoteCertThumbprints.SequenceEqual(fromNative.RemoteCertThumbprints));
            Assert.IsTrue(x509Credentials.RemoteX509Names.SequenceEqual(fromNative.RemoteX509Names));
            Assert.IsTrue(x509Credentials.RemoteCertIssuers.SequenceEqual(fromNative.RemoteCertIssuers));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("leikong")]
        public unsafe void X509CredentialsRoundTripTest5()
        {
            var x509Credentials = new X509Credentials
            {
                RemoteCommonNames = {"cn1", "cn2"},
                RemoteCertThumbprints = {"10 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"},
                FindType = X509FindType.FindByThumbprint,
                FindValue = "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf"
            };

            var pinCollection = new PinCollection();
            NativeX509CredentialConverter converter = new NativeX509CredentialConverter(x509Credentials);
            var nativeIntPtr = converter.ToNative(pinCollection);
            Assert.AreNotEqual(nativeIntPtr, IntPtr.Zero);

            var fromNative = X509Credentials.CreateFromNative((NativeTypes.FABRIC_X509_CREDENTIALS*) nativeIntPtr);

            Assert.AreEqual(x509Credentials.ProtectionLevel, fromNative.ProtectionLevel);
            Assert.AreEqual(x509Credentials.StoreName, fromNative.StoreName);
            Assert.AreEqual(x509Credentials.StoreLocation, fromNative.StoreLocation);
            Assert.AreEqual(x509Credentials.FindType, fromNative.FindType);
            Assert.AreEqual(x509Credentials.FindValue, fromNative.FindValue);
            Assert.AreEqual(x509Credentials.FindValueSecondary, fromNative.FindValueSecondary);
            Assert.IsTrue(x509Credentials.RemoteCommonNames.SequenceEqual(fromNative.RemoteCommonNames));
            Assert.IsTrue(x509Credentials.IssuerThumbprints.SequenceEqual(fromNative.IssuerThumbprints));
            Assert.IsTrue(x509Credentials.RemoteCertThumbprints.SequenceEqual(fromNative.RemoteCertThumbprints));
            Assert.IsTrue(x509Credentials.RemoteX509Names.SequenceEqual(fromNative.RemoteX509Names));
            Assert.IsTrue(x509Credentials.RemoteCertIssuers.SequenceEqual(fromNative.RemoteCertIssuers));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("leikong")]
        public unsafe void X509CredentialsRoundTripTest6()
        {
            var x509Credentials = new X509Credentials
            {
                IssuerThumbprints = {"1f ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"},
                RemoteCertThumbprints =
                {
                    "10 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff",
                    "20 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"
                },
                FindType = X509FindType.FindByThumbprint,
                FindValue = "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf"
            };

            var pinCollection = new PinCollection();
            NativeX509CredentialConverter converter = new NativeX509CredentialConverter(x509Credentials);
            var nativeIntPtr = converter.ToNative(pinCollection);
            Assert.AreNotEqual(nativeIntPtr, IntPtr.Zero);

            var fromNative = X509Credentials.CreateFromNative((NativeTypes.FABRIC_X509_CREDENTIALS*) nativeIntPtr);

            Assert.AreEqual(x509Credentials.ProtectionLevel, fromNative.ProtectionLevel);
            Assert.AreEqual(x509Credentials.StoreName, fromNative.StoreName);
            Assert.AreEqual(x509Credentials.StoreLocation, fromNative.StoreLocation);
            Assert.AreEqual(x509Credentials.FindType, fromNative.FindType);
            Assert.AreEqual(x509Credentials.FindValue, fromNative.FindValue);
            Assert.AreEqual(x509Credentials.FindValueSecondary, fromNative.FindValueSecondary);
            Assert.IsTrue(x509Credentials.RemoteCommonNames.SequenceEqual(fromNative.RemoteCommonNames));
            Assert.IsTrue(x509Credentials.IssuerThumbprints.SequenceEqual(fromNative.IssuerThumbprints));
            Assert.IsTrue(x509Credentials.RemoteCertThumbprints.SequenceEqual(fromNative.RemoteCertThumbprints));
            Assert.IsTrue(x509Credentials.RemoteX509Names.SequenceEqual(fromNative.RemoteX509Names));
            Assert.IsTrue(x509Credentials.RemoteCertIssuers.SequenceEqual(fromNative.RemoteCertIssuers));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("leikong")]
        public unsafe void X509CredentialsRoundTripTest7()
        {
            var x509Credentials = new X509Credentials
            {
                RemoteCommonNames = {"cn1", "cn2", "cn3"},
                IssuerThumbprints =
                {
                    "1f ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff",
                    "2f ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"
                },
                RemoteCertThumbprints =
                {
                    "10 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff",
                    "20 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff",
                    "30 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"
                },
                FindType = X509FindType.FindByThumbprint,
                FindValue = "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf",
                FindValueSecondary = "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf",
                ProtectionLevel = ProtectionLevel.Sign
            };

            var pinCollection = new PinCollection();
            NativeX509CredentialConverter converter = new NativeX509CredentialConverter(x509Credentials);
            var nativeIntPtr = converter.ToNative(pinCollection);
            Assert.AreNotEqual(nativeIntPtr, IntPtr.Zero);

            var fromNative = X509Credentials.CreateFromNative((NativeTypes.FABRIC_X509_CREDENTIALS*) nativeIntPtr);

            Assert.AreEqual(x509Credentials.ProtectionLevel, fromNative.ProtectionLevel);
            Assert.AreEqual(x509Credentials.StoreName, fromNative.StoreName);
            Assert.AreEqual(x509Credentials.StoreLocation, fromNative.StoreLocation);
            Assert.AreEqual(x509Credentials.FindType, fromNative.FindType);
            Assert.AreEqual(x509Credentials.FindValue, fromNative.FindValue);
            Assert.AreEqual(x509Credentials.FindValueSecondary, fromNative.FindValueSecondary);
            Assert.IsTrue(x509Credentials.RemoteCommonNames.SequenceEqual(fromNative.RemoteCommonNames));
            Assert.IsTrue(x509Credentials.IssuerThumbprints.SequenceEqual(fromNative.IssuerThumbprints));
            Assert.IsTrue(x509Credentials.RemoteCertThumbprints.SequenceEqual(fromNative.RemoteCertThumbprints));
            Assert.IsTrue(x509Credentials.RemoteX509Names.SequenceEqual(fromNative.RemoteX509Names));
            Assert.IsTrue(x509Credentials.RemoteCertIssuers.SequenceEqual(fromNative.RemoteCertIssuers));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("leikong")]
        public unsafe void X509CredentialsRoundTripTest8()
        {
            var x509Credentials = new X509Credentials
            {
                RemoteCommonNames = {"cn1", "cn2", "cn3"},
                FindType = X509FindType.FindByThumbprint,
                FindValue = "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf",
                FindValueSecondary = "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf",
                ProtectionLevel = ProtectionLevel.Sign
            };

            var pinCollection = new PinCollection();
            NativeX509CredentialConverter converter = new NativeX509CredentialConverter(x509Credentials);
            var nativeIntPtr = converter.ToNative(pinCollection);
            Assert.AreNotEqual(nativeIntPtr, IntPtr.Zero);

            var fromNative = X509Credentials.CreateFromNative((NativeTypes.FABRIC_X509_CREDENTIALS*) nativeIntPtr);

            Assert.AreEqual(x509Credentials.ProtectionLevel, fromNative.ProtectionLevel);
            Assert.AreEqual(x509Credentials.StoreName, fromNative.StoreName);
            Assert.AreEqual(x509Credentials.StoreLocation, fromNative.StoreLocation);
            Assert.AreEqual(x509Credentials.FindType, fromNative.FindType);
            Assert.AreEqual(x509Credentials.FindValue, fromNative.FindValue);
            Assert.AreEqual(x509Credentials.FindValueSecondary, fromNative.FindValueSecondary);
            Assert.IsTrue(x509Credentials.RemoteCommonNames.SequenceEqual(fromNative.RemoteCommonNames));
            Assert.IsTrue(x509Credentials.IssuerThumbprints.SequenceEqual(fromNative.IssuerThumbprints));
            Assert.IsTrue(x509Credentials.RemoteCertThumbprints.SequenceEqual(fromNative.RemoteCertThumbprints));
            Assert.IsTrue(x509Credentials.RemoteX509Names.SequenceEqual(fromNative.RemoteX509Names));
            Assert.IsTrue(x509Credentials.RemoteCertIssuers.SequenceEqual(fromNative.RemoteCertIssuers));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("leikong")]
        public unsafe void X509CredentialsRoundTripTest9()
        {
            var x509Credentials = new X509Credentials
            {
                IssuerThumbprints =
                {
                    "1f ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff",
                    "2f ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"
                },
                FindType = X509FindType.FindByThumbprint,
                FindValue = "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf",
                FindValueSecondary = "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf",
                ProtectionLevel = ProtectionLevel.Sign
            };

            var pinCollection = new PinCollection();
            NativeX509CredentialConverter converter = new NativeX509CredentialConverter(x509Credentials);
            var nativeIntPtr = converter.ToNative(pinCollection);
            Assert.AreNotEqual(nativeIntPtr, IntPtr.Zero);

            var fromNative = X509Credentials.CreateFromNative((NativeTypes.FABRIC_X509_CREDENTIALS*) nativeIntPtr);

            Assert.AreEqual(x509Credentials.ProtectionLevel, fromNative.ProtectionLevel);
            Assert.AreEqual(x509Credentials.StoreName, fromNative.StoreName);
            Assert.AreEqual(x509Credentials.StoreLocation, fromNative.StoreLocation);
            Assert.AreEqual(x509Credentials.FindType, fromNative.FindType);
            Assert.AreEqual(x509Credentials.FindValue, fromNative.FindValue);
            Assert.AreEqual(x509Credentials.FindValueSecondary, fromNative.FindValueSecondary);
            Assert.IsTrue(x509Credentials.RemoteCommonNames.SequenceEqual(fromNative.RemoteCommonNames));
            Assert.IsTrue(x509Credentials.IssuerThumbprints.SequenceEqual(fromNative.IssuerThumbprints));
            Assert.IsTrue(x509Credentials.RemoteCertThumbprints.SequenceEqual(fromNative.RemoteCertThumbprints));
            Assert.IsTrue(x509Credentials.RemoteX509Names.SequenceEqual(fromNative.RemoteX509Names));
            Assert.IsTrue(x509Credentials.RemoteCertIssuers.SequenceEqual(fromNative.RemoteCertIssuers));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("leikong")]
        public unsafe void X509CredentialsRoundTripTest10()
        {
            var x509Credentials = new X509Credentials
            {
                FindType = X509FindType.FindByThumbprint,
                FindValue = "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf",
                FindValueSecondary = "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf",
                ProtectionLevel = ProtectionLevel.Sign
            };

            x509Credentials.RemoteX509Names.Add(new X509Name("name1", "1f ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"));
            x509Credentials.RemoteX509Names.Add(new X509Name("name1", "2f ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"));
            x509Credentials.RemoteX509Names.Add(new X509Name("name2", "3f ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"));
            x509Credentials.RemoteX509Names.Add(new X509Name("name3", null));
            Assert.IsTrue(x509Credentials.RemoteX509Names[0].Equals(x509Credentials.RemoteX509Names[0]));
            Assert.IsTrue(x509Credentials.RemoteX509Names[0].Equals(new X509Name("NAME1", "1f ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff")));
            Assert.IsFalse(x509Credentials.RemoteX509Names[0].Equals(x509Credentials.RemoteX509Names[1]));
            Assert.IsFalse(x509Credentials.RemoteX509Names[0].Equals(x509Credentials.RemoteX509Names[2]));
            Assert.IsFalse(x509Credentials.RemoteX509Names[0].Equals(x509Credentials.RemoteX509Names[3]));
            Assert.IsFalse(x509Credentials.RemoteX509Names[0].Equals(null));

            var pinCollection = new PinCollection();
            NativeX509CredentialConverter converter = new NativeX509CredentialConverter(x509Credentials);
            var nativeIntPtr = converter.ToNative(pinCollection); 
            Assert.AreNotEqual(nativeIntPtr, IntPtr.Zero);

            var fromNative = X509Credentials.CreateFromNative((NativeTypes.FABRIC_X509_CREDENTIALS*) nativeIntPtr);

            Assert.AreEqual(x509Credentials.ProtectionLevel, fromNative.ProtectionLevel);
            Assert.AreEqual(x509Credentials.StoreName, fromNative.StoreName);
            Assert.AreEqual(x509Credentials.StoreLocation, fromNative.StoreLocation);
            Assert.AreEqual(x509Credentials.FindType, fromNative.FindType);
            Assert.AreEqual(x509Credentials.FindValue, fromNative.FindValue);
            Assert.AreEqual(x509Credentials.FindValueSecondary, fromNative.FindValueSecondary);
            Assert.IsTrue(x509Credentials.RemoteCommonNames.SequenceEqual(fromNative.RemoteCommonNames));
            Assert.IsTrue(x509Credentials.IssuerThumbprints.SequenceEqual(fromNative.IssuerThumbprints));
            Assert.IsTrue(x509Credentials.RemoteCertThumbprints.SequenceEqual(fromNative.RemoteCertThumbprints));
            Assert.IsTrue(x509Credentials.RemoteX509Names.SequenceEqual(fromNative.RemoteX509Names));
            Assert.IsTrue(x509Credentials.RemoteCertIssuers.SequenceEqual(fromNative.RemoteCertIssuers));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("leikong")]
        public unsafe void X509CredentialsRoundTripTest11()
        {
            var x509Credentials = new X509Credentials
            {
                FindType = X509FindType.FindByThumbprint,
                FindValue = "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf",
                ProtectionLevel = ProtectionLevel.Sign
            };

            x509Credentials.RemoteX509Names.Add(new X509Name("name1", "1f ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"));
            x509Credentials.RemoteX509Names.Add(new X509Name("name1", "2f ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"));
            x509Credentials.RemoteX509Names.Add(new X509Name("name2", "3f ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"));
            x509Credentials.RemoteX509Names.Add(new X509Name("name3", null));

            var pinCollection = new PinCollection();
            NativeX509CredentialConverter converter = new NativeX509CredentialConverter(x509Credentials);
            var nativeIntPtr = converter.ToNative(pinCollection);
            Assert.AreNotEqual(nativeIntPtr, IntPtr.Zero);

            var fromNative = X509Credentials.CreateFromNative((NativeTypes.FABRIC_X509_CREDENTIALS*)nativeIntPtr);

            Assert.AreEqual(x509Credentials.ProtectionLevel, fromNative.ProtectionLevel);
            Assert.AreEqual(x509Credentials.StoreName, fromNative.StoreName);
            Assert.AreEqual(x509Credentials.StoreLocation, fromNative.StoreLocation);
            Assert.AreEqual(x509Credentials.FindType, fromNative.FindType);
            Assert.AreEqual(x509Credentials.FindValue, fromNative.FindValue);
            Assert.AreEqual(x509Credentials.FindValueSecondary, fromNative.FindValueSecondary);
            Assert.IsTrue(x509Credentials.RemoteCommonNames.SequenceEqual(fromNative.RemoteCommonNames));
            Assert.IsTrue(x509Credentials.IssuerThumbprints.SequenceEqual(fromNative.IssuerThumbprints));
            Assert.IsTrue(x509Credentials.RemoteCertThumbprints.SequenceEqual(fromNative.RemoteCertThumbprints));
            Assert.IsTrue(x509Credentials.RemoteX509Names.SequenceEqual(fromNative.RemoteX509Names));
            Assert.IsTrue(x509Credentials.RemoteCertIssuers.SequenceEqual(fromNative.RemoteCertIssuers));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("ratando")]
        public unsafe void X509CredentialsRoundTripTest12()
        {
            var x509Credentials = new X509Credentials
            {
                FindType = X509FindType.FindByThumbprint,
                FindValue = "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf",
                ProtectionLevel = ProtectionLevel.Sign
            };

            x509Credentials.RemoteX509Names.Add(new X509Name("name1", "1f ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"));
            x509Credentials.RemoteX509Names.Add(new X509Name("name1", "2f ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"));
            x509Credentials.RemoteX509Names.Add(new X509Name("name2", "3f ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff"));
            x509Credentials.RemoteX509Names.Add(new X509Name("name3", null));

            x509Credentials.RemoteCertIssuers.Add(new X509IssuerStore("issuer1", new List<string>() { "Root", "My" }));
            x509Credentials.RemoteCertIssuers.Add(new X509IssuerStore("issuer2", new List<string>() { "My" }));

            var pinCollection = new PinCollection();
            NativeX509CredentialConverter converter = new NativeX509CredentialConverter(x509Credentials);
            var nativeIntPtr = converter.ToNative(pinCollection);
            Assert.AreNotEqual(nativeIntPtr, IntPtr.Zero);

            var fromNative = X509Credentials.CreateFromNative((NativeTypes.FABRIC_X509_CREDENTIALS*)nativeIntPtr);

            Assert.AreEqual(x509Credentials.ProtectionLevel, fromNative.ProtectionLevel);
            Assert.AreEqual(x509Credentials.StoreName, fromNative.StoreName);
            Assert.AreEqual(x509Credentials.StoreLocation, fromNative.StoreLocation);
            Assert.AreEqual(x509Credentials.FindType, fromNative.FindType);
            Assert.AreEqual(x509Credentials.FindValue, fromNative.FindValue);
            Assert.AreEqual(x509Credentials.FindValueSecondary, fromNative.FindValueSecondary);
            Assert.IsTrue(x509Credentials.RemoteCommonNames.SequenceEqual(fromNative.RemoteCommonNames));
            Assert.IsTrue(x509Credentials.IssuerThumbprints.SequenceEqual(fromNative.IssuerThumbprints));
            Assert.IsTrue(x509Credentials.RemoteCertThumbprints.SequenceEqual(fromNative.RemoteCertThumbprints));
            Assert.IsTrue(x509Credentials.RemoteX509Names.SequenceEqual(fromNative.RemoteX509Names));
            Assert.IsTrue(x509Credentials.RemoteCertIssuers.SequenceEqual(fromNative.RemoteCertIssuers));
        }
      }
    }