// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Test
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric;
    using System.Fabric.Test;
    using System.Fabric.Data;
    using System.Fabric.Data.Common;
    using System.Fabric.Data.LockManager;
    using System.Fabric.Data.TransactionManager;
    using System.Fabric.Data.Btree;
    using System.Fabric.Data.StateProviders;
    using System.Linq;
    using System.Text;
    using System.Globalization;
    using VS = Microsoft.VisualStudio.TestTools.UnitTesting;

    [VS.TestClass]
    public class TransactionManagerTest
    {
        [VS.ClassInitialize]
        public static void ClassInitialize(VS.TestContext context)
        {
        }

        [SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", MessageId = "System.GC.Collect")]
        [VS.ClassCleanup]
        public static void ClassCleanup()
        {
            // To validate no tasks are left behind with unhandled exceptions
            GC.Collect();
            GC.WaitForPendingFinalizers();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void OpenClose()
        {
            Uri owner = new Uri("fabric://test/txmgr");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void CreateTerminateReadOnlyTransaction()
        {
            Uri owner = new Uri("fabric://test/txmgr");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            LogHelper.Log("Create read only transaction");
            IReadOnlyTransaction rotx = txMrg.CreateTransaction();

            LogHelper.Log("Terminate read only transaction");
            rotx.Terminate();

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void CreateTerminateReadOnlyTransaction2()
        {
            Uri owner = new Uri("fabric://test/txmgr");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            LogHelper.Log("Create read only transaction");
            IReadOnlyTransaction rotx = txMrg.CreateTransaction();

            LogHelper.Log("Terminate read only transaction");
            txMrg.RemoveTransaction(rotx);

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void CreateTerminateReadWriteTransaction()
        {
            Uri owner = new Uri("fabric://test/txmgr#bar");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            LogHelper.Log("Create read write transaction");
            bool existing = false;
            long atomicGroupId = 1;
            IReadWriteTransaction rwtx = txMrg.CreateTransaction(atomicGroupId, out existing);
            if (existing)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log("Terminate read write transaction");
            txMrg.RemoveTransaction(rwtx);

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void CreateTerminateReadWriteTransaction2()
        {
            Uri owner = new Uri("fabric://test/txmgr#bar");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            LogHelper.Log("Create read write transaction");
            bool existing = false;
            long atomicGroupId = 1;
            IReadWriteTransaction rwtx1 = txMrg.CreateTransaction(atomicGroupId, out existing);
            if (existing)
            {
                throw new InvalidOperationException();
            }
            IReadWriteTransaction rwtx2 = txMrg.CreateTransaction(atomicGroupId, out existing);
            if (!existing)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log("Terminate read write transaction 1");
            txMrg.RemoveTransaction(rwtx1);

            LogHelper.Log("Terminate read write transaction 2");
            txMrg.RemoveTransaction(rwtx2);

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void GetRemoveReadWriteTransaction()
        {
            Uri owner = new Uri("fabric://test/txmgr#bar");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            LogHelper.Log("Create read write transaction");
            bool existing = false;
            long atomicGroupId = 1;
            IReadWriteTransaction rwtx1 = txMrg.CreateTransaction(atomicGroupId, out existing);
            if (existing)
            {
                throw new InvalidOperationException();
            }
            IReadWriteTransaction rwtx2 = txMrg.GetTransaction(atomicGroupId);
            if (rwtx1 != rwtx2)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log("Terminate read write transaction 1");
            txMrg.RemoveTransaction(rwtx1.AtomicGroupId);

            LogHelper.Log("Terminate read write transaction 2");
            txMrg.RemoveTransaction(rwtx2);

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void AcquireReleaseLock()
        {
            Uri owner = new Uri("fabric://test/txmgr#bar");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            LogHelper.Log("Create read write transaction");
            bool existing = false;
            long atomicGroupId = 1;
            IReadWriteTransaction rwtx = txMrg.CreateTransaction(atomicGroupId, out existing);
            if (existing)
            {
                throw new InvalidOperationException();
            }

            string lockResourceName = "A";
            LockMode modeExclusive = LockMode.Exclusive;
            ILock lock_A_X = rwtx.LockAsync(lockResourceName, modeExclusive, 100).Result;
            if (null == lock_A_X)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log(
                "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}", 
                lock_A_X.Owner, 
                lock_A_X.ResourceName, 
                lock_A_X.Mode,
                lock_A_X.Timeout,
                lock_A_X.Status, 
                lock_A_X.GrantTime, 
                lock_A_X.Count);

            if (lock_A_X.Owner != rwtx.Id || 
                lock_A_X.ResourceName != lockResourceName || 
                LockStatus.Granted != lock_A_X.Status || 
                1 != lock_A_X.Count || 
                modeExclusive != lock_A_X.Mode)
            {
                throw new InvalidOperationException();
            }

            rwtx.Unlock(lock_A_X);

            LogHelper.Log(
                "Released lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                lock_A_X.Owner,
                lock_A_X.ResourceName,
                lock_A_X.Mode,
                lock_A_X.Timeout,
                lock_A_X.Status,
                lock_A_X.GrantTime,
                lock_A_X.Count);

            if (lock_A_X.Owner != rwtx.Id || 
                lock_A_X.ResourceName != lockResourceName ||
                LockStatus.Granted != lock_A_X.Status || 
                0 != lock_A_X.Count ||
                modeExclusive != lock_A_X.Mode)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log("Terminate read write transaction 1");
            txMrg.RemoveTransaction(rwtx);

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void AcquireReleaseLock2()
        {
            Uri owner = new Uri("fabric://test/txmgr#bar");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            LogHelper.Log("Create read write transaction");
            bool existing = false;
            long atomicGroupId = 1;
            IReadWriteTransaction rwtx = txMrg.CreateTransaction(atomicGroupId, out existing);
            if (existing)
            {
                throw new InvalidOperationException();
            }

            string lockResourceName = "A";
            LockMode modeExclusive = LockMode.Exclusive;
            ILock lock_A_X = rwtx.LockAsync(lockResourceName, modeExclusive, 100).Result;
            if (null == lock_A_X)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log(
                "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                lock_A_X.Owner,
                lock_A_X.ResourceName,
                lock_A_X.Mode,
                lock_A_X.Timeout,
                lock_A_X.Status,
                lock_A_X.GrantTime,
                lock_A_X.Count);

            if (lock_A_X.Owner != rwtx.Id || 
                lock_A_X.ResourceName != lockResourceName || 
                LockStatus.Granted != lock_A_X.Status || 
                1 != lock_A_X.Count || 
                modeExclusive != lock_A_X.Mode)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log("Terminate read write transaction 1");
            txMrg.RemoveTransaction(rwtx);

            LogHelper.Log(
                "Released lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                lock_A_X.Owner,
                lock_A_X.ResourceName,
                lock_A_X.Mode,
                lock_A_X.Timeout,
                lock_A_X.Status,
                lock_A_X.GrantTime,
                lock_A_X.Count);

            if (lock_A_X.Owner != rwtx.Id ||
                lock_A_X.ResourceName != lockResourceName ||
                LockStatus.Granted != lock_A_X.Status ||
                0 != lock_A_X.Count ||
                modeExclusive != lock_A_X.Mode)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void AcquireReleaseTimeoutLock()
        {
            Uri owner = new Uri("fabric://test/txmgr");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            LogHelper.Log("Create read write transaction 1");
            bool existing1 = false;
            long atomicGroupId1 = 1;
            IReadWriteTransaction rwtx1 = txMrg.CreateTransaction(atomicGroupId1, out existing1);
            if (existing1)
            {
                throw new InvalidOperationException();
            }

            string lockResourceName = "A";
            LockMode modeExclusive = LockMode.Exclusive;

            ILock lock_A_X_1 = rwtx1.LockAsync(lockResourceName, modeExclusive, 100).Result;
            if (null == lock_A_X_1)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log(
                "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                lock_A_X_1.Owner,
                lock_A_X_1.ResourceName,
                lock_A_X_1.Mode,
                lock_A_X_1.Timeout,
                lock_A_X_1.Status,
                lock_A_X_1.GrantTime,
                lock_A_X_1.Count);

            if (lock_A_X_1.Owner != rwtx1.Id ||
                lock_A_X_1.ResourceName != lockResourceName ||
                LockStatus.Granted != lock_A_X_1.Status ||
                1 != lock_A_X_1.Count ||
                modeExclusive != lock_A_X_1.Mode)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log("Create read write transaction 2");
            bool existing2 = false;
            long atomicGroupId2 = 2;
            IReadWriteTransaction rwtx2 = txMrg.CreateTransaction(atomicGroupId2, out existing2);
            if (existing2)
            {
                throw new InvalidOperationException();
            }

            ILock lock_A_X_2 = null;
            try
            {
                lock_A_X_2 = rwtx2.LockAsync(lockResourceName, modeExclusive, 100).Result;
            }
            catch (Exception e)
            {
                AggregateException ae = e as AggregateException;
                if (null == ae || !(ae.InnerException is TimeoutException))
                {
                    throw new InvalidOperationException();
                }
                LogHelper.Log("Timeout exception occured for read write transaction 2");
            }

            LogHelper.Log("Terminate read write transaction 1");
            txMrg.RemoveTransaction(rwtx1);

            LogHelper.Log("Terminate read write transaction 2");
            txMrg.RemoveTransaction(rwtx2);

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void AcquireReleaseTimeoutReacquireLock()
        {
            Uri owner = new Uri("fabric://test/txmgr");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            LogHelper.Log("Create read write transaction 1");
            bool existing1 = false;
            long atomicGroupId1 = 1;
            IReadWriteTransaction rwtx1 = txMrg.CreateTransaction(atomicGroupId1, out existing1);
            if (existing1)
            {
                throw new InvalidOperationException();
            }

            string lockResourceName = "A";
            LockMode modeExclusive = LockMode.Exclusive;

            ILock lock_A_X_1 = rwtx1.LockAsync(lockResourceName, modeExclusive, 100).Result;
            if (null == lock_A_X_1)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log(
                "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                lock_A_X_1.Owner,
                lock_A_X_1.ResourceName,
                lock_A_X_1.Mode,
                lock_A_X_1.Timeout,
                lock_A_X_1.Status,
                lock_A_X_1.GrantTime,
                lock_A_X_1.Count);

            if (lock_A_X_1.Owner != rwtx1.Id ||
                lock_A_X_1.ResourceName != lockResourceName ||
                LockStatus.Granted != lock_A_X_1.Status ||
                1 != lock_A_X_1.Count ||
                modeExclusive != lock_A_X_1.Mode)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log("Create read write transaction 2");
            bool existing2 = false;
            long atomicGroupId2 = 2;
            IReadWriteTransaction rwtx2 = txMrg.CreateTransaction(atomicGroupId2, out existing2);
            if (existing2)
            {
                throw new InvalidOperationException();
            }

            ILock lock_A_X_2 = null;
            try
            {
                lock_A_X_2 = rwtx2.LockAsync(lockResourceName, modeExclusive, 100).Result;
            }
            catch (Exception e)
            {
                AggregateException ae = e as AggregateException;
                if (null == ae || !(ae.InnerException is TimeoutException))
                {
                    throw new InvalidOperationException();
                }
                LogHelper.Log("Timeout exception occured for read write transaction 2");
            }

            LogHelper.Log("Terminate read write transaction 1");
            txMrg.RemoveTransaction(rwtx1);

            lock_A_X_2 = rwtx2.LockAsync(lockResourceName, modeExclusive, 100).Result;
            if (null == lock_A_X_2)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log(
                "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                lock_A_X_2.Owner,
                lock_A_X_2.ResourceName,
                lock_A_X_2.Mode,
                lock_A_X_2.Timeout,
                lock_A_X_2.Status,
                lock_A_X_2.GrantTime,
                lock_A_X_2.Count);

            if (lock_A_X_2.Owner != rwtx2.Id ||
                lock_A_X_2.ResourceName != lockResourceName ||
                LockStatus.Granted != lock_A_X_2.Status ||
                1 != lock_A_X_2.Count ||
                modeExclusive != lock_A_X_2.Mode)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log("Terminate read write transaction 2");
            txMrg.RemoveTransaction(rwtx2);

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void SharedLock()
        {
            Uri owner = new Uri("fabric://test/txmgr");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            LogHelper.Log("Create read write transaction 1");
            bool existing1 = false;
            long atomicGroupId1 = 1;
            IReadWriteTransaction rwtx1 = txMrg.CreateTransaction(atomicGroupId1, out existing1);
            if (existing1)
            {
                throw new InvalidOperationException();
            }
            LogHelper.Log("Create read write transaction 2");
            bool existing2 = false;
            long atomicGroupId2 = 2;
            IReadWriteTransaction rwtx2 = txMrg.CreateTransaction(atomicGroupId2, out existing2);
            if (existing2)
            {
                throw new InvalidOperationException();
            }

            string lockResourceNameCommon = "Common";
            LockMode modeIntentExclusive = LockMode.IntentExclusive;

            ILock lock_Common_IX_1 = rwtx1.LockAsync(lockResourceNameCommon, modeIntentExclusive, 100).Result;
            if (null == lock_Common_IX_1)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log(
                "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                lock_Common_IX_1.Owner,
                lock_Common_IX_1.ResourceName,
                lock_Common_IX_1.Mode,
                lock_Common_IX_1.Timeout,
                lock_Common_IX_1.Status,
                lock_Common_IX_1.GrantTime,
                lock_Common_IX_1.Count);

            if (lock_Common_IX_1.Owner != rwtx1.Id ||
                lock_Common_IX_1.ResourceName != lockResourceNameCommon ||
                LockStatus.Granted != lock_Common_IX_1.Status ||
                1 != lock_Common_IX_1.Count ||
                modeIntentExclusive != lock_Common_IX_1.Mode)
            {
                throw new InvalidOperationException();
            }

            ILock lock_Common_IX_2 = rwtx2.LockAsync(lockResourceNameCommon, modeIntentExclusive, 100).Result;
            if (null == lock_Common_IX_2)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log(
                "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                lock_Common_IX_2.Owner,
                lock_Common_IX_2.ResourceName,
                lock_Common_IX_2.Mode,
                lock_Common_IX_2.Timeout,
                lock_Common_IX_2.Status,
                lock_Common_IX_2.GrantTime,
                lock_Common_IX_2.Count);

            if (lock_Common_IX_2.Owner != rwtx2.Id ||
                lock_Common_IX_2.ResourceName != lockResourceNameCommon ||
                LockStatus.Granted != lock_Common_IX_2.Status ||
                1 != lock_Common_IX_2.Count ||
                modeIntentExclusive != lock_Common_IX_2.Mode)
            {
                throw new InvalidOperationException();
            }
            
            LogHelper.Log("Terminate read write transaction 1");
            txMrg.RemoveTransaction(rwtx1);

            LogHelper.Log("Terminate read write transaction 2");
            txMrg.RemoveTransaction(rwtx2);

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void SharedLock2()
        {
            Uri owner = new Uri("fabric://test/txmgr");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            int count = 10;
            string lockResourceNameCommon = "Common";
            LockMode modeShared = LockMode.Shared;
            List<IReadOnlyTransaction> rotxList = new List<IReadOnlyTransaction>();
            for (int x = 0; x < count; x++)
            {
                LogHelper.Log("Create read only transaction");
                IReadOnlyTransaction rotx = txMrg.CreateTransaction();
                rotxList.Add(rotx);

                ILock lock_Common_S = rotx.LockAsync(lockResourceNameCommon, modeShared, 100).Result;
                if (null == lock_Common_S)
                {
                    throw new InvalidOperationException();
                }

                LogHelper.Log(
                    "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                    lock_Common_S.Owner,
                    lock_Common_S.ResourceName,
                    lock_Common_S.Mode,
                    lock_Common_S.Timeout,
                    lock_Common_S.Status,
                    lock_Common_S.GrantTime,
                    lock_Common_S.Count);

                if (lock_Common_S.Owner != rotx.Id ||
                    lock_Common_S.ResourceName != lockResourceNameCommon ||
                    LockStatus.Granted != lock_Common_S.Status ||
                    1 != lock_Common_S.Count ||
                    modeShared != lock_Common_S.Mode)
                {
                    throw new InvalidOperationException();
                }
            }
            for (int x = 0; x < count; x++)
            {
                LogHelper.Log("Terminate read only transaction");
                rotxList[x].Terminate();
            }

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void SharedLock3()
        {
            Uri owner = new Uri("fabric://test/txmgr#bar");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            long count = 10;
            string lockResourceNameCommon = "Common";
            LockMode modeShared = LockMode.Shared;
            List<IReadWriteTransaction> rwtxList = new List<IReadWriteTransaction>();
            for (long x = 0; x < count; x++)
            {
                LogHelper.Log("Create read write transaction");
                bool existing = false;
                IReadWriteTransaction rwtx = txMrg.CreateTransaction(x, out existing);
                if (existing)
                {
                    throw new InvalidOperationException();
                }
                rwtxList.Add(rwtx);

                ILock lock_Common_S = rwtx.LockAsync(lockResourceNameCommon, modeShared, 100).Result;
                if (null == lock_Common_S)
                {
                    throw new InvalidOperationException();
                }

                LogHelper.Log(
                    "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                    lock_Common_S.Owner,
                    lock_Common_S.ResourceName,
                    lock_Common_S.Mode,
                    lock_Common_S.Timeout,
                    lock_Common_S.Status,
                    lock_Common_S.GrantTime,
                    lock_Common_S.Count);

                if (lock_Common_S.Owner != rwtx.Id ||
                    lock_Common_S.ResourceName != lockResourceNameCommon ||
                    LockStatus.Granted != lock_Common_S.Status ||
                    1 != lock_Common_S.Count ||
                    modeShared != lock_Common_S.Mode)
                {
                    throw new InvalidOperationException();
                }
            }
            for (long x = 0; x < count; x++)
            {
                LogHelper.Log("Terminate read write transaction");
                txMrg.RemoveTransaction(rwtxList[(int)x]);
            }

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void SharedDisjointLock()
        {
            Uri owner = new Uri("fabric://test/txmgr");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            LogHelper.Log("Create read write transaction 1");
            bool existing1 = false;
            long atomicGroupId1 = 1;
            IReadWriteTransaction rwtx1 = txMrg.CreateTransaction(atomicGroupId1, out existing1);
            if (existing1)
            {
                throw new InvalidOperationException();
            }
            LogHelper.Log("Create read write transaction 2");
            bool existing2 = false;
            long atomicGroupId2 = 2;
            IReadWriteTransaction rwtx2 = txMrg.CreateTransaction(atomicGroupId2, out existing2);
            if (existing2)
            {
                throw new InvalidOperationException();
            }

            string lockResourceNameCommon = "Common";
            LockMode modeIntentExclusive = LockMode.IntentExclusive;
            string lockResourceNameA = "A";
            string lockResourceNameB = "B";
            LockMode modeExclusive = LockMode.Exclusive;

            ILock lock_Common_IX_1 = rwtx1.LockAsync(lockResourceNameCommon, modeIntentExclusive, 100).Result;
            if (null == lock_Common_IX_1)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log(
                "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                lock_Common_IX_1.Owner,
                lock_Common_IX_1.ResourceName,
                lock_Common_IX_1.Mode,
                lock_Common_IX_1.Timeout,
                lock_Common_IX_1.Status,
                lock_Common_IX_1.GrantTime,
                lock_Common_IX_1.Count);

            if (lock_Common_IX_1.Owner != rwtx1.Id ||
                lock_Common_IX_1.ResourceName != lockResourceNameCommon ||
                LockStatus.Granted != lock_Common_IX_1.Status ||
                1 != lock_Common_IX_1.Count ||
                modeIntentExclusive != lock_Common_IX_1.Mode)
            {
                throw new InvalidOperationException();
            }

            ILock lock_Common_IX_2 = rwtx2.LockAsync(lockResourceNameCommon, modeIntentExclusive, 100).Result;
            if (null == lock_Common_IX_2)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log(
                "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                lock_Common_IX_2.Owner,
                lock_Common_IX_2.ResourceName,
                lock_Common_IX_2.Mode,
                lock_Common_IX_2.Timeout,
                lock_Common_IX_2.Status,
                lock_Common_IX_2.GrantTime,
                lock_Common_IX_2.Count);

            if (lock_Common_IX_2.Owner != rwtx2.Id ||
                lock_Common_IX_2.ResourceName != lockResourceNameCommon ||
                LockStatus.Granted != lock_Common_IX_2.Status ||
                1 != lock_Common_IX_2.Count ||
                modeIntentExclusive != lock_Common_IX_2.Mode)
            {
                throw new InvalidOperationException();
            }

            ILock lock_A_X = rwtx1.LockAsync(lockResourceNameA, modeExclusive, 100).Result;
            if (null == lock_A_X)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log(
                "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                lock_A_X.Owner,
                lock_A_X.ResourceName,
                lock_A_X.Mode,
                lock_A_X.Timeout,
                lock_A_X.Status,
                lock_A_X.GrantTime,
                lock_A_X.Count);

            if (lock_A_X.Owner != rwtx1.Id ||
                lock_A_X.ResourceName != lockResourceNameA ||
                LockStatus.Granted != lock_A_X.Status ||
                1 != lock_A_X.Count ||
                modeExclusive != lock_A_X.Mode)
            {
                throw new InvalidOperationException();
            }

            ILock lock_B_X = rwtx2.LockAsync(lockResourceNameB, modeExclusive, 100).Result;
            if (null == lock_B_X)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log(
                "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                lock_B_X.Owner,
                lock_B_X.ResourceName,
                lock_B_X.Mode,
                lock_B_X.Timeout,
                lock_B_X.Status,
                lock_B_X.GrantTime,
                lock_B_X.Count);

            if (lock_B_X.Owner != rwtx2.Id ||
                lock_B_X.ResourceName != lockResourceNameB ||
                LockStatus.Granted != lock_B_X.Status ||
                1 != lock_B_X.Count ||
                modeExclusive != lock_B_X.Mode)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log("Terminate read write transaction 1");
            txMrg.RemoveTransaction(rwtx1);

            LogHelper.Log("Terminate read write transaction 2");
            txMrg.RemoveTransaction(rwtx2);

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void SharedLockMultiple()
        {
            Uri owner = new Uri("fabric://test/txmgr#bar");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            long count = 10;
            string lockResourceNameCommon = "Common";
            LockMode modeShared = LockMode.Shared;
            List<IReadWriteTransaction> rwtxList = new List<IReadWriteTransaction>();
            for (long x = 0; x < count; x++)
            {
                LogHelper.Log("Create read write transaction");
                bool existing = false;
                IReadWriteTransaction rwtx = txMrg.CreateTransaction(x, out existing);
                if (existing)
                {
                    throw new InvalidOperationException();
                }
                rwtxList.Add(rwtx);

                for (long y = 0; y < count; y++)
                {
                    ILock lock_Common_S = rwtx.LockAsync(lockResourceNameCommon, modeShared, 100).Result;
                    if (null == lock_Common_S)
                    {
                        throw new InvalidOperationException();
                    }

                    LogHelper.Log(
                        "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                        lock_Common_S.Owner,
                        lock_Common_S.ResourceName,
                        lock_Common_S.Mode,
                        lock_Common_S.Timeout,
                        lock_Common_S.Status,
                        lock_Common_S.GrantTime,
                        lock_Common_S.Count);

                    if (lock_Common_S.Owner != rwtx.Id ||
                        lock_Common_S.ResourceName != lockResourceNameCommon ||
                        LockStatus.Granted != lock_Common_S.Status ||
                        y + 1 != lock_Common_S.Count ||
                        modeShared != lock_Common_S.Mode)
                    {
                        throw new InvalidOperationException();
                    }
                }
            }
            for (long x = 0; x < count; x++)
            {
                LogHelper.Log("Terminate read write transaction");
                txMrg.RemoveTransaction(rwtxList[(int)x]);
            }

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void Concurrent()
        {
            Uri owner = new Uri("fabric://test/txmgr#bar");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            int timeout = 1000 * 1000;
            long count = 100;
            string lockResourceNameCommon = "Common";
            LockMode modeShared = LockMode.Shared;
            LockMode modeExclusive = LockMode.Exclusive;
            List<IReadWriteTransaction> rwtxList = new List<IReadWriteTransaction>();
            List<Task> workList = new List<Task>();
            for (long x = 0; x < count; x++)
            {
                LogHelper.Log("Create read write transaction");
                bool existing = false;
                IReadWriteTransaction rwtx = txMrg.CreateTransaction(x, out existing);
                if (existing)
                {
                    throw new InvalidOperationException();
                }
                rwtxList.Add(rwtx);

                Task work = null;
                if (0 == x % 2)
                {
                    work = Task.Factory.StartNew(() =>
                    {
                        ILock lock_Common_S = rwtx.LockAsync(lockResourceNameCommon, modeShared, timeout).Result;
                        if (null == lock_Common_S)
                        {
                            throw new InvalidOperationException();
                        }

                        LogHelper.Log(
                            "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                            lock_Common_S.Owner,
                            lock_Common_S.ResourceName,
                            lock_Common_S.Mode,
                            lock_Common_S.Timeout,
                            lock_Common_S.Status,
                            lock_Common_S.GrantTime,
                            lock_Common_S.Count);

                        if (lock_Common_S.Owner != rwtx.Id ||
                            lock_Common_S.ResourceName != lockResourceNameCommon ||
                            LockStatus.Granted != lock_Common_S.Status ||
                            1 != lock_Common_S.Count ||
                            modeShared != lock_Common_S.Mode)
                        {
                            throw new InvalidOperationException();
                        }
                        txMrg.RemoveTransaction(rwtx);
                    });
                }
                else
                {
                    work = Task.Factory.StartNew(() =>
                    {
                        ILock lock_Common_X = rwtx.LockAsync(lockResourceNameCommon, modeExclusive, timeout).Result;
                        if (null == lock_Common_X)
                        {
                            throw new InvalidOperationException();
                        }

                        LogHelper.Log(
                            "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                            lock_Common_X.Owner,
                            lock_Common_X.ResourceName,
                            lock_Common_X.Mode,
                            lock_Common_X.Timeout,
                            lock_Common_X.Status,
                            lock_Common_X.GrantTime,
                            lock_Common_X.Count);

                        if (lock_Common_X.Owner != rwtx.Id ||
                            lock_Common_X.ResourceName != lockResourceNameCommon ||
                            LockStatus.Granted != lock_Common_X.Status ||
                            1 != lock_Common_X.Count ||
                            modeExclusive != lock_Common_X.Mode)
                        {
                            throw new InvalidOperationException();
                        }
                        txMrg.RemoveTransaction(rwtx);
                    });
                }
                workList.Add(work);
            }

            LogHelper.Log("Waiting for tasks to complete");
            Task.WaitAll(workList.ToArray());

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void InfiniteTimeout()
        {
            Uri owner = new Uri("fabric://test/txmgr#bar");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            LogHelper.Log("Create read write transaction");
            bool existing = false;
            long atomicGroupId = 1;
            IReadWriteTransaction rwtx = txMrg.CreateTransaction(atomicGroupId, out existing);
            if (existing)
            {
                throw new InvalidOperationException();
            }

            string lockResourceName = "A";
            LockMode modeExclusive = LockMode.Exclusive;
            ILock lock_A_X = rwtx.LockAsync(lockResourceName, modeExclusive, Timeout.Infinite).Result;
            if (null == lock_A_X)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log(
                "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                lock_A_X.Owner,
                lock_A_X.ResourceName,
                lock_A_X.Mode,
                lock_A_X.Timeout,
                lock_A_X.Status,
                lock_A_X.GrantTime,
                lock_A_X.Count);

            if (lock_A_X.Owner != rwtx.Id ||
                lock_A_X.ResourceName != lockResourceName ||
                LockStatus.Granted != lock_A_X.Status ||
                1 != lock_A_X.Count ||
                modeExclusive != lock_A_X.Mode)
            {
                throw new InvalidOperationException();
            }

            rwtx.Unlock(lock_A_X);

            LogHelper.Log(
                "Released lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                lock_A_X.Owner,
                lock_A_X.ResourceName,
                lock_A_X.Mode,
                lock_A_X.Timeout,
                lock_A_X.Status,
                lock_A_X.GrantTime,
                lock_A_X.Count);

            if (lock_A_X.Owner != rwtx.Id ||
                lock_A_X.ResourceName != lockResourceName ||
                LockStatus.Granted != lock_A_X.Status ||
                0 != lock_A_X.Count ||
                modeExclusive != lock_A_X.Mode)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log("Terminate read write transaction 1");
            txMrg.RemoveTransaction(rwtx);

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void Concurrent2()
        {
            Uri owner = new Uri("fabric://test/txmgr#bar");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            int timeout = Timeout.Infinite;
            long count = 100;
            string lockResourceNameCommon = "Common";
            LockMode modeShared = LockMode.Shared;
            LockMode modeExclusive = LockMode.Exclusive;
            List<IReadWriteTransaction> rwtxList = new List<IReadWriteTransaction>();
            List<Task> workList = new List<Task>();
            for (long x = 0; x < count; x++)
            {
                LogHelper.Log("Create read write transaction");
                bool existing = false;
                IReadWriteTransaction rwtx = txMrg.CreateTransaction(x, out existing);
                if (existing)
                {
                    throw new InvalidOperationException();
                }
                rwtxList.Add(rwtx);

                Task work = null;
                if (0 == x % 2)
                {
                    work = Task.Factory.StartNew(() =>
                    {
                        ILock lock_Common_S = rwtx.LockAsync(lockResourceNameCommon, modeShared, timeout).Result;
                        if (null == lock_Common_S)
                        {
                            throw new InvalidOperationException();
                        }

                        LogHelper.Log(
                            "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                            lock_Common_S.Owner,
                            lock_Common_S.ResourceName,
                            lock_Common_S.Mode,
                            lock_Common_S.Timeout,
                            lock_Common_S.Status,
                            lock_Common_S.GrantTime,
                            lock_Common_S.Count);

                        if (lock_Common_S.Owner != rwtx.Id ||
                            lock_Common_S.ResourceName != lockResourceNameCommon ||
                            LockStatus.Granted != lock_Common_S.Status ||
                            1 != lock_Common_S.Count ||
                            modeShared != lock_Common_S.Mode)
                        {
                            throw new InvalidOperationException();
                        }
                        txMrg.RemoveTransaction(rwtx);
                    });
                }
                else
                {
                    work = Task.Factory.StartNew(() =>
                    {
                        ILock lock_Common_X = rwtx.LockAsync(lockResourceNameCommon, modeExclusive, timeout).Result;
                        if (null == lock_Common_X)
                        {
                            throw new InvalidOperationException();
                        }

                        LogHelper.Log(
                            "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                            lock_Common_X.Owner,
                            lock_Common_X.ResourceName,
                            lock_Common_X.Mode,
                            lock_Common_X.Timeout,
                            lock_Common_X.Status,
                            lock_Common_X.GrantTime,
                            lock_Common_X.Count);

                        if (lock_Common_X.Owner != rwtx.Id ||
                            lock_Common_X.ResourceName != lockResourceNameCommon ||
                            LockStatus.Granted != lock_Common_X.Status ||
                            1 != lock_Common_X.Count ||
                            modeExclusive != lock_Common_X.Mode)
                        {
                            throw new InvalidOperationException();
                        }
                        txMrg.RemoveTransaction(rwtx);
                    });
                }
                workList.Add(work);
            }

            LogHelper.Log("Waiting for tasks to complete");
            Task.WaitAll(workList.ToArray());

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void ConcurrentClose()
        {
            Uri owner = new Uri("fabric://test/txmgr#bar");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            LogHelper.Log("Create read write transaction");
            bool existing = false;
            long atomicGroupId = 1;
            IReadWriteTransaction rwtx = txMrg.CreateTransaction(atomicGroupId, out existing);
            if (existing)
            {
                throw new InvalidOperationException();
            }

            string lockResourceNameCommon = "Common";
            LockMode modeExclusive = LockMode.Exclusive;
            Task work = Task.Factory.StartNew(() =>
            {
                ILock lock_Common_X = rwtx.LockAsync(lockResourceNameCommon, modeExclusive, Timeout.Infinite).Result;
                if (null == lock_Common_X)
                {
                    throw new InvalidOperationException();
                }

                LogHelper.Log(
                    "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                    lock_Common_X.Owner,
                    lock_Common_X.ResourceName,
                    lock_Common_X.Mode,
                    lock_Common_X.Timeout,
                    lock_Common_X.Status,
                    lock_Common_X.GrantTime,
                    lock_Common_X.Count);

                if (lock_Common_X.Owner != rwtx.Id ||
                    lock_Common_X.ResourceName != lockResourceNameCommon ||
                    LockStatus.Granted != lock_Common_X.Status ||
                    1 != lock_Common_X.Count ||
                    modeExclusive != lock_Common_X.Mode)
                {
                    throw new InvalidOperationException();
                }

                Thread.Sleep(3000);
                
                txMrg.RemoveTransaction(rwtx);
            });

            Thread.Sleep(3000);

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();

            LogHelper.Log("Waiting for work task");
            work.Wait();
            LogHelper.Log("Work task done");
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void AcquireReleaseLockRepeated()
        {
            Uri owner = new Uri("fabric://test/txmgr#bar");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            LogHelper.Log("Create read write transaction");
            bool existing = false;
            long atomicGroupId = 1;
            IReadWriteTransaction rwtx = txMrg.CreateTransaction(atomicGroupId, out existing);
            if (existing)
            {
                throw new InvalidOperationException();
            }

            string lockResourceName = "A";
            LockMode modeExclusive = LockMode.Exclusive;

            int count = 10;
            for (int x = 0; x < count; x++)
            {
                ILock lock_A_X = rwtx.LockAsync(lockResourceName, modeExclusive, 100).Result;
                if (null == lock_A_X)
                {
                    throw new InvalidOperationException();
                }

                LogHelper.Log(
                    "Acquired lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                    lock_A_X.Owner,
                    lock_A_X.ResourceName,
                    lock_A_X.Mode,
                    lock_A_X.Timeout,
                    lock_A_X.Status,
                    lock_A_X.GrantTime,
                    lock_A_X.Count);

                if (lock_A_X.Owner != rwtx.Id ||
                    lock_A_X.ResourceName != lockResourceName ||
                    LockStatus.Granted != lock_A_X.Status ||
                    1 != lock_A_X.Count ||
                    modeExclusive != lock_A_X.Mode)
                {
                    throw new InvalidOperationException();
                }

                rwtx.Unlock(lock_A_X);

                LogHelper.Log(
                    "Released lock: {0}, {1}, {2}, {3}, {4}, {5}, {6}",
                    lock_A_X.Owner,
                    lock_A_X.ResourceName,
                    lock_A_X.Mode,
                    lock_A_X.Timeout,
                    lock_A_X.Status,
                    lock_A_X.GrantTime,
                    lock_A_X.Count);

                if (lock_A_X.Owner != rwtx.Id ||
                    lock_A_X.ResourceName != lockResourceName ||
                    LockStatus.Granted != lock_A_X.Status ||
                    0 != lock_A_X.Count ||
                    modeExclusive != lock_A_X.Mode)
                {
                    throw new InvalidOperationException();
                }
            }

            LogHelper.Log("Terminate read write transaction 1");
            txMrg.RemoveTransaction(rwtx);

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }

        public class UnicodeStringValueBitConverter : IValueBitConverter<string>
        {
            public byte[] ToByteArray(string key)
            {
                return Encoding.Unicode.GetBytes(key);
            }
            public string FromByteArray(byte[] bytes)
            {
                return Encoding.Unicode.GetString(bytes);
            }
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void CommonClassesAndStructs()
        {
            BtreeKey<string, UnicodeStringKeyBitConverter> key = new BtreeKey<string, UnicodeStringKeyBitConverter>("testKey");
            key.Pin();
            key.Unpin();

            BtreeValue<string, UnicodeStringValueBitConverter> value = new BtreeValue<string, UnicodeStringValueBitConverter>("testValue");
            value.Pin();
            value.Unpin();

            BtreeConfigurationDescription config = new BtreeConfigurationDescription();
            config.PartitionId = Guid.NewGuid();
            config.ReplicaId = 1;
            config.StorageConfiguration = new BtreeStorageConfigurationDescription();
            config.KeyComparison = new KeyComparisonDescription();

            config.StorageConfiguration.Path = "C:\\temp\\foo.txt";
            config.StorageConfiguration.MaximumStorageInMB = 16;
            config.StorageConfiguration.IsVolatile = true;
            config.StorageConfiguration.MaximumMemoryInMB = 32;
            config.StorageConfiguration.MaximumPageSizeInKB = 4;
            config.StorageConfiguration.StoreDataInline = true;
            config.StorageConfiguration.RetriesBeforeTimeout = 8;

            config.KeyComparison.KeyDataType = (int)DataType.String;
            config.KeyComparison.CultureInfo = new CultureInfo("en-US");
            config.KeyComparison.MaximumKeySizeInBytes = 256;
            config.KeyComparison.IsFixedLengthKey = false;

            IntPtr configPin = config.Pin();
            config.Unpin();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void UpgradeLock()
        {
            Uri owner = new Uri("fabric://test/txmgr#bar");
            TransactionManager txMrg = new TransactionManager(owner);

            LogHelper.Log("Start transaction manager");
            txMrg.OpenAsync(CancellationToken.None).Wait();

            bool existing = false;

            LogHelper.Log("Create read write transaction 1");
            long atomicGroupId1 = 1;
            IReadWriteTransaction rwtx1 = txMrg.CreateTransaction(atomicGroupId1, out existing);
            if (existing)
            {
                throw new InvalidOperationException();
            }

            LogHelper.Log("Create read write transaction 2");
            long atomicGroupId2 = 2;
            IReadWriteTransaction rwtx2 = txMrg.CreateTransaction(atomicGroupId2, out existing);
            if (existing)
            {
                throw new InvalidOperationException();
            }

            string lockResourceName = "UpgradedResource";
            LockMode modeShared = LockMode.Shared;
            LockMode modeExclusive = LockMode.Exclusive;

            ILock lock_A_X_1 = rwtx1.LockAsync(lockResourceName, modeShared, 100).Result;
            if (null == lock_A_X_1)
            {
                throw new InvalidOperationException();
            }

            ILock lock_A_X_2 = rwtx2.LockAsync(lockResourceName, modeShared, 100).Result;
            if (null == lock_A_X_2)
            {
                throw new InvalidOperationException();
            }

            Task work = Task.Factory.StartNew(() =>
            {
                ILock lock_A_X_1_Upgraded = rwtx1.LockAsync(lockResourceName, modeExclusive, Timeout.Infinite).Result;
                if (null == lock_A_X_1_Upgraded)
                {
                    throw new InvalidOperationException();
                }
                if (!lock_A_X_1_Upgraded.IsUpgraded)
                {
                    throw new InvalidOperationException();
                }
                rwtx1.Unlock(lock_A_X_1_Upgraded);
            });

            Thread.Sleep(5000);
            rwtx2.Unlock(lock_A_X_2);

            work.Wait();
            rwtx1.Unlock(lock_A_X_1);

            LogHelper.Log("Terminate read write transaction 1");
            txMrg.RemoveTransaction(rwtx1);

            LogHelper.Log("Terminate read write transaction 2");
            txMrg.RemoveTransaction(rwtx2);

            LogHelper.Log("Stop transaction manager");
            txMrg.CloseAsync(CancellationToken.None).Wait();
        }
    }
}