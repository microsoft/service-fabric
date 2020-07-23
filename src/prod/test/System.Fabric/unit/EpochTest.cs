// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System.Fabric;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class EpochTest
    {

        #region Equality

        private readonly Func<Epoch, Epoch, bool>[] equalityComparisons = new Func<Epoch, Epoch, bool>[]
        {
            (e1, e2) => e1 == e2,
            (e1, e2) => e1.Equals(e2),
            (e1, e2) => object.Equals(e1, e2),
            (e1, e2) => ((IEquatable<Epoch>)e1).Equals(e2),
        };

        private readonly Func<Epoch, Epoch, bool>[] inEqualityComparisons = new Func<Epoch, Epoch, bool>[]
        {
            (e1, e2) => e1 != e2,
        };

        private void EqualsTestHelper(Epoch left, Epoch right)
        {
            foreach (var item in this.equalityComparisons)
            {
                Assert.IsTrue(item(left, right));
            }

            foreach (var item in this.inEqualityComparisons)
            {
                Assert.IsFalse(item(left, right));
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void EpochTest_NullReturnsFalse()
        {
            Epoch e1 = new Epoch(5, 3);
            Assert.IsFalse(e1.Equals(null));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void EpochTest_InvalidTypeReturnsFalse()
        {
            Epoch e1 = new Epoch(5, 3);
            Assert.IsFalse(e1.Equals(new object()));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void EpochTest_Equality_NonNull()
        {
            Epoch e1 = new Epoch(5, 3);
            Epoch e2 = new Epoch(5, 3);

            this.EqualsTestHelper(e1, e2);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void EpochTest_Equality_SameObject()
        {
            Epoch e1 = new Epoch(5, 3);

            this.EqualsTestHelper(e1, e1);
        }

        private void InEqualTestHelper(Epoch left, Epoch right)
        {
            foreach (var item in this.equalityComparisons)
            {
                Assert.IsFalse(item(left, right));
            }

            foreach (var item in this.inEqualityComparisons)
            {
                Assert.IsTrue(item(left, right));
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void EpochTest_Equality_Different()
        {
            Epoch e1 = new Epoch(5, 3);
            Epoch e2 = new Epoch(66, 1);

            this.InEqualTestHelper(e1, e2);
        }


        #endregion

        #region Comparison

        private readonly Func<Epoch, Epoch, bool>[] lessThanComparisons = new Func<Epoch, Epoch, bool>[]
        {
            (e1, e2) => e1 < e2,
            (e1, e2) => e1.CompareTo(e2) < 0,
        };

        private readonly Func<Epoch, Epoch, bool>[] lessThanEqualComparisons = new Func<Epoch, Epoch, bool>[]
        {
            (e1, e2) => e1 <= e2,
            (e1, e2) => e1.CompareTo(e2) <= 0,
        };

        private readonly Func<Epoch, Epoch, bool>[] greaterThanComparisons = new Func<Epoch, Epoch, bool>[]
        {
            (e1, e2) => e1 > e2,
            (e1, e2) => e1.CompareTo(e2) > 0,
        };

        private readonly Func<Epoch, Epoch, bool>[] greaterThanEqualComparisons = new Func<Epoch, Epoch, bool>[]
        {
            (e1, e2) => e1 >= e2,
            (e1, e2) => e1.CompareTo(e2) >= 0,
        };

        private void StrictlyLessThanTest(Epoch e1, Epoch e2)
        {
            foreach (var item in this.lessThanComparisons)
            {
                Assert.IsTrue(item(e1, e2));
            }

            foreach (var item in this.lessThanEqualComparisons)
            {
                Assert.IsTrue(item(e1, e2));
            }

            foreach (var item in this.greaterThanEqualComparisons)
            {
                Assert.IsFalse(item(e1, e2));
            }

            foreach (var item in this.greaterThanComparisons)
            {
                Assert.IsFalse(item(e1, e2));
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void Epoch_LessThanDataLossNumberIsLesser()
        {
            Epoch e1 = new Epoch(5, 199);
            Epoch e2 = new Epoch(6, 198);

            this.StrictlyLessThanTest(e1, e2);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void Epoch_EqualDataLossNumberComparesConfiguration()
        {
            Epoch e1 = new Epoch(5, 199);
            Epoch e2 = new Epoch(5, 200);

            this.StrictlyLessThanTest(e1, e2);
        }

        #endregion
    }
}