// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Test
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric;
    using System.Fabric.Interop;
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
    public class BtreeTest
    {
        [VS.ClassInitialize]
        public static void ClassInitialize(VS.TestContext context)
        {
        }

        [SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", MessageId = "System.GC.Collect")]
        [VS.ClassCleanup]
        public static void ClassCleanup()
        {
            GC.Collect();
            GC.WaitForPendingFinalizers();
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void OpenClose()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                Uri owner = new Uri("fabric://test/btree");
                LogHelper.Log("Begin construct btree");
                Btree<int, int, Int32KeyBitConverter, Int32ValueBitConverter> tree = new Btree<int, int, Int32KeyBitConverter, Int32ValueBitConverter>();
                LogHelper.Log("End construct btree");

                //
                // Key configuration
                //
                LogHelper.Log("Start create key description");
                KeyComparisonDescription keyDescription = new KeyComparisonDescription();
                keyDescription.CultureInfo = Globalization.CultureInfo.InvariantCulture;
                keyDescription.IsFixedLengthKey = true;
                keyDescription.KeyDataType = (int)DataType.Int32;
                keyDescription.MaximumKeySizeInBytes = sizeof(int);
                LogHelper.Log("End create key description");

                //
                // Storage configuration
                //
                LogHelper.Log("Start create storage description");
                BtreeStorageConfigurationDescription storageDescription = new BtreeStorageConfigurationDescription();
                storageDescription.IsVolatile = true;
                storageDescription.MaximumMemoryInMB = 1024;
                storageDescription.MaximumPageSizeInKB = 4;
                storageDescription.MaximumStorageInMB = 0;
                storageDescription.RetriesBeforeTimeout = 100;
                storageDescription.StoreDataInline = true;
                LogHelper.Log("End create key description");

                //
                // Btree configuration
                //
                LogHelper.Log("Start create btree description");
                BtreeConfigurationDescription btreeConfig = new BtreeConfigurationDescription();
                btreeConfig.KeyComparison = keyDescription;
                btreeConfig.StorageConfiguration = storageDescription;
                btreeConfig.PartitionId = Guid.NewGuid();
                btreeConfig.ReplicaId = 130083631515748206;
                LogHelper.Log("End create btree description");

                //
                // Open the btree
                //
                LogHelper.Log("Begin open btree");
                tree.OpenAsync(btreeConfig, false, CancellationToken.None).Wait();
                LogHelper.Log("End open btree");

                //
                // Close the btree
                //
                LogHelper.Log("Begin close btree");
                tree.CloseAsync(true, CancellationToken.None).Wait();
                LogHelper.Log("End close btree");
            }, 
            "OpenClose");
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void OpenCloseInsert()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                Uri owner = new Uri("fabric://test/btree");
                LogHelper.Log("Begin construct btree");
                Btree<int, int, Int32KeyBitConverter, Int32ValueBitConverter> tree = new Btree<int, int, Int32KeyBitConverter, Int32ValueBitConverter>();
                LogHelper.Log("End construct btree");

                //
                // Key configuration
                //
                LogHelper.Log("Start create key description");
                KeyComparisonDescription keyDescription = new KeyComparisonDescription();
                keyDescription.CultureInfo = Globalization.CultureInfo.InvariantCulture;
                keyDescription.IsFixedLengthKey = true;
                keyDescription.KeyDataType = (int)DataType.Int32;
                keyDescription.MaximumKeySizeInBytes = sizeof(int);
                LogHelper.Log("End create key description");

                //
                // Storage configuration
                //
                LogHelper.Log("Start create storage description");
                BtreeStorageConfigurationDescription storageDescription = new BtreeStorageConfigurationDescription();
                storageDescription.IsVolatile = true;
                storageDescription.MaximumMemoryInMB = 1024;
                storageDescription.MaximumPageSizeInKB = 4;
                storageDescription.MaximumStorageInMB = 0;
                storageDescription.RetriesBeforeTimeout = 100;
                storageDescription.StoreDataInline = true;
                LogHelper.Log("End create key description");

                //
                // Btree configuration
                //
                LogHelper.Log("Start create btree description");
                BtreeConfigurationDescription btreeConfig = new BtreeConfigurationDescription();
                btreeConfig.KeyComparison = keyDescription;
                btreeConfig.StorageConfiguration = storageDescription;
                btreeConfig.PartitionId = Guid.NewGuid();
                btreeConfig.ReplicaId = 130083631515748206;
                LogHelper.Log("End create btree description");

                //
                // Open the btree
                //
                LogHelper.Log("Begin open btree");
                tree.OpenAsync(btreeConfig, false, CancellationToken.None).Wait();
                LogHelper.Log("End open btree");

                //
                // Insert into the btree
                //
                for (int sequenceNumber = 0; sequenceNumber < 100; sequenceNumber++)
                {
                    BtreeKey<int, Int32KeyBitConverter> key = new BtreeKey<int, Int32KeyBitConverter>(sequenceNumber);
                    BtreeValue<int, Int32ValueBitConverter> value = new BtreeValue<int, Int32ValueBitConverter>(sequenceNumber);
                    IRedoUndoInformation infoRedoUndo = tree.InsertAsync(key, value, sequenceNumber, CancellationToken.None).Result;
                    LogHelper.Log("Insert redo buffers {0}", infoRedoUndo.Redo.Count());
                    LogHelper.Log("Insert undo buffers {0}", infoRedoUndo.Undo.Count());
                }

                //
                // Close the btree
                //
                LogHelper.Log("Begin close btree");
                tree.CloseAsync(true, CancellationToken.None).Wait();
                LogHelper.Log("End close btree");
            },
           "OpenCloseInsert");
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void OpenCloseInsertConcurrent()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                Uri owner = new Uri("fabric://test/btree");
                LogHelper.Log("Begin construct btree");
                Btree<int, int, Int32KeyBitConverter, Int32ValueBitConverter> tree = new Btree<int, int, Int32KeyBitConverter, Int32ValueBitConverter>();
                LogHelper.Log("End construct btree");

                //
                // Key configuration
                //
                LogHelper.Log("Start create key description");
                KeyComparisonDescription keyDescription = new KeyComparisonDescription();
                keyDescription.CultureInfo = Globalization.CultureInfo.InvariantCulture;
                keyDescription.IsFixedLengthKey = true;
                keyDescription.KeyDataType = (int)DataType.Int32;
                keyDescription.MaximumKeySizeInBytes = sizeof(int);
                LogHelper.Log("End create key description");

                //
                // Storage configuration
                //
                LogHelper.Log("Start create storage description");
                BtreeStorageConfigurationDescription storageDescription = new BtreeStorageConfigurationDescription();
                storageDescription.IsVolatile = true;
                storageDescription.MaximumMemoryInMB = 1024;
                storageDescription.MaximumPageSizeInKB = 4;
                storageDescription.MaximumStorageInMB = 0;
                storageDescription.RetriesBeforeTimeout = 100;
                storageDescription.StoreDataInline = true;
                LogHelper.Log("End create key description");

                //
                // Btree configuration
                //
                LogHelper.Log("Start create btree description");
                BtreeConfigurationDescription btreeConfig = new BtreeConfigurationDescription();
                btreeConfig.KeyComparison = keyDescription;
                btreeConfig.StorageConfiguration = storageDescription;
                btreeConfig.PartitionId = Guid.NewGuid();
                btreeConfig.ReplicaId = 130083631515748206;
                LogHelper.Log("End create btree description");

                //
                // Open the btree
                //
                LogHelper.Log("Begin open btree");
                tree.OpenAsync(btreeConfig, false, CancellationToken.None).Wait();
                LogHelper.Log("End open btree");

                //
                // Insert into the btree
                //
                int max = 128;
                List<Task<IRedoUndoInformation>> tasks = new List<Task<IRedoUndoInformation>>();
                DateTime dtStart = DateTime.UtcNow;
                for (int sequenceNumber = 0; sequenceNumber < max; sequenceNumber++)
                {
                    BtreeKey<int, Int32KeyBitConverter> key = new BtreeKey<int, Int32KeyBitConverter>(sequenceNumber);
                    BtreeValue<int, Int32ValueBitConverter> value = new BtreeValue<int, Int32ValueBitConverter>(sequenceNumber);
                    Task<IRedoUndoInformation> newInsertTask = tree.InsertAsync(key, value, sequenceNumber, CancellationToken.None);
                    tasks.Add(newInsertTask);
                }
                for (int sequenceNumber = 0; sequenceNumber < max; sequenceNumber++)
                {
                    tasks[sequenceNumber].Wait();
                }
                DateTime dtEnd = DateTime.UtcNow;
                LogHelper.Log((dtEnd - dtStart).TotalMilliseconds.ToString());
                tree.OnOperationStableAsync(max / 2, CancellationToken.None).Wait();
                BtreeStatistics stats = tree.Statistics;
                LogHelper.Log("Items {0}", stats.RecordCount);


                //
                // Close the btree
                //
                LogHelper.Log("Begin close btree");
                tree.CloseAsync(true, CancellationToken.None).Wait();
                LogHelper.Log("End close btree");
           },
           "OpenCloseInsertConcurrent");
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void OpenCloseInsertGuidKeyStringValue()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                Uri owner = new Uri("fabric://test/btree");
                LogHelper.Log("Begin construct btree");
                Btree<Guid, string, GuidKeyBitConverter, UnicodeStringValueBitConverter> tree = new Btree<Guid, string, GuidKeyBitConverter, UnicodeStringValueBitConverter>();
                LogHelper.Log("End construct btree");

                //
                // Key configuration
                //
                LogHelper.Log("Start create key description");
                GuidKeyComparison guidKeyComparison = new GuidKeyComparison();
                LogHelper.Log("End create key description");

                //
                // Storage configuration
                //
                LogHelper.Log("Start create storage description");
                BtreeStorageConfigurationDescription storageDescription = new BtreeStorageConfigurationDescription();
                storageDescription.IsVolatile = true;
                storageDescription.MaximumMemoryInMB = 1024;
                storageDescription.MaximumPageSizeInKB = 4;
                storageDescription.MaximumStorageInMB = 0;
                storageDescription.RetriesBeforeTimeout = 100;
                storageDescription.StoreDataInline = true;
                LogHelper.Log("End create key description");

                //
                // Btree configuration
                //
                LogHelper.Log("Start create btree description");
                BtreeConfigurationDescription btreeConfig = new BtreeConfigurationDescription();
                btreeConfig.KeyComparison = guidKeyComparison.KeyComparison;
                btreeConfig.StorageConfiguration = storageDescription;
                btreeConfig.PartitionId = Guid.NewGuid();
                btreeConfig.ReplicaId = 130083631515748206;
                LogHelper.Log("End create btree description");

                //
                // Open the btree
                //
                LogHelper.Log("Begin open btree");
                tree.OpenAsync(btreeConfig, false, CancellationToken.None).Wait();
                LogHelper.Log("End open btree");

                //
                // Insert into the btree
                //
                for (int sequenceNumber = 0; sequenceNumber < 16; sequenceNumber++)
                {
                    BtreeKey<Guid, GuidKeyBitConverter> key = new BtreeKey<Guid, GuidKeyBitConverter>(Guid.NewGuid());
                    BtreeValue<string, UnicodeStringValueBitConverter> value = new BtreeValue<string, UnicodeStringValueBitConverter>(sequenceNumber.ToString());
                    IRedoUndoInformation infoRedoUndo = tree.InsertAsync(key, value, sequenceNumber, CancellationToken.None).Result;
                }

                //
                // Close the btree
                //
                LogHelper.Log("Begin close btree");
                tree.CloseAsync(true, CancellationToken.None).Wait();
                LogHelper.Log("End close btree");
            },
           "OpenCloseInsertGuidKeyStringValue");
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void OpenCloseInsertStringKeyGuidValue()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                Uri owner = new Uri("fabric://test/btree");
                LogHelper.Log("Begin construct btree");
                Btree<string, Guid, UnicodeStringKeyBitConverter, GuidValueBitConverter> tree = new Btree<string, Guid, UnicodeStringKeyBitConverter, GuidValueBitConverter>();
                LogHelper.Log("End construct btree");

                //
                // Key configuration
                //
                LogHelper.Log("Start create key description");
                StringKeyComparisonEnglishUs stringKeyComparison = new StringKeyComparisonEnglishUs();
                LogHelper.Log("End create key description");

                //
                // Storage configuration
                //
                LogHelper.Log("Start create storage description");
                BtreeStorageConfigurationDescription storageDescription = new BtreeStorageConfigurationDescription();
                storageDescription.IsVolatile = true;
                storageDescription.MaximumMemoryInMB = 1 << 16;
                storageDescription.MaximumPageSizeInKB = 64;
                storageDescription.MaximumStorageInMB = 0;
                storageDescription.RetriesBeforeTimeout = 100;
                storageDescription.StoreDataInline = true;
                LogHelper.Log("End create key description");

                //
                // Btree configuration
                //
                LogHelper.Log("Start create btree description");
                BtreeConfigurationDescription btreeConfig = new BtreeConfigurationDescription();
                btreeConfig.KeyComparison = stringKeyComparison.KeyComparison;
                btreeConfig.StorageConfiguration = storageDescription;
                btreeConfig.PartitionId = Guid.NewGuid();
                btreeConfig.ReplicaId = 130083631515748206;
                LogHelper.Log("End create btree description");

                //
                // Open the btree
                //
                LogHelper.Log("Begin open btree");
                tree.OpenAsync(btreeConfig, true, CancellationToken.None).Wait();
                LogHelper.Log("End open btree");

                //
                // Insert into the btree
                //
                for (int sequenceNumber = 0; sequenceNumber < 16; sequenceNumber++)
                {
                    BtreeKey<string, UnicodeStringKeyBitConverter> key = new BtreeKey<string, UnicodeStringKeyBitConverter>(sequenceNumber.ToString());
                    BtreeValue<Guid, GuidValueBitConverter> value = new BtreeValue<Guid, GuidValueBitConverter>(Guid.NewGuid());
                    IRedoUndoInformation infoRedoUndo = tree.InsertAsync(key, value, sequenceNumber, CancellationToken.None).Result;
                }

                //
                // Close the btree
                //
                LogHelper.Log("Begin close btree");
                tree.CloseAsync(true, CancellationToken.None).Wait();
                LogHelper.Log("End close btree");
            },
           "OpenCloseInsertStringKeyGuidValue");
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void OpenCloseInsertApply()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                Uri owner = new Uri("fabric://test/btree");
                LogHelper.Log("Begin construct btrees");
                Btree<int, int, Int32KeyBitConverter, Int32ValueBitConverter> tree = new Btree<int, int, Int32KeyBitConverter, Int32ValueBitConverter>();
                Btree<int, int, Int32KeyBitConverter, Int32ValueBitConverter> treeApply = new Btree<int, int, Int32KeyBitConverter, Int32ValueBitConverter>();
                LogHelper.Log("End construct btrees");

                //
                // Key configuration
                //
                LogHelper.Log("Start create key description");
                KeyComparisonDescription keyDescription = new KeyComparisonDescription();
                keyDescription.CultureInfo = Globalization.CultureInfo.InvariantCulture;
                keyDescription.IsFixedLengthKey = true;
                keyDescription.KeyDataType = (int)DataType.Int32;
                keyDescription.MaximumKeySizeInBytes = sizeof(int);
                LogHelper.Log("End create key description");

                //
                // Storage configuration
                //
                LogHelper.Log("Start create storage description");
                BtreeStorageConfigurationDescription storageDescription = new BtreeStorageConfigurationDescription();
                storageDescription.IsVolatile = true;
                storageDescription.MaximumMemoryInMB = 1024;
                storageDescription.MaximumPageSizeInKB = 4;
                storageDescription.MaximumStorageInMB = 0;
                storageDescription.RetriesBeforeTimeout = 100;
                storageDescription.StoreDataInline = true;
                LogHelper.Log("End create key description");

                Guid treeGuid = Guid.NewGuid();
                Guid treeApplyGuid = Guid.NewGuid();

                //
                // Btree configuration
                //
                LogHelper.Log("Start create btree description");
                BtreeConfigurationDescription btreeConfig = new BtreeConfigurationDescription();
                btreeConfig.KeyComparison = keyDescription;
                btreeConfig.StorageConfiguration = storageDescription;
                btreeConfig.PartitionId = treeGuid;
                btreeConfig.ReplicaId = 130083631515748206;
                LogHelper.Log("End create btree description");

                //
                // Open the btrees
                //
                LogHelper.Log("Begin open btree {0}", treeGuid);
                tree.OpenAsync(btreeConfig, false, CancellationToken.None).Wait();
                LogHelper.Log("End open btree {0}", treeGuid);

                LogHelper.Log("Begin open btree {0}", treeApplyGuid);
                btreeConfig.PartitionId = treeApplyGuid;
                treeApply.OpenAsync(btreeConfig, false, CancellationToken.None).Wait();
                LogHelper.Log("End open btree {0}", treeApplyGuid);

                //
                // Insert/Apply into the btrees
                //
                DateTime dtStart = DateTime.UtcNow;
                for (int sequenceNumber = 0; sequenceNumber < 1024; sequenceNumber++)
                {
                    BtreeKey<int, Int32KeyBitConverter> key = new BtreeKey<int, Int32KeyBitConverter>(sequenceNumber);
                    BtreeValue<int, Int32ValueBitConverter> value = new BtreeValue<int, Int32ValueBitConverter>(sequenceNumber);
                    IRedoUndoInformation infoRedoUndo = tree.InsertAsync(key, value, sequenceNumber, CancellationToken.None).Result;
                    if (3 != infoRedoUndo.Redo.Count())
                    {
                        LogHelper.Log("Insert redo buffers {0}", infoRedoUndo.Redo.Count());
                        throw new InvalidOperationException();
                    }
                    if (3 != infoRedoUndo.Undo.Count())
                    {
                        LogHelper.Log("Insert undo buffers {0}", infoRedoUndo.Undo.Count());
                        throw new InvalidOperationException();
                    }
                    IBtreeOperation btreeOperation = treeApply.ApplyWithOutputAsync(sequenceNumber, infoRedoUndo.Redo, true, CancellationToken.None).Result;
                    IBtreeOperation btreeOperation2 = treeApply.ApplyWithOutputAsync(sequenceNumber, infoRedoUndo.Redo, false, CancellationToken.None).Result;
                    BtreeKey<int, Int32KeyBitConverter> keyDecodeOnly = new BtreeKey<int, Int32KeyBitConverter>(btreeOperation.Key.Bytes);
                    BtreeKey<int, Int32KeyBitConverter> keyDecode = new BtreeKey<int, Int32KeyBitConverter>(btreeOperation2.Key.Bytes);
                    BtreeValue<int, Int32ValueBitConverter> valueDecodeOnly = new BtreeValue<int, Int32ValueBitConverter>(btreeOperation.Value.Bytes);
                    BtreeValue<int, Int32ValueBitConverter> valueDecode = new BtreeValue<int, Int32ValueBitConverter>(btreeOperation2.Value.Bytes);
                    if (keyDecodeOnly.Key != keyDecode.Key || key.Key != keyDecode.Key)
                    {
                        throw new InvalidOperationException();
                    }
                    if (valueDecodeOnly.Value != valueDecode.Value || valueDecode.Value != value.Value)
                    {
                        throw new InvalidOperationException();
                    }
                }
                DateTime dtEnd = DateTime.UtcNow;
                BtreeStatistics stats = tree.Statistics;
                LogHelper.Log("Items {0}", stats.RecordCount);
                LogHelper.Log((dtEnd - dtStart).TotalMilliseconds.ToString());

                //
                // Close the btrees
                //
                LogHelper.Log("Begin close btree {0}", treeGuid);
                tree.CloseAsync(true, CancellationToken.None).Wait();
                LogHelper.Log("End close btree {0}", treeGuid);

                LogHelper.Log("Begin close btree {0}", treeApplyGuid);
                treeApply.CloseAsync(true, CancellationToken.None).Wait();
                LogHelper.Log("End close btree {0}", treeApplyGuid);
            },
           "OpenCloseInsertApply");
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void OpenCloseInsertSeekDeleteGuidKeyStringValue()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                Uri owner = new Uri("fabric://test/btree");
                LogHelper.Log("Begin construct btree");
                Btree<Guid, string, GuidKeyBitConverter, UnicodeStringValueBitConverter> tree = new Btree<Guid, string, GuidKeyBitConverter, UnicodeStringValueBitConverter>();
                Btree<Guid, string, GuidKeyBitConverter, UnicodeStringValueBitConverter> treeApply = new Btree<Guid, string, GuidKeyBitConverter, UnicodeStringValueBitConverter>();
                LogHelper.Log("End construct btree");

                //
                // Key configuration
                //
                LogHelper.Log("Start create key description");
                GuidKeyComparison guidKeyComparison = new GuidKeyComparison();
                LogHelper.Log("End create key description");

                //
                // Storage configuration
                //
                LogHelper.Log("Start create storage description");
                BtreeStorageConfigurationDescription storageDescription = new BtreeStorageConfigurationDescription();
                storageDescription.IsVolatile = true;
                storageDescription.MaximumMemoryInMB = 1024;
                storageDescription.MaximumPageSizeInKB = 4;
                storageDescription.MaximumStorageInMB = 0;
                storageDescription.RetriesBeforeTimeout = 100;
                storageDescription.StoreDataInline = true;
                LogHelper.Log("End create key description");

                //
                // Btree configuration
                //
                LogHelper.Log("Start create btree description");
                BtreeConfigurationDescription btreeConfig = new BtreeConfigurationDescription();
                btreeConfig.KeyComparison = guidKeyComparison.KeyComparison;
                btreeConfig.StorageConfiguration = storageDescription;
                btreeConfig.PartitionId = Guid.NewGuid();
                btreeConfig.ReplicaId = 130083631515748206;
                LogHelper.Log("End create btree description");

                //
                // Open the btree
                //
                LogHelper.Log("Begin open btree");
                tree.OpenAsync(btreeConfig, false, CancellationToken.None).Wait();
                btreeConfig.PartitionId = Guid.NewGuid();
                treeApply.OpenAsync(btreeConfig, false, CancellationToken.None).Wait();
                LogHelper.Log("End open btree");

                long sequenceNumber = 1;

                BtreeKey<Guid, GuidKeyBitConverter> key1 = new BtreeKey<Guid, GuidKeyBitConverter>(Guid.NewGuid());
                BtreeKey<Guid, GuidKeyBitConverter> key2 = new BtreeKey<Guid, GuidKeyBitConverter>(Guid.NewGuid());
                BtreeValue<string, UnicodeStringValueBitConverter> value1 = new BtreeValue<string, UnicodeStringValueBitConverter>("value");
                BtreeValue<string, UnicodeStringValueBitConverter> value2 = new BtreeValue<string, UnicodeStringValueBitConverter>(null as string);

                //
                // Insert into the btree
                //
                IRedoUndoInformation infoRedoUndoInsert1 = tree.InsertAsync(key1, value1, sequenceNumber++, CancellationToken.None).Result;
                IBtreeOperation insertOperation11 = treeApply.ApplyWithOutputAsync(sequenceNumber, infoRedoUndoInsert1.Redo, true, CancellationToken.None).Result;
                IBtreeOperation insertOperation12 = treeApply.ApplyWithOutputAsync(sequenceNumber, infoRedoUndoInsert1.Redo, false, CancellationToken.None).Result;
                IRedoUndoInformation infoRedoUndoInsert2 = tree.InsertAsync(key2, value2, sequenceNumber++, CancellationToken.None).Result;
                //
                // Insert again the same key from the btree
                //
                try
                {
                    tree.InsertAsync(key2, value2, sequenceNumber++, CancellationToken.None).Wait();
                    Debug.Assert(false);
                }
                catch (AggregateException e)
                {
                    if (!(e.InnerException is ArgumentException))
                    {
                        Debug.Assert(false);
                    }
                }
                //
                // Read from the btree
                //
                IBtreeValue seekValue1 = tree.SeekAsync(key1, CancellationToken.None).Result;
                IBtreeValue seekValue2 = tree.SeekAsync(key2, CancellationToken.None).Result;
                byte[] seekValue2InBytes = seekValue2.Bytes;
                if (null != seekValue2InBytes)
                {
                    LogHelper.Log("Seek value {0}", seekValue2InBytes.Length);
                    throw new InvalidOperationException();
                }
                //
                // Delete from the btree
                //
                Tuple<bool, IRedoUndoInformation> infoRedoUndoDelete1 = tree.DeleteAsync(key1, sequenceNumber++, CancellationToken.None).Result;
                if (3 != infoRedoUndoDelete1.Item2.Redo.Count())
                {
                    LogHelper.Log("Delete redo buffers {0}", infoRedoUndoDelete1.Item2.Redo.Count());
                    throw new InvalidOperationException();
                }
                if (3 != infoRedoUndoDelete1.Item2.Undo.Count())
                {
                    LogHelper.Log("Delete undo buffers {0}", infoRedoUndoDelete1.Item2.Undo.Count());
                    throw new InvalidOperationException();
                }
                Tuple<bool, IRedoUndoInformation> infoRedoUndoDelete2 = tree.DeleteAsync(key2, sequenceNumber++, CancellationToken.None).Result;
                if (3 != infoRedoUndoDelete2.Item2.Redo.Count())
                {
                    LogHelper.Log("Delete redo buffers {0}", infoRedoUndoDelete2.Item2.Redo.Count());
                    throw new InvalidOperationException();
                }
                if (3 != infoRedoUndoDelete2.Item2.Undo.Count())
                {
                    LogHelper.Log("Delete undo buffers {0}", infoRedoUndoDelete2.Item2.Undo.Count());
                    throw new InvalidOperationException();
                }
                IBtreeOperation deleteOperation11 = treeApply.ApplyWithOutputAsync(sequenceNumber, infoRedoUndoDelete1.Item2.Redo, true, CancellationToken.None).Result;
                IBtreeOperation deleteOperation12 = treeApply.ApplyWithOutputAsync(sequenceNumber, infoRedoUndoDelete1.Item2.Redo, false, CancellationToken.None).Result;
                //
                // Delete from the btree
                //
                try
                {
                    tree.DeleteAsync(key1, sequenceNumber++, CancellationToken.None).Wait();
                    Debug.Assert(false);
                }
                catch (AggregateException e)
                {
                    if (!(e.InnerException is KeyNotFoundException))
                    {
                        Debug.Assert(false);
                    }
                }
                //
                // Read from the btree
                //
                try
                {
                    tree.SeekAsync(key1, CancellationToken.None).Wait();
                    Debug.Assert(false);
                }
                catch (AggregateException e)
                {
                    if (!(e.InnerException is KeyNotFoundException))
                    {
                        Debug.Assert(false);
                    }
                }

                //
                // Close the btree
                //
                LogHelper.Log("Begin close btree");
                tree.CloseAsync(true, CancellationToken.None).Wait();
                treeApply.CloseAsync(true, CancellationToken.None).Wait();
                LogHelper.Log("End close btree");
            },
           "OpenCloseInsertSeekDeleteGuidKeyStringValue");
        }

        [VS.TestMethod]
        [VS.Owner("mtarta")]
        public void OpenCloseInsertPerf()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                Uri owner = new Uri("fabric://test/btree");
                LogHelper.Log("Begin construct btrees");
                Btree<int, int, Int32KeyBitConverter, Int32ValueBitConverter> tree = new Btree<int, int, Int32KeyBitConverter, Int32ValueBitConverter>();
                LogHelper.Log("End construct btrees");

                //
                // Key configuration
                //
                LogHelper.Log("Start create key description");
                KeyComparisonDescription keyDescription = new KeyComparisonDescription();
                keyDescription.CultureInfo = Globalization.CultureInfo.InvariantCulture;
                keyDescription.IsFixedLengthKey = true;
                keyDescription.KeyDataType = (int)DataType.Int32;
                keyDescription.MaximumKeySizeInBytes = sizeof(int);
                LogHelper.Log("End create key description");

                //
                // Storage configuration
                //
                LogHelper.Log("Start create storage description");
                BtreeStorageConfigurationDescription storageDescription = new BtreeStorageConfigurationDescription();
                storageDescription.IsVolatile = true;
                storageDescription.MaximumMemoryInMB = 1024;
                storageDescription.MaximumPageSizeInKB = 4;
                storageDescription.MaximumStorageInMB = 0;
                storageDescription.RetriesBeforeTimeout = 100;
                storageDescription.StoreDataInline = true;
                LogHelper.Log("End create key description");

                Guid treeGuid = Guid.NewGuid();
                Guid treeApplyGuid = Guid.NewGuid();

                //
                // Btree configuration
                //
                LogHelper.Log("Start create btree description");
                BtreeConfigurationDescription btreeConfig = new BtreeConfigurationDescription();
                btreeConfig.KeyComparison = keyDescription;
                btreeConfig.StorageConfiguration = storageDescription;
                btreeConfig.PartitionId = treeGuid;
                btreeConfig.ReplicaId = 130083631515748206;
                LogHelper.Log("End create btree description");

                //
                // Open the btrees
                //
                LogHelper.Log("Begin open btree {0}", treeGuid);
                tree.OpenAsync(btreeConfig, false, CancellationToken.None).Wait();
                LogHelper.Log("End open btree {0}", treeGuid);

                //
                // Insert/Apply into the btrees
                //
                List<Task> tasks = new List<Task>();
                DateTime dtStart = DateTime.UtcNow;
                for (int sequenceNumber = 0; sequenceNumber < 1024; sequenceNumber++)
                {
                    BtreeKey<int, Int32KeyBitConverter> key = new BtreeKey<int, Int32KeyBitConverter>(sequenceNumber);
                    BtreeValue<int, Int32ValueBitConverter> value = new BtreeValue<int, Int32ValueBitConverter>(sequenceNumber);
                    tasks.Add(tree.InsertAsync(key, value, sequenceNumber, CancellationToken.None));
                }
                Task.WaitAll(tasks.ToArray());
                DateTime dtEnd = DateTime.UtcNow;
                LogHelper.Log((dtEnd - dtStart).TotalMilliseconds.ToString());
                BtreeStatistics stats = tree.Statistics;
                LogHelper.Log("Items {0}", stats.RecordCount);

                //
                // Close the btrees
                //
                LogHelper.Log("Begin close btree {0}", treeGuid);
                tree.CloseAsync(true, CancellationToken.None).Wait();
                LogHelper.Log("End close btree {0}", treeGuid);
            },
           "OpenCloseInsertPerf");
        }
    }
}