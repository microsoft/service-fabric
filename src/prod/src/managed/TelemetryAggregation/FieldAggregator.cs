// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TelemetryAggregation
{
    using System;
    using System.Collections.Generic;
    using Newtonsoft.Json;

    // this class is responsible for performing the aggregation of a single field of the trace
    // it only computes the necessary information to produce the aggregations it is configured to do.
    [JsonObject(MemberSerialization.Fields)]
    public class FieldAggregator
    {
        private FieldAggregationConfig fieldAggrConfig;

        // aggregator types enabled
        private bool snapshotEnabled;
        private bool averageEnabled;
        private bool varianceEnabled;
        private bool minimumEnabled;
        private bool maximumEnabled;
        private bool sumEnabled;
        private bool countEnabled;

        // variance is computed as a function of sum, count, average, and partialVariance (which is basically the sum of the squared values)
        private double partialVariance;

        // Initializes the object by specifying the aggregation types required.
        public FieldAggregator(FieldAggregationConfig fieldAggrConfig)
        {
            this.fieldAggrConfig = fieldAggrConfig;
            this.Clear();

            // if the field type is a number it supports also the following types of aggregations
            if (this.fieldAggrConfig.Field.FieldKind != Field.Kind.Property)
            {
                // setting up aggregator types as booleans to make code slightly more efficient
                this.minimumEnabled = (Aggregation.Kinds.Minimum & this.fieldAggrConfig.AggregationTypesFlag) != 0;
                this.maximumEnabled = (Aggregation.Kinds.Maximum & this.fieldAggrConfig.AggregationTypesFlag) != 0;
                this.varianceEnabled = (Aggregation.Kinds.Variance & this.fieldAggrConfig.AggregationTypesFlag) != 0;
                this.averageEnabled = this.varianceEnabled || (Aggregation.Kinds.Average & this.fieldAggrConfig.AggregationTypesFlag) != 0;
                this.sumEnabled = this.averageEnabled || this.varianceEnabled || (Aggregation.Kinds.Sum & this.fieldAggrConfig.AggregationTypesFlag) != 0;
            }

            // and snapshot is is allowed to both types (metric, property)
            this.snapshotEnabled = (Aggregation.Kinds.Snapshot & this.fieldAggrConfig.AggregationTypesFlag) != 0;
            this.countEnabled = this.averageEnabled || this.varianceEnabled || (Aggregation.Kinds.Count & this.fieldAggrConfig.AggregationTypesFlag) != 0;
        }

        [JsonConstructor]
        private FieldAggregator()
        {
        }

        // aggregators
        public double Snapshot { get; private set; }

        public double Minimum { get; private set; }

        public double Maximum { get; private set; }

        public double Average
        {
            get
            {
                return this.Sum / this.Count;
            }
        }

        // variance is computed as Var(x) = \sum\limits_{i=1}^N \frac{x_i - Average)^2}{N}
        // where N is the count, x_i each value aggregated, average is the average computed.
        // to compute it without storing all values we are using:
        // Var(x) = \frac{Sum(x^2)}{N-1} - \frac{2 Average Sum(x)}{N} + \frac{N Average^2}{N-1}
        // where PartialVariance is the summation of the squares of the values and Sum(x) the summation of the same values
        public double Variance
        {
            get
            {
                double average = this.Average;
                double n = this.Count;

                if (n == 1)
                {
                    return 0.0;
                }

                return (this.partialVariance / (n - 1)) + (((n * average * average) - (2.0 * average * this.Sum)) / (n - 1));
            }
        }

        public double Sum { get; private set; }

        public double Count { get; private set; }

        public string PropertySnapshot { get; private set; }

        // Clears all aggregators by leaving them all with 0.0 value
        public void Clear()
        {
            this.PropertySnapshot = null;
            this.Snapshot = double.NegativeInfinity;
            this.partialVariance = 0.0;
            this.Minimum = double.PositiveInfinity;
            this.Maximum = double.NegativeInfinity;
            this.Sum = 0.0;
            this.Count = 0.0;
        }

        // Aggregates the provided value
        public void Aggregate(double val)
        {
            // aggregating Last seen value for field
            // it does not test if it is enabled since the cost of testing increases the work and there is no side-effect of tracking this value
            this.Snapshot = val;

            // aggregating Minimum value for field
            if (this.minimumEnabled)
            {
                if (this.Minimum > val)
                {
                    this.Minimum = val;
                }
            }

            // aggregating Minimum value for field
            if (this.maximumEnabled)
            {
                if (this.Maximum < val)
                {
                    this.Maximum = val;
                }
            }

            // Average aggregation is computed as a function of sum and count aggregations
            // variance aggregation is partially computed here, the final result
            // is computed as a function of variance and count aggregations
            if (this.varianceEnabled)
            {
                // the partial computation is basically the sum of value_i^2
                this.partialVariance += val * val;
            }

            // aggregating the Sum of value for field
            if (this.sumEnabled)
            {
                this.Sum += val;
            }

            if (this.countEnabled)
            {
                this.Count++;
            }
        }

        // Aggregates the provided value
        public void Aggregate(string val)
        {
            // aggregating Last seen value for field
            if (this.snapshotEnabled)
            {
                this.PropertySnapshot = val;
            }

            if (this.countEnabled)
            {
                this.Count++;
            }
        }

        // returns the configured aggregations and respective values
        public Dictionary<Aggregation.Kinds, double> GetMetricAggregationValues()
        {
            Dictionary<Aggregation.Kinds, double> aggregations = new Dictionary<Aggregation.Kinds, double>();

            if (this.fieldAggrConfig.Field.FieldKind == Field.Kind.Metric && (Aggregation.Kinds.Snapshot & this.fieldAggrConfig.AggregationTypesFlag) != 0)
            {
                aggregations.Add(Aggregation.Kinds.Snapshot, this.Snapshot);
            }

            if ((Aggregation.Kinds.Minimum & this.fieldAggrConfig.AggregationTypesFlag) != 0)
            {
                aggregations.Add(Aggregation.Kinds.Minimum, this.Minimum);
            }
            
            if ((Aggregation.Kinds.Maximum & this.fieldAggrConfig.AggregationTypesFlag) != 0)
            {
                aggregations.Add(Aggregation.Kinds.Maximum, this.Maximum);
            }

            if ((Aggregation.Kinds.Variance & this.fieldAggrConfig.AggregationTypesFlag) != 0)
            {
                aggregations.Add(Aggregation.Kinds.Variance, this.Variance);
            }

            if ((Aggregation.Kinds.Average & this.fieldAggrConfig.AggregationTypesFlag) != 0)
            {
                aggregations.Add(Aggregation.Kinds.Average, this.Average);
            }

            if ((Aggregation.Kinds.Sum & this.fieldAggrConfig.AggregationTypesFlag) != 0)
            {
                aggregations.Add(Aggregation.Kinds.Sum, this.Sum);
            }

            if ((Aggregation.Kinds.Count & this.fieldAggrConfig.AggregationTypesFlag) != 0)
            {
                aggregations.Add(Aggregation.Kinds.Count, this.Count);
            }

            return aggregations;
        }

        // returns the configured aggregations and respective values
        public Dictionary<Aggregation.Kinds, string> GetPropertyAggregationValues()
        {
            Dictionary<Aggregation.Kinds, string> aggregations = new Dictionary<Aggregation.Kinds, string>();

            if (this.fieldAggrConfig.Field.FieldKind == Field.Kind.Property && (Aggregation.Kinds.Snapshot & this.fieldAggrConfig.AggregationTypesFlag) != 0)
            {
                aggregations.Add(Aggregation.Kinds.Snapshot, this.PropertySnapshot);
            }

            return aggregations;
        }

        // returns the configured aggregations and respective values
        //  in a format which can be directly used by AppInsights TelemetryClient.
        public void AppendAggregationValuesFormated(Dictionary<string, string> propertiesFormatted, Dictionary<string, double> metricsFormatted)
        {
            var propertyAggregationValues = this.GetPropertyAggregationValues();

            foreach (var val in propertyAggregationValues)
            {
                propertiesFormatted.Add(string.Format("{0}_{1}", this.fieldAggrConfig.Field.Name, Aggregation.AggregationKindToName(val.Key)), val.Value);
            }

            // create the list of metric aggregations (note that the count of a property field type is a metric)
            var metricAggregationValues = this.GetMetricAggregationValues();

            foreach (var val in metricAggregationValues)
            {
                metricsFormatted.Add(string.Format("{0}_{1}", this.fieldAggrConfig.Field.Name, Aggregation.AggregationKindToName(val.Key)), val.Value);
            }
        }

        public override string ToString()
        {
            Dictionary<string, string> propertiesFormatted = new Dictionary<string, string>();
            Dictionary<string, double> metricsFormatted = new Dictionary<string, double>();
            this.AppendAggregationValuesFormated(propertiesFormatted, metricsFormatted);
            return string.Format("properties: [{0}], metrics [{1}]", propertiesFormatted.ToString() + metricsFormatted.ToString());
        }
    }
}
