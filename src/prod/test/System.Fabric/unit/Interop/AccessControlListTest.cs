// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Interop
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System;
    using System.Fabric.Interop;
    using System.Fabric.Common;
    using System.Fabric.Security;

    [TestClass]
    public class AccessControlListTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("vipulm")]
        public void ResourceIdentifierToAndFromNative()
        {
            ResourceIdentifier resourceIdentifier = new FabricImageStorePathResourceIdentifier("incoming\\MyApplicationType");
            TestToAndFromNative(resourceIdentifier);

            resourceIdentifier = new ApplicationTypeResourceIdentifier("MyApplicationType");
            TestToAndFromNative(resourceIdentifier);

            resourceIdentifier = new ApplicationResourceIdentifier(new Uri("fabric:/System"));
            TestToAndFromNative(resourceIdentifier);

            resourceIdentifier = new ServiceResourceIdentifier(new Uri("fabric:/Samples/QueueApp/QueueService"));
            TestToAndFromNative(resourceIdentifier);

            resourceIdentifier = new NameResourceIdentifier(new Uri("fabric:/TestApplication/HelloWorld/PropertyTables"));
            TestToAndFromNative(resourceIdentifier);
        }


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("vipulm")]
        public void AclToAndFromNative()
        {
            // empty ACL
            var acl = new Acl();
            TestToAndFromNative(acl);

            // one ACE
            acl = new Acl();
            acl.AceItems.Add(new AllowedAce(new RolePrincipalIdentifier("Admin")));
            TestToAndFromNative(acl);

            // multiple ACE
            acl = new Acl();
            acl.AceItems.Add(new AllowedAce(new RolePrincipalIdentifier("Admin"), AccessMask.GenericAll));
            acl.AceItems.Add(new AllowedAce(new RolePrincipalIdentifier("User"), AccessMask.GenericRead));
            TestToAndFromNative(acl);


            // multiple Principal Identifier types
            acl = new Acl();
            acl.AceItems.Add(new AllowedAce(new RolePrincipalIdentifier("Admin"), AccessMask.GenericAll));
            acl.AceItems.Add(new AllowedAce(new RolePrincipalIdentifier("User"), AccessMask.GenericRead));
            acl.AceItems.Add(new AllowedAce(new X509PrincipalIdentifier("users.fabric.com"), AccessMask.Read));
            acl.AceItems.Add(new AllowedAce(new X509PrincipalIdentifier("admins.fabric.com"), AccessMask.Write | AccessMask.Delete | AccessMask.Create | AccessMask.Read));
            TestToAndFromNative(acl);


            // multiple Principal Identifier types
            acl = new Acl();
            acl.AceItems.Add(new AllowedAce(new RolePrincipalIdentifier("Admin"), AccessMask.GenericAll));
            acl.AceItems.Add(new AllowedAce(new RolePrincipalIdentifier("User"), AccessMask.GenericRead));
            acl.AceItems.Add(new AllowedAce(new X509PrincipalIdentifier("users.fabric.com"), AccessMask.Read));
            acl.AceItems.Add(new AllowedAce(new X509PrincipalIdentifier("admins.fabric.com"), AccessMask.Write | AccessMask.Delete | AccessMask.Create | AccessMask.Read));
            acl.AceItems.Add(new AllowedAce(new WindowsPrincipalIdentifier("NTDEV\\someuser"), AccessMask.GenericWrite | AccessMask.ReadAcl));
            acl.AceItems.Add(new AllowedAce(new WindowsPrincipalIdentifier("NTAUTHORITY\\NETWORK SERVICE"), AccessMask.Delete));
            acl.AceItems.Add(new AllowedAce(new ClaimPrincipalIdentifier("SystemAdmin"), 
                                                AccessMask.None | AccessMask.Read | AccessMask.Delete | AccessMask.Create | AccessMask.Write |
                                                AccessMask.GenericAll | AccessMask.GenericRead | AccessMask.GenericWrite | 
                                                AccessMask.ReadAcl | AccessMask.WriteAcl));
            acl.AceItems.Add(new AllowedAce(new ClaimPrincipalIdentifier("AclModifier"), AccessMask.ReadAcl | AccessMask.WriteAcl));
            TestToAndFromNative(acl);
        }

        private unsafe void TestToAndFromNative(Acl acl)
        {
            IntPtr nativePtr = IntPtr.Zero;
            Acl second = null;
            Acl first = acl;

            var firstStr = AccessControlUtility.ToString(first);
            LogHelper.Log("Managed ACL = {0}", firstStr);

            using (var pinCollection = new PinCollection())
            {
                nativePtr = acl.ToNative(pinCollection);
                second = Acl.FromNative((NativeTypes.FABRIC_SECURITY_ACL*)nativePtr);
            }

            var secondStr = AccessControlUtility.ToString(second);
            LogHelper.Log("FromNative ACL = {0}", secondStr);
            Assert.AreEqual<string>(firstStr, secondStr, "Managed Acl == FromNative Acl");
        }

        private unsafe void TestToAndFromNative(ResourceIdentifier resourceIdentifier)
        {
            IntPtr nativePtr = IntPtr.Zero;
            ResourceIdentifier second = null;
            ResourceIdentifier first = resourceIdentifier;

            var firstStr = AccessControlUtility.ToString(first);
            LogHelper.Log("Managed ResourceIdentifier = {0}", firstStr);

            using (var pinCollection = new PinCollection())
            {
                nativePtr = resourceIdentifier.ToNative(pinCollection);
                second = ResourceIdentifier.FromNative((NativeTypes.FABRIC_SECURITY_RESOURCE_IDENTIFIER*)nativePtr);
            }

            var secondStr = AccessControlUtility.ToString(second);
            LogHelper.Log("FromNative ResourceIdentifier = {0}", secondStr);
            Assert.AreEqual<string>(firstStr, secondStr, "Managed ResourceIdentifier == FromNative ResourceIdentifier");
        }
    }
}