// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TelemetryConfigParser
{
    public enum ParseCode
    {
        EmptyLine,                // blank line
        Success,            // able to parse a single config from the line
        ErrorUnexpectedToken,   // a different token is expected by the parser
        SemanticsInvalidAggregationType,    // the syntax is correct but the identifier provided does not map to an aggregation type
        SemanticsInvalidFieldKind,  // expecting "number" or "string" but got something else
        SemanticsUndefinedEventInManifest, // the trace event (taskname, eventname) was not defined in the manifest files provided
        SemanticsUndefinedFieldInEvent, // the field used for aggregation is no defined in the manifest
        SemanticsFieldKindDoesNotMatch, // the field type definined in the configuration file being parsed does not match the type defined in the etw events manifest
        NotImplemented,  // unstructured trace support not yet implemented
        ParserError, // unexpected error in parsing
        Comment // line comment
    }

    public class ParseResult
    {
        public ParseResult(ParseCode parseCode, string errorMsg = "", int columnPos = 0, int lineNumber = 0)
        {
            this.ParseCode = parseCode;
            this.ErrorMsg = errorMsg;
            this.ColumnPos = columnPos;
            this.LineNumber = lineNumber;
        }

        // Error/Success code
        public ParseCode ParseCode { get; private set; }

        // Error message if any
        public string ErrorMsg { get; private set; }

        // Position of the lexical analyzer in the line being parsed
        public int ColumnPos { get; private set; }

        public int LineNumber { get; set; }
    }
}