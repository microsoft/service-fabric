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
    public class ItemListTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ItemListAddOperationTest()
        {
            PerformReadOnlyTest((list) => list.Add("Test string"), new string[] { string.Empty });
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ItemListRemoveOperationTest()
        {
            PerformReadOnlyTest((list) => list.Remove(string.Empty), new string[] { string.Empty });
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ItemListRemoveAtOperationTest()
        {
            PerformReadOnlyTest((list) => list.RemoveAt(0), new string[] { string.Empty });
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ItemListClearOperationTest()
        {
            PerformReadOnlyTest((list) => list.Clear(), new string[] { string.Empty });
        }

        private static void PerformReadOnlyTest(Action<IList<string>> listOperation, IEnumerable<string> createFrom)
        {
            ItemList<string> testListReadOnly = new ItemList<string>(false, false, new List<string>(createFrom));
            TestUtility.ExpectException<NotSupportedException>(
                () => listOperation(testListReadOnly.AsReadOnly()));

            ItemList<string> testListWritable = new ItemList<string>(false, false, new List<string>(createFrom));
            listOperation(testListWritable);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ItemListNullDataTest()
        {
            ItemList<string> testList = new ItemList<string>(false, false, new List<string>() { string.Empty });
            TestUtility.ExpectException<ArgumentNullException>(
                () => testList.Add(null));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ItemListDuplicateDataTest()
        {
            ItemList<string> testList = new ItemList<string>(false, false, new List<string>() { string.Empty });
            TestUtility.ExpectException<ArgumentException>(
                () => testList.Add(string.Empty));
        }
    }
}