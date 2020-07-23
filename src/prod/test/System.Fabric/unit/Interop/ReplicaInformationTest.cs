// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Interop
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Linq;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class ReplicaInformationTest
    {
        NativeTypes.FABRIC_REPLICA_INFORMATION CreateNativeReplicaInformation(
            PinCollection pin,
            NativeTypes.FABRIC_REPLICA_ROLE role = NativeTypes.FABRIC_REPLICA_ROLE.FABRIC_REPLICA_ROLE_NONE,
            NativeTypes.FABRIC_REPLICA_STATUS status = NativeTypes.FABRIC_REPLICA_STATUS.FABRIC_REPLICA_STATUS_UP,
            long catchupCapability = 1,
            long currentProgress = 2,
            long id = 3,
            string replicatorAddress = "",
            bool mustCatchup = false
            )
        {
            var ex = new NativeTypes.FABRIC_REPLICA_INFORMATION_EX1[1];
            ex[0].MustCatchup = NativeTypes.ToBOOLEAN(mustCatchup);
            ex[0].Reserved = IntPtr.Zero;

            return new NativeTypes.FABRIC_REPLICA_INFORMATION
            {
                CatchUpCapability = catchupCapability,
                CurrentProgress = currentProgress,
                Id = id,
                ReplicatorAddress = pin.AddBlittable(replicatorAddress),
                Role = (NativeTypes.FABRIC_REPLICA_ROLE)role,
                Status = (NativeTypes.FABRIC_REPLICA_STATUS)status,
                Reserved = pin.AddBlittable(ex)
            };
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("sumukhs")]
        public void ConvertFromAndToNative()
        {
            using (PinCollection pin = new PinCollection())
            {
                var native = CreateNativeReplicaInformation(pin, replicatorAddress: "address");
                native = Validate(pin, native);
            }

            using (PinCollection pin = new PinCollection())
            {
                var native = CreateNativeReplicaInformation(pin, role: NativeTypes.FABRIC_REPLICA_ROLE.FABRIC_REPLICA_ROLE_NONE);
                native = Validate(pin, native);
            }

            using (PinCollection pin = new PinCollection())
            {
                var native = CreateNativeReplicaInformation(pin, role: NativeTypes.FABRIC_REPLICA_ROLE.FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
                native = Validate(pin, native);
            }

            using (PinCollection pin = new PinCollection())
            {
                var native = CreateNativeReplicaInformation(pin, role: NativeTypes.FABRIC_REPLICA_ROLE.FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
                native = Validate(pin, native);
            }

            using (PinCollection pin = new PinCollection())
            {
                var native = CreateNativeReplicaInformation(pin, role: NativeTypes.FABRIC_REPLICA_ROLE.FABRIC_REPLICA_ROLE_UNKNOWN);
                native = Validate(pin, native);
            }

            using (PinCollection pin = new PinCollection())
            {
                var native = CreateNativeReplicaInformation(pin, role: NativeTypes.FABRIC_REPLICA_ROLE.FABRIC_REPLICA_ROLE_PRIMARY);
                native = Validate(pin, native);
            }

            using (PinCollection pin = new PinCollection())
            {
                var native = CreateNativeReplicaInformation(pin, status: NativeTypes.FABRIC_REPLICA_STATUS.FABRIC_REPLICA_STATUS_DOWN);
                native = Validate(pin, native);
            }

            using (PinCollection pin = new PinCollection())
            {
                var native = CreateNativeReplicaInformation(pin, status: NativeTypes.FABRIC_REPLICA_STATUS.FABRIC_REPLICA_STATUS_UP);
                native = Validate(pin, native);
            }
        }

        private static NativeTypes.FABRIC_REPLICA_INFORMATION Validate(PinCollection pin, NativeTypes.FABRIC_REPLICA_INFORMATION native)
        {
            var managed = ReplicaInformation.FromNative(pin.AddBlittable(native));

            AssertAreEqual(native, managed);

            NativeTypes.FABRIC_REPLICA_INFORMATION converted;
            managed.ToNative(pin, out converted);
            AssertAreEqual(native, converted);
            return native;
        }

        private static void AssertAreEqual(NativeTypes.FABRIC_REPLICA_INFORMATION first, NativeTypes.FABRIC_REPLICA_INFORMATION second)
        {
            Assert.AreEqual(first.CatchUpCapability, second.CatchUpCapability);
            Assert.AreEqual(first.CurrentProgress, second.CurrentProgress);
            Assert.AreEqual(first.Id, second.Id);
            Assert.AreEqual(NativeTypes.FromNativeString(first.ReplicatorAddress), NativeTypes.FromNativeString(second.ReplicatorAddress));
            Assert.AreEqual(first.Role, second.Role);
            Assert.AreEqual(first.Status, second.Status);

            unsafe
            {
                var firstEx1 = GetEx1(first);
                var secondEx1 = GetEx1(second);

                Assert.AreEqual(firstEx1->Reserved, secondEx1->Reserved);
                Assert.AreEqual(firstEx1->MustCatchup, secondEx1->MustCatchup);
            }
        }

        private static void AssertAreEqual(NativeTypes.FABRIC_REPLICA_INFORMATION native, ReplicaInformation managed)
        {
            Assert.AreEqual(native.CatchUpCapability, managed.CatchUpCapability);
            Assert.AreEqual(native.CurrentProgress, managed.CurrentProgress);
            Assert.AreEqual(native.Id, managed.Id);
            Assert.AreEqual(NativeTypes.FromNativeString(native.ReplicatorAddress), managed.ReplicatorAddress);
            Assert.AreEqual((int)native.Role, (int)managed.Role);
            Assert.AreEqual((int)native.Status, (int)managed.Status);

            unsafe
            {
                var casted = GetEx1(native);
                Assert.AreEqual(NativeTypes.FromBOOLEAN(casted->MustCatchup), managed.MustCatchup);
                Assert.AreEqual(IntPtr.Zero, casted->Reserved);
            }
        }

        private static unsafe NativeTypes.FABRIC_REPLICA_INFORMATION_EX1* GetEx1(NativeTypes.FABRIC_REPLICA_INFORMATION native)
        {
            Assert.AreNotEqual(IntPtr.Zero, native.Reserved);
            return (NativeTypes.FABRIC_REPLICA_INFORMATION_EX1*) (native.Reserved);
        }
    }
}