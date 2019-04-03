// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TelemetryAggregation.Test
{
    using System.Collections.Generic;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class FieldAggregatorTest
    {
        [ClassInitialize]
        public static void ClassSetup(TestContext context)
        {
        }

        [TestMethod]
        public void TestMetricAggregations()
        {
            // metric aggregation types
            Aggregation.Kinds[] aggrKindsMetrics =
            {
                Aggregation.Kinds.Average,
                Aggregation.Kinds.Count,
                Aggregation.Kinds.Maximum,
                Aggregation.Kinds.Minimum,
                Aggregation.Kinds.Snapshot,
                Aggregation.Kinds.Sum,
                Aggregation.Kinds.Variance
            };

            FieldAggregationConfig fieldAggrConfig = new FieldAggregationConfig(
                new Field("metric", Field.Kind.Metric),
                aggrKindsMetrics);

            FieldAggregator fieldAggr = new FieldAggregator(fieldAggrConfig);

            // test single element
            fieldAggr.Aggregate(1234.0);

            Assert.AreEqual(1234.0, fieldAggr.Snapshot);
            Assert.AreEqual(1.0, fieldAggr.Count);
            Assert.AreEqual(1234.0, fieldAggr.Minimum);
            Assert.AreEqual(1234.0, fieldAggr.Maximum);
            Assert.AreEqual(1234.0, fieldAggr.Average);
            Assert.AreEqual(1234.0, fieldAggr.Sum);
            Assert.AreEqual(0.0, fieldAggr.Variance);

            // cleaning
            fieldAggr.Clear();

            // test - with more values
            for (int i = 0; i <= 100; i++)
            {
                double val = -50.0 + i;
                fieldAggr.Aggregate(val);
            }

            Assert.AreEqual(50.0, fieldAggr.Snapshot);
            Assert.AreEqual(101.0, fieldAggr.Count);
            Assert.AreEqual(-50.0, fieldAggr.Minimum);
            Assert.AreEqual(50.0, fieldAggr.Maximum);
            Assert.AreEqual(0.0, fieldAggr.Average);
            Assert.AreEqual(0.0, fieldAggr.Sum);
            Assert.AreEqual(858.5, fieldAggr.Variance);

            // test a 0 variance
            fieldAggr.Clear();
            for (int i = 0; i <= 100; i++)
            {
                double val = 10.0;
                fieldAggr.Aggregate(val);
            }

            Assert.AreEqual(10.0, fieldAggr.Snapshot);
            Assert.AreEqual(101.0, fieldAggr.Count);
            Assert.AreEqual(10.0, fieldAggr.Minimum);
            Assert.AreEqual(10.0, fieldAggr.Maximum);
            Assert.AreEqual(10.0, fieldAggr.Average);
            Assert.AreEqual(1010.0, fieldAggr.Sum);
            Assert.AreEqual(0.0, fieldAggr.Variance);
        }

        [TestMethod]
        public void TestPropertyAggregations()
        {
            // only Snapshot and Count are supported for properties
            Aggregation.Kinds[] aggrKindsProperties =
            {
                Aggregation.Kinds.Count,
                Aggregation.Kinds.Snapshot
            };

            FieldAggregationConfig fieldAggrConfig = new FieldAggregationConfig(
                new Field("prop", Field.Kind.Property),
                aggrKindsProperties);

            FieldAggregator fieldAggr = new FieldAggregator(fieldAggrConfig);

            for (int i = 0; i < 20; i++)
            {
                fieldAggr.Aggregate(string.Format("property-{0}", i));
            }

            fieldAggr.Aggregate("lastValue");

            Assert.AreEqual("lastValue", fieldAggr.PropertySnapshot);
            Assert.AreEqual(21, fieldAggr.Count);
        }

        // Tests that all aggregation types are initialized with 0
        [TestMethod]
        public void TestMetricCleanInitialization()
        {
            // metric aggregation types
            Aggregation.Kinds[] aggrKindsMetric =
            {
                Aggregation.Kinds.Average,
                Aggregation.Kinds.Count,
                Aggregation.Kinds.Maximum,
                Aggregation.Kinds.Minimum,
                Aggregation.Kinds.Snapshot,
                Aggregation.Kinds.Sum,
                Aggregation.Kinds.Variance
            };

            // only Snapshot and Count are supported for properties
            Aggregation.Kinds[] aggrKindsProperties =
            {
                Aggregation.Kinds.Count,
                Aggregation.Kinds.Snapshot
            };

            FieldAggregationConfig fieldAggrConfig = new FieldAggregationConfig(
                new Field("metric", Field.Kind.Metric),
                aggrKindsMetric);

            FieldAggregator fieldAggr = new FieldAggregator(fieldAggrConfig);

            Assert.AreEqual(double.NegativeInfinity, fieldAggr.Snapshot);
            Assert.AreEqual(0.0, fieldAggr.Count);
            Assert.AreEqual(double.PositiveInfinity, fieldAggr.Minimum);
            Assert.AreEqual(double.NegativeInfinity, fieldAggr.Maximum);
            Assert.AreEqual(double.NaN, fieldAggr.Average);
            Assert.AreEqual(0.0, fieldAggr.Sum);
            Assert.AreEqual(double.NaN, fieldAggr.Variance);
        }

        // Tests whether the property aggregation types are properly initialized
        [TestMethod]
        public void TestPropertyCleanInitialization()
        {
            // metric aggregation types
            Aggregation.Kinds[] aggrKindsProperty =
            {
                Aggregation.Kinds.Count,
                Aggregation.Kinds.Snapshot
            };

            // only Snapshot and Count are supported for properties
            Aggregation.Kinds[] aggrKindsProperties =
            {
                Aggregation.Kinds.Count,
                Aggregation.Kinds.Snapshot
            };

            FieldAggregationConfig fieldAggrConfig = new FieldAggregationConfig(
                new Field("prop", Field.Kind.Metric),
                aggrKindsProperty);

            FieldAggregator fieldAggr = new FieldAggregator(fieldAggrConfig);

            Assert.AreEqual(null, fieldAggr.PropertySnapshot);
            Assert.AreEqual(0.0, fieldAggr.Count);
        }

        // Tests that all aggregation types are initialized with 0
        [TestMethod]
        public void TestMetricClear()
        {
            // metric aggregation types
            Aggregation.Kinds[] aggrKindsMetric =
            {
                Aggregation.Kinds.Average,
                Aggregation.Kinds.Count,
                Aggregation.Kinds.Maximum,
                Aggregation.Kinds.Minimum,
                Aggregation.Kinds.Snapshot,
                Aggregation.Kinds.Sum,
                Aggregation.Kinds.Variance
            };

            // only Snapshot and Count are supported for properties
            Aggregation.Kinds[] aggrKindsProperties =
            {
                Aggregation.Kinds.Count,
                Aggregation.Kinds.Snapshot
            };

            FieldAggregationConfig fieldAggrConfig = new FieldAggregationConfig(
                new Field("metric", Field.Kind.Metric),
                aggrKindsMetric);

            FieldAggregator fieldAggr = new FieldAggregator(fieldAggrConfig);

            // test - variance
            for (int i = 0; i <= 10; i++)
            {
                double val = -50.0 + i;
                fieldAggr.Aggregate(val);
            }

            // cleaning aggregations
            fieldAggr.Clear();

            Assert.AreEqual(double.NegativeInfinity, fieldAggr.Snapshot);
            Assert.AreEqual(0.0, fieldAggr.Count);
            Assert.AreEqual(double.PositiveInfinity, fieldAggr.Minimum);
            Assert.AreEqual(double.NegativeInfinity, fieldAggr.Maximum);
            Assert.AreEqual(double.NaN, fieldAggr.Average);
            Assert.AreEqual(0.0, fieldAggr.Sum);
            Assert.AreEqual(double.NaN, fieldAggr.Variance);

            // testing whether aggregating again doesn't fail after clearing
            for (int i = 0; i <= 100; i++)
            {
                double val = 10.0;
                fieldAggr.Aggregate(val);
            }

            Assert.AreEqual(10.0, fieldAggr.Snapshot);
            Assert.AreEqual(101.0, fieldAggr.Count);
            Assert.AreEqual(10.0, fieldAggr.Minimum);
            Assert.AreEqual(10.0, fieldAggr.Maximum);
            Assert.AreEqual(10.0, fieldAggr.Average);
            Assert.AreEqual(1010.0, fieldAggr.Sum);
            Assert.AreEqual(0.0, fieldAggr.Variance);
        }

        // Tests whether the property aggregation types are properly initialized
        [TestMethod]
        public void TestPropertyClear()
        {
            // only Snapshot and Count are supported for properties
            Aggregation.Kinds[] aggrKindsProperties =
            {
                Aggregation.Kinds.Count,
                Aggregation.Kinds.Snapshot
            };

            FieldAggregationConfig fieldAggrConfig = new FieldAggregationConfig(
                new Field("prop", Field.Kind.Property),
                aggrKindsProperties);

            FieldAggregator fieldAggr = new FieldAggregator(fieldAggrConfig);

            for (int i = 0; i < 20; i++)
            {
                fieldAggr.Aggregate(string.Format("property-{0}", i));
            }

            // Cleaning aggregations
            fieldAggr.Clear();

            Assert.AreEqual(null, fieldAggr.PropertySnapshot);
            Assert.AreEqual(0.0, fieldAggr.Count);

            // testing whether aggregating again doesn't fail after clearing
            for (int i = 0; i < 5; i++)
            {
                fieldAggr.Aggregate(string.Format("property-{0}", i));
            }

            fieldAggr.Aggregate("lastValue");

            Assert.AreEqual("lastValue", fieldAggr.PropertySnapshot);
            Assert.AreEqual(6, fieldAggr.Count);
        }
    }
}