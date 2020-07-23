// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TelemetryAggregation.Test
{
    using System.Collections.Generic;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class TraceAggregatorTest
    {
        [ClassInitialize]
        public static void ClassSetup(TestContext context)
        {
        }

        [TestMethod]
        public void TestMetricsAggregations()
        {
            // initializing the configuration of the aggregation for a single field with all types of aggregation
            string[] properties = { };
            string[] metrics = { "metric1", "metric2" };

            Aggregation.Kinds[] aggrKindsProperties = { };

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

        TraceAggregationConfig traceAggrconfig = TraceAggregationConfig.CreateTraceAggregatorConfig(
                "TestTaskName",
                "TestEventName",
                string.Empty,
                properties,
                aggrKindsProperties,
                metrics,
                aggrKindsMetrics);

            // create the trace aggregator
            TraceAggregator traceAggr = new TraceAggregator(traceAggrconfig);

            // creating some metric values to test the aggregations
            KeyValuePair<string, double>[] metricValues =
            {
                new KeyValuePair<string, double>("metric1", 0.0),
                new KeyValuePair<string, double>("metric2", 1.0)
            };

            // aggregating
            traceAggr.Aggregate(metricValues);

            Dictionary<string, double> metricResults;
            Dictionary<string, string> propResults;

            traceAggr.CurrentFormattedFieldAggregationsValues(out propResults, out metricResults);

            // checking whether metric values are properly set 
            Assert.AreEqual(0.0, metricResults["metric1_Snapshot"]);
            Assert.AreEqual(1.0, metricResults["metric1_Count"]);
            Assert.AreEqual(0.0, metricResults["metric1_Minimum"]);
            Assert.AreEqual(0.0, metricResults["metric1_Maximum"]);
            Assert.AreEqual(0.0, metricResults["metric1_Sum"]);
            Assert.AreEqual(0.0, metricResults["metric1_Average"]);
            Assert.AreEqual(0.0, metricResults["metric1_Variance"]);

            Assert.AreEqual(1.0, metricResults["metric2_Snapshot"]);
            Assert.AreEqual(1.0, metricResults["metric2_Count"]);
            Assert.AreEqual(1.0, metricResults["metric2_Minimum"]);
            Assert.AreEqual(1.0, metricResults["metric2_Maximum"]);
            Assert.AreEqual(1.0, metricResults["metric2_Sum"]);
            Assert.AreEqual(1.0, metricResults["metric2_Average"]);
            Assert.AreEqual(0.0, metricResults["metric2_Variance"]);

            // creating second set of metrics to test the aggregations
            KeyValuePair<string, double>[] metricValues2 =
            {
                new KeyValuePair<string, double>("metric1", -1.0),
                new KeyValuePair<string, double>("metric2", 2.0),
                new KeyValuePair<string, double>("metric3", 3.0)
            };

            traceAggr.Aggregate(metricValues2);

            traceAggr.CurrentFormattedFieldAggregationsValues(out propResults, out metricResults);

            Assert.AreEqual(-1.0, metricResults["metric1_Snapshot"]);
            Assert.AreEqual(2.0, metricResults["metric1_Count"]);
            Assert.AreEqual(-1.0, metricResults["metric1_Minimum"]);
            Assert.AreEqual(0.0, metricResults["metric1_Maximum"]);
            Assert.AreEqual(-1.0, metricResults["metric1_Sum"]);
            Assert.AreEqual(-0.5, metricResults["metric1_Average"]);
            Assert.AreEqual(0.5, metricResults["metric1_Variance"]);

            Assert.AreEqual(2.0, metricResults["metric2_Snapshot"]);
            Assert.AreEqual(2.0, metricResults["metric2_Count"]);
            Assert.AreEqual(1.0, metricResults["metric2_Minimum"]);
            Assert.AreEqual(2.0, metricResults["metric2_Maximum"]);
            Assert.AreEqual(3.0, metricResults["metric2_Sum"]);
            Assert.AreEqual(1.5, metricResults["metric2_Average"]);
            Assert.AreEqual(0.5, metricResults["metric2_Variance"]);

            // checking whether "metric3" which is not part of of the configuration added values
            Assert.AreEqual(14, metricResults.Values.Count);

            // checking whether any value was added by mistake to the properties
            Assert.IsTrue(propResults.Count == 0);
        }

        [TestMethod]
        public void TestPropertiesAggregations()
        {
            // initializing the configuration of the aggregation for a single field with all types of aggregation
            string[] properties = { "prop1", "prop2", "prop3", "prop4" };
            string[] metrics = { };

            Aggregation.Kinds[] aggrKindsProperties =
            {
                Aggregation.Kinds.Count,
                Aggregation.Kinds.Snapshot
            };

            Aggregation.Kinds[] aggrKindsMetrics = { };

            TraceAggregationConfig traceAggrconfig = TraceAggregationConfig.CreateTraceAggregatorConfig(
                    "TestTaskName",
                    "TestEventName",
                    string.Empty,
                    properties,
                    aggrKindsProperties,
                    metrics,
                    aggrKindsMetrics);

            // create the trace aggregator
            TraceAggregator traceAggr = new TraceAggregator(traceAggrconfig);

            // creating some property values to test the aggregations
            KeyValuePair<string, double>[] metricValues = { };

            KeyValuePair<string, string>[] propertyValues =
            {
                new KeyValuePair<string, string>("prop1", null),
                new KeyValuePair<string, string>("prop2", string.Empty),
                new KeyValuePair<string, string>("prop3", "test1"),
                new KeyValuePair<string, string>("prop4", "test2")
            };

            // aggregating
            traceAggr.Aggregate(propertyValues);

            Dictionary<string, double> metricResults;
            Dictionary<string, string> propResults;

            traceAggr.CurrentFormattedFieldAggregationsValues(out propResults, out metricResults);

            // checking whether metric values are properly set 
            Assert.AreEqual(null, propResults["prop1_Snapshot"]);
            Assert.AreEqual(string.Empty, propResults["prop2_Snapshot"]);
            Assert.AreEqual("test1", propResults["prop3_Snapshot"]);
            Assert.AreEqual("test2", propResults["prop4_Snapshot"]);

            // the count should go to the metrics since it is a number
            Assert.IsFalse(propResults.ContainsKey("prop1_Count"));
            Assert.IsFalse(propResults.ContainsKey("prop2_Count"));
            Assert.IsFalse(propResults.ContainsKey("prop3_Count"));
            Assert.IsFalse(propResults.ContainsKey("prop4_Count"));

            // checks that all have count of 1
            Assert.AreEqual(1.0, metricResults["prop1_Count"]);
            Assert.AreEqual(1.0, metricResults["prop2_Count"]);
            Assert.AreEqual(1.0, metricResults["prop3_Count"]);
            Assert.AreEqual(1.0, metricResults["prop4_Count"]);

            // creating second set of propertis to test the aggregations
            KeyValuePair<string, string>[] propertyValues2 =
            {
                new KeyValuePair<string, string>("prop4", "test2NewValue"),
                new KeyValuePair<string, string>("prop5", "notPartOfAggregator")
            };

            traceAggr.Aggregate(propertyValues2);

            traceAggr.CurrentFormattedFieldAggregationsValues(out propResults, out metricResults);

            // checking whether new values are properly set prop1, prop2, prop3 should have the same value
            Assert.AreEqual(null, propResults["prop1_Snapshot"]);
            Assert.AreEqual(string.Empty, propResults["prop2_Snapshot"]);
            Assert.AreEqual("test1", propResults["prop3_Snapshot"]);
            Assert.AreEqual("test2NewValue", propResults["prop4_Snapshot"]);

            Assert.IsFalse(metricResults.ContainsKey("prop5_Count"));
            Assert.IsFalse(propResults.ContainsKey("prop5_Count"));
            Assert.IsFalse(propResults.ContainsKey("prop5_Snapshot"));

            // checks that all have count of 1 but prop4 which should have 2
            Assert.AreEqual(1.0, metricResults["prop1_Count"]);
            Assert.AreEqual(1.0, metricResults["prop2_Count"]);
            Assert.AreEqual(1.0, metricResults["prop3_Count"]);
            Assert.AreEqual(2.0, metricResults["prop4_Count"]);

            // checking whether "metric3" which is not part of of the configuration added values
            Assert.AreEqual(4, metricResults.Values.Count);
            Assert.AreEqual(4, propResults.Values.Count);
        }
    }
}