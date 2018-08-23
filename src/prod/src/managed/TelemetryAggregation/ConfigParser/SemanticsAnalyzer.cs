// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TelemetryConfigParser
{
    using System.Collections.Generic;
    using TelemetryAggregation;
    using Tools.EtlReader;

    public class SemanticsAnalyzer
    {
        private readonly ManifestEventFieldsLookup manifestLookup;
        private readonly bool manifestSemanticsCheckEnabled;

        public SemanticsAnalyzer(IEnumerable<string> manifestFilePaths, bool enableManifestSemanticsCheck, LogErrorDelegate logErrorDelegate, bool verbose)
        {
            this.manifestSemanticsCheckEnabled = enableManifestSemanticsCheck;
            this.manifestLookup = new ManifestEventFieldsLookup(manifestFilePaths, logErrorDelegate, verbose);
        }

        // Tries to translate the string describing into an Aggregation.Type
        // if there is not corresponding aggregator return false to signal the semantic failure
        public static bool CreateAggregationTypeFromName(string aggregationName, ref Aggregation.Kinds aggregationKind)
        {
            aggregationKind = Aggregation.CreateAggregationKindFromName(aggregationName);

            if (aggregationKind == Aggregation.Kinds.Undefined)
            {
                return false;
            }

            return true;
        }

        public static ParseResult CreateAggregationKindsFromTokenList(IEnumerable<Token> tokenList, Field.Kind fieldKind, out List<Aggregation.Kinds> aggregationTypes)
        {
            aggregationTypes = new List<Aggregation.Kinds>();

            foreach (var token in tokenList)
            {
                Aggregation.Kinds aggrType = Aggregation.Kinds.Undefined;

                // check whether the aggregation type name exists
                if (!SemanticsAnalyzer.CreateAggregationTypeFromName(token.Text, ref aggrType))
                {
                    return new ParseResult(ParseCode.SemanticsInvalidAggregationType, "Invalid aggregation type", token.ColumnPos);
                }

                // check whether the aggregation type is supported by the field type
                if (!Field.IsAggregationAvailable(fieldKind, aggrType))
                {
                    return new ParseResult(ParseCode.SemanticsInvalidAggregationType, "Invalid aggregation type for the requested field type", token.ColumnPos);
                }

                aggregationTypes.Add(aggrType);
            }

            return new ParseResult(ParseCode.Success);
        }

        public static ParseResult CreateFieldsFromTokenList(IEnumerable<Token> tokenList, Field.Kind fieldKind, out List<Field> paramList)
        {
            paramList = new List<Field>();

            foreach (var token in tokenList)
            {
                paramList.Add(new Field(token.Text, fieldKind));
            }

            return new ParseResult(ParseCode.Success);
        }

        public ParseResult CheckManifestEventDefinition(string taskName, string eventName, int columnPos)
        {
            // by-passing check in case semantic validadation against manifest files is disabled
            if (!this.manifestSemanticsCheckEnabled)
            {
                return new ParseResult(ParseCode.Success);
            }

            Dictionary<string, FieldDefinition> eventFields;

            if (!this.manifestLookup.TryGetEvent(taskName, eventName, out eventFields))
            {
                return new ParseResult(
                    ParseCode.SemanticsUndefinedEventInManifest,
                    string.Format("The specified pair (TaskName: {0}, EventName: {1}) is not available in manifest files", taskName, eventName),
                    columnPos);
            }

            return new ParseResult(ParseCode.Success);
        }

        public ParseResult CheckManifestFieldDefinition(string taskName, string eventName, string fieldName, int columnPos)
        {
            // by-passing check in case semantic validadation against manifest files is disabled
            if (!this.manifestSemanticsCheckEnabled)
            {
                return new ParseResult(ParseCode.Success);
            }

            FieldDefinition fieldDef;

            if (!this.manifestLookup.TryGetField(taskName, eventName, fieldName, out fieldDef))
            {
                return new ParseResult(
                    ParseCode.SemanticsUndefinedEventInManifest,
                    string.Format("The specified field ({0}) is not available for the event defined by the pair (TaskName: {1}, EventName: {2})", fieldName, taskName, eventName),
                    columnPos);
            }

            return new ParseResult(ParseCode.Success);
        }

        public ParseResult CheckManifestFieldKindAndDefinition(string taskName, string eventName, string fieldName, Field.Kind expectedFieldKind, int columnPos)
        {
            // by-passing check in case semantic validadation against manifest files is disabled
            if (!this.manifestSemanticsCheckEnabled)
            {
                return new ParseResult(ParseCode.Success);
            }

            FieldDefinition fieldDef;

            if (!this.manifestLookup.TryGetField(taskName, eventName, fieldName, out fieldDef))
            {
                return new ParseResult(
                    ParseCode.SemanticsUndefinedEventInManifest,
                    string.Format("The specified field ({0}) is not available for the event defined by the pair (TaskName: {1}, EventName: {2})", fieldName, taskName, eventName),
                    columnPos);
            }

            // check whether field matches the expected type
            Field.Kind fieldKind = SemanticsAnalyzer.GetAggregationFieldKind(fieldDef);
            if (fieldKind != expectedFieldKind)
            {
                return new ParseResult(
                    ParseCode.SemanticsInvalidFieldKind,
                    string.Format("The specified field ({0}) is of type ({1}) but was defined as a ({2}) type", fieldName, fieldKind.ToString(), expectedFieldKind.ToString()),
                    columnPos);
            }

            return new ParseResult(ParseCode.Success);
        }

        private static Field.Kind GetAggregationFieldKind(FieldDefinition fieldDef)
        {
            switch (fieldDef.Type)
            {
                case FieldDefinition.FieldType.UInt16:
                case FieldDefinition.FieldType.UInt32:
                case FieldDefinition.FieldType.UInt64:
                case FieldDefinition.FieldType.Int8:
                case FieldDefinition.FieldType.Int16:
                case FieldDefinition.FieldType.Int32:
                case FieldDefinition.FieldType.Int64:
                case FieldDefinition.FieldType.HexInt32:
                case FieldDefinition.FieldType.HexInt64:
                case FieldDefinition.FieldType.Float:
                case FieldDefinition.FieldType.Double:
                    return Field.Kind.Metric;
                case FieldDefinition.FieldType.UnicodeString:
                case FieldDefinition.FieldType.AnsiString:
                case FieldDefinition.FieldType.Boolean:
                case FieldDefinition.FieldType.UInt8:
                case FieldDefinition.FieldType.DateTime:
                case FieldDefinition.FieldType.Guid:
                    return Field.Kind.Property;
                default:
                    return Field.Kind.Undefined;
            }
        }
    }
}