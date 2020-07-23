// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class ItemCollectionTest
    {
        private static KeyedItemCollection<string, MyClass> CreateNew()
        {
            return new KeyedItemCollection<string, MyClass>((c) => c.Key);
        }

        private static KeyedItemCollection<string, MyClass> CreateNew(MyClass obj)
        {
            return new KeyedItemCollection<string, MyClass>((c) => c.Key) { obj };
        }

        private static KeyedItemCollection<string, MyClass> Clone(KeyedItemCollection<string, MyClass> init)
        {
            var collection = ItemCollectionTest.CreateNew();
            if (init != null)
            {
                foreach (var item in init)
                {
                    collection.Add(item);
                }
            }

            return collection;
        }

        private static readonly MyClass object1 = new MyClass { Key = "a" };
        private static readonly MyClass object2 = new MyClass { Key = "b" };

        private static IEnumerable<MyClass> GetMultiple()
        {
            yield return ItemCollectionTest.object1;
            yield return ItemCollectionTest.object2;
        }


        // run the delegate
        private void TestHelper2(Action<KeyedItemCollection<string, MyClass>> action, KeyedItemCollection<string, MyClass> collection)
        {
            action(collection);
        }

        private void TestHelper(string actionName, Action<KeyedItemCollection<string, MyClass>> action, bool isReadonlyExceptionExpected, KeyedItemCollection<string, MyClass> collection = null)
        {
            LogHelper.Log("Testing {0} ", actionName);
            this.TestHelper2(action, ItemCollectionTest.Clone(collection));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ItemCollection_AddingAndFetchingItemTest()
        {
            this.TestHelper(
                "AddAndFetch",
                (col) =>
                {
                    col.Add(ItemCollectionTest.object1);
                    Assert.AreSame(ItemCollectionTest.object1, col[ItemCollectionTest.object1.Key]);
                },
                true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ItemCollection_ClearTest()
        {
            this.TestHelper(
                "Clear",
                (col) =>
                {
                    col.Clear();
                    Assert.AreEqual(0, col.Count);
                },
                true,
                ItemCollectionTest.CreateNew(ItemCollectionTest.object1));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ItemCollection_ContainsTest()
        {
            this.TestHelper(
                "Contains",
                (col) =>
                {
                    Assert.IsTrue(col.Contains(ItemCollectionTest.object1.Key));
                },
                false,
                ItemCollectionTest.CreateNew(ItemCollectionTest.object1));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ItemCollection_RemoveItemTest()
        {
            this.TestHelper(
                "RemoveItem",
                (col) =>
                {
                    col.Remove(ItemCollectionTest.object1);
                    Assert.AreEqual(0, col.Count);
                },
                true,
                ItemCollectionTest.CreateNew(ItemCollectionTest.object1));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ItemCollection_IndexterTest()
        {
            this.TestHelper(
                "Indexer",
                (col) =>
                {
                    Assert.AreSame(ItemCollectionTest.object1, col[ItemCollectionTest.object1.Key]);
                },
                false,
                ItemCollectionTest.CreateNew(ItemCollectionTest.object1));
        }

        class MyClass
        {
            public string Key { get; set; }
        }
    }
}