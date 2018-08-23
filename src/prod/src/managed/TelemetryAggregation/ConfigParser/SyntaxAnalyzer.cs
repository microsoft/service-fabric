// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TelemetryConfigParser
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using TelemetryAggregation;

    public delegate void LogErrorDelegate(string type, string format, params object[] args);

    // this class implements the syntax parser for the following LL1 grammar:
    // L -> Comment | EOL | T
    // T -> T_t ; identifier ; identifier ; identifier ; A
    // A -> B | C
    // B -> property : D
    // C -> metric: D
    // D -> E F G
    // E -> (H)
    // F -> {H}
    // G -> , A | \epsilon
    // H -> identifier I
    // I -> , H | \epsilon
    // where \epsilon is the empty production, and identifier any string containing [a-Z0-9_.]
    public class SyntaxAnalyzer
    {
        private const string LogSourceId = "SyntaxAnalyzer";
        private StreamReader fileStream;
        private LexicalAnalyzer lexAnalyzer;
        private SemanticsAnalyzer semanticsAnalyzer;

        // private constructor
        // the only public exposed method is the static method Parse
        private SyntaxAnalyzer(StreamReader fileStream, IEnumerable<string> manifestFilesPaths, LogErrorDelegate logErrorDelegate, bool verbose)
        {
            this.fileStream = fileStream;
            this.lexAnalyzer = null;
            this.semanticsAnalyzer = new SemanticsAnalyzer(manifestFilesPaths, manifestFilesPaths.Any(), logErrorDelegate, verbose);
        }

        // Static method which returns a list with the white-listed traces and
        //  respective aggregation configurations parsed from the config file provided
        public static List<ParseResult> Parse(
            string filePath,
            IEnumerable<string> manifestFilesPaths,
            LogErrorDelegate logErrorDelegate,
            bool verbose,
            out List<TraceAggregationConfig> telemetryConfigEntryList)
        {
            telemetryConfigEntryList = new List<TraceAggregationConfig>();
            List<ParseResult> parseResultsList = new List<ParseResult>();

            try
            {
                using (StreamReader fileStream = File.OpenText(filePath))
                {
                    SyntaxAnalyzer telemetryParser = new SyntaxAnalyzer(fileStream, manifestFilesPaths, logErrorDelegate, verbose);

                    // parse line by line using the grammar and stores the parsed trace configurations
                    int currentLine = 0;
                    while (true)
                    {
                        TraceAggregationConfig traceConfigEntry;

                        currentLine++;
                        string configLine = telemetryParser.ReadNextLine(logErrorDelegate);
                        if (configLine == null)
                        {
                            // EOL
                            break;
                        }

                        telemetryParser.lexAnalyzer = new LexicalAnalyzer(configLine);

                        var parseResult = telemetryParser.L(out traceConfigEntry);
                        parseResult.LineNumber = currentLine;

                        if (parseResult.ParseCode != ParseCode.EmptyLine &&
                            parseResult.ParseCode != ParseCode.Comment)
                        {
                            // checking if something went wrong during parsing and a null object was created
                            if (traceConfigEntry == null && parseResult.ParseCode == ParseCode.Success)
                            {
                                logErrorDelegate(LogSourceId, "Unexpected null entry generated on parsing of file - {0} - ParseResult: {1}", filePath, parseResult);
                                parseResult = new ParseResult(ParseCode.ParserError, string.Format("Previous ParseResult Value: {0}", parseResult), 0, currentLine);
                            }

                            parseResultsList.Add(parseResult);

                            if (parseResult.ParseCode == ParseCode.Success)
                            {
                                telemetryConfigEntryList.Add(traceConfigEntry);
                            }
                        }
                    }
                }
            }
            catch (Exception e)
            {
                parseResultsList.Add(new ParseResult(ParseCode.ParserError, string.Format("Exception while opening or parsing config file - {0} - Exception: {1}", filePath, e)));
            }

            return parseResultsList;
        }

        // Returns the next line in the file being parsed
        private string ReadNextLine(LogErrorDelegate logErrorDelegate)
        {
            string configLine = null;
            try
            {
                configLine = this.fileStream.ReadLine();
            }
            catch (Exception e)
            {
                logErrorDelegate(LogSourceId, "Exception while reading line from config file: {0}", e.Message);
            }

            return configLine;
        }

        // This method starts the parsing of a new line in the input configuration file
        // this method implements the 
        // L -> Comment | EOL | T
        private ParseResult L(out TraceAggregationConfig telConfigEntry)
        {
            telConfigEntry = null;

            // getting next Token;
            Token token = this.lexAnalyzer.GetCurrentToken();

            // check for EmptyLine first
            if (token.Id == TokenId.EOL)
            {
                return new ParseResult(ParseCode.EmptyLine);
            }

            if (token.Id == TokenId.Comment)
            {
                return new ParseResult(ParseCode.Comment);
            }

            return this.T(out telConfigEntry);
        }

        // this method implements the T production for the grammar
        // T -> T_t ; identifier ; identifier ; identifier ; A
        private ParseResult T(out TraceAggregationConfig telConfigEntry)
        {
            telConfigEntry = null;

            // getting next Token;
            Token token = this.lexAnalyzer.GetCurrentToken();

            // T_t should be 's'
            if (token.Id != TokenId.Identifier)
            {
                return new ParseResult(ParseCode.ErrorUnexpectedToken, "Expected u|s", this.lexAnalyzer.Position);
            }

            if (token.Text != "s")
            {
                 return new ParseResult(ParseCode.NotImplemented, "Unstructured trace not implemented yet.", this.lexAnalyzer.Position);
            }

            // look for ;
            this.lexAnalyzer.ConsumeToken();
            token = this.lexAnalyzer.GetCurrentToken();

            if (token.Id != TokenId.Semicolon)
            {
                return new ParseResult(ParseCode.ErrorUnexpectedToken, "Expected ;", this.lexAnalyzer.Position);
            }

            // look for TaskName
            this.lexAnalyzer.ConsumeToken();
            token = this.lexAnalyzer.GetCurrentToken();

            if (token.Id != TokenId.Identifier)
            {
                return new ParseResult(ParseCode.ErrorUnexpectedToken, "Expected name of TaskName", this.lexAnalyzer.Position);
            }

            // save the Task name in TelemetryConfigEntry
            string telConfigEntryTaskName = token.Text;

            // look for ;
            this.lexAnalyzer.ConsumeToken();
            token = this.lexAnalyzer.GetCurrentToken();

            if (token.Id != TokenId.Semicolon)
            {
                return new ParseResult(ParseCode.ErrorUnexpectedToken, "Expected ;", this.lexAnalyzer.Position);
            }

            // look for the EventName
            this.lexAnalyzer.ConsumeToken();
            token = this.lexAnalyzer.GetCurrentToken();

            if (token.Id != TokenId.Identifier)
            {
                return new ParseResult(ParseCode.ErrorUnexpectedToken, "Expected name of EventName", this.lexAnalyzer.Position);
            }

            // save the Event name in TelemetryConfigEntry
            string telConfigEntryEventName = token.Text;

            // look for ;
            this.lexAnalyzer.ConsumeToken();
            token = this.lexAnalyzer.GetCurrentToken();

            if (token.Id != TokenId.Semicolon)
            {
                return new ParseResult(ParseCode.ErrorUnexpectedToken, "Expected ;", this.lexAnalyzer.Position);
            }

            // semantics - check whether the event is defined in the etw manifest file
            ParseResult parseRes = this.semanticsAnalyzer.CheckManifestEventDefinition(telConfigEntryTaskName, telConfigEntryEventName, this.lexAnalyzer.Position);
            if (parseRes.ParseCode != ParseCode.Success)
            {
                return parseRes;
            }

            // look for Diffentiator name which can be empty
            this.lexAnalyzer.ConsumeToken();
            token = this.lexAnalyzer.GetCurrentToken();

            if (token.Id != TokenId.Identifier && token.Id != TokenId.Semicolon)
            {
                return new ParseResult(ParseCode.ErrorUnexpectedToken, "Expected name of field to be used as differentiator", this.lexAnalyzer.Position);
            }

            // save the Task name in TelemetryConfigEntry
            string telConfigEntryDifferentiatorFieldName;

            if (token.Id == TokenId.Identifier)
            {
                // only consume token if not empty
                this.lexAnalyzer.ConsumeToken();
                telConfigEntryDifferentiatorFieldName = token.Text;
            }
            else
            {
                telConfigEntryDifferentiatorFieldName = string.Empty;
            }

            // semantics - check whether the differentiator field exists in the manifest file in case it is provided in the config file
            if (telConfigEntryDifferentiatorFieldName != string.Empty)
            {
                parseRes = this.semanticsAnalyzer.CheckManifestFieldDefinition(telConfigEntryTaskName, telConfigEntryEventName, telConfigEntryDifferentiatorFieldName, this.lexAnalyzer.Position);
                if (parseRes.ParseCode != ParseCode.Success)
                {
                    return parseRes;
                }
            }

            // construct the TraceAggregationConfig with the parsed TaskName, EventName, DifferentiatorField
            telConfigEntry = new TraceAggregationConfig(telConfigEntryTaskName, telConfigEntryEventName, telConfigEntryDifferentiatorFieldName);

            // look for Semicolon ;
            token = this.lexAnalyzer.GetCurrentToken();
            if (token.Id != TokenId.Semicolon)
            {
                return new ParseResult(ParseCode.ErrorUnexpectedToken, "Expected ;", this.lexAnalyzer.Position);
            }

            this.lexAnalyzer.ConsumeToken();

            // try to parse the aggregation information
            parseRes = this.A(ref telConfigEntry);

            if (parseRes.ParseCode != ParseCode.Success)
            {
                return parseRes;
            }

            // look for EmptyLine
            token = this.lexAnalyzer.GetCurrentToken();
            if (token.Id != TokenId.EOL)
            {
                return new ParseResult(ParseCode.ErrorUnexpectedToken, "Expected End of line", this.lexAnalyzer.Position);
            }

            this.lexAnalyzer.ConsumeToken();

            return new ParseResult(ParseCode.Success);
        }

        // this method implements the B production for the grammar
        // A -> B | C
        private ParseResult A(ref TraceAggregationConfig telConfigEntry)
        {
            // check for "string" first
            Token token = this.lexAnalyzer.GetCurrentToken();

            if (token.Id != TokenId.Identifier)
            {
                return new ParseResult(ParseCode.ErrorUnexpectedToken, "Identifier expected", this.lexAnalyzer.Position);
            }

            Field.Kind fieldKind = Field.CreateFieldKindFromName(token.Text);

            switch (fieldKind)
            {
                case Field.Kind.Property:
                    return this.B(ref telConfigEntry);
                case Field.Kind.Metric:
                    return this.C(ref telConfigEntry);
                default:
                    return new ParseResult(ParseCode.SemanticsInvalidFieldKind, "Invalid field kind specified - Expecting \"metric\" or \"property\"", this.lexAnalyzer.Position);
            }
        }

        // this method implements the B production for the grammar
        // B -> property : D
        private ParseResult B(ref TraceAggregationConfig telConfigEntry)
        {
            // check for "string" first
            Token token = this.lexAnalyzer.GetCurrentToken();

            if (token.Id != TokenId.Identifier || Field.Kind.Property != Field.CreateFieldKindFromName(token.Text))
            {
                return new ParseResult(ParseCode.SemanticsInvalidFieldKind, "Invalid field kind specified", this.lexAnalyzer.Position);
            }

            // consume the identifier (property) token
            this.lexAnalyzer.ConsumeToken();

            // check for :
            token = this.lexAnalyzer.GetCurrentToken();

            if (token.Id != TokenId.Colon)
            {
                return new ParseResult(ParseCode.ErrorUnexpectedToken, "Expected : ", this.lexAnalyzer.Position);
            }

            // consume the : token
            this.lexAnalyzer.ConsumeToken();

            return this.D(Field.Kind.Property, ref telConfigEntry);
        }

        // this method implements the C production for the grammar
        // C -> metric : D
        private ParseResult C(ref TraceAggregationConfig telConfigEntry)
        {
            // check for "metric" first
            Token token = this.lexAnalyzer.GetCurrentToken();

            if (token.Id != TokenId.Identifier || Field.Kind.Metric != Field.CreateFieldKindFromName(token.Text))
            {
                return new ParseResult(ParseCode.SemanticsInvalidFieldKind, "Invalid field kind specified", this.lexAnalyzer.Position);
            }

            // consume the identifier (metric) token
            this.lexAnalyzer.ConsumeToken();

            // check for :
            token = this.lexAnalyzer.GetCurrentToken();

            if (token.Id != TokenId.Colon)
            {
                return new ParseResult(ParseCode.ErrorUnexpectedToken, "Expected : ", this.lexAnalyzer.Position);
            }

            // consume the : token
            this.lexAnalyzer.ConsumeToken();

            return this.D(Field.Kind.Metric, ref telConfigEntry);
        }

        // this method implements the D production for the grammar
        // D -> E F G
        private ParseResult D(Field.Kind fieldKind, ref TraceAggregationConfig telConfigEntry)
        {
            // try parsing E , i.e get the param names
            List<Token> fieldTokenList = new List<Token>();
            ParseResult parseRes = this.E(fieldKind, telConfigEntry, ref fieldTokenList);

            if (parseRes.ParseCode != ParseCode.Success)
            {
                return parseRes;
            }

            // get the field names
            List<Field> fieldParamList;
            parseRes = SemanticsAnalyzer.CreateFieldsFromTokenList(fieldTokenList, fieldKind, out fieldParamList);

            if (parseRes.ParseCode != ParseCode.Success)
            {
                return parseRes;
            }

            // try parsing F , i.e get the aggregation kinds
            List<Token> aggregationParamList = new List<Token>();
            parseRes = this.F(ref aggregationParamList);

            if (parseRes.ParseCode != ParseCode.Success)
            {
                return parseRes;
            }

            // check if the aggregation kinds are valid
            List<Aggregation.Kinds> aggregationKindsList;
            parseRes = SemanticsAnalyzer.CreateAggregationKindsFromTokenList(aggregationParamList, fieldKind, out aggregationKindsList);

            if (parseRes.ParseCode != ParseCode.Success)
            {
                return parseRes;
            }

            // sucessfully parsed the aggregation entry E F
            // setting the current aggregation entry
            telConfigEntry.AddAggregationEntries(fieldParamList, aggregationKindsList);

            // try to find a new aggregation G
            parseRes = this.G(ref telConfigEntry);

            if (parseRes.ParseCode != ParseCode.Success)
            {
                return parseRes;
            }

            return new ParseResult(ParseCode.Success);
        }

        // this method implements the E production for the grammar
        // E -> ( H )
        private ParseResult E(Field.Kind fieldKind, TraceAggregationConfig telConfigEntry, ref List<Token> identifierList)
        {
            // check for ( first
            Token token = this.lexAnalyzer.GetCurrentToken();

            if (token.Id != TokenId.OpenParenthesis)
            {
                return new ParseResult(ParseCode.ErrorUnexpectedToken, "Expected (", this.lexAnalyzer.Position);
            }

            // consume the ( token
            this.lexAnalyzer.ConsumeToken();

            // try parsing H , i.e get the param names
            List<string> fieldParamList = new List<string>();
            ParseResult parseRes = this.H(fieldKind, telConfigEntry, ref identifierList);

            if (parseRes.ParseCode != ParseCode.Success)
            {
                return parseRes;
            }

            // A semantics check could be done here later against the trace manifest file

            // check for )
            token = this.lexAnalyzer.GetCurrentToken();

            if (token.Id != TokenId.CloseParenthesis)
            {
                return new ParseResult(ParseCode.ErrorUnexpectedToken, "Expected )", this.lexAnalyzer.Position);
            }

            // consume the ) token
            this.lexAnalyzer.ConsumeToken();

            return new ParseResult(ParseCode.Success);
        }

        // this method implements the F production for the grammar
        // F -> { H }
        private ParseResult F(ref List<Token> identifierList)
        {
            // check for { first
            Token token = this.lexAnalyzer.GetCurrentToken();

            if (token.Id != TokenId.OpenCurlyBraces)
            {
                return new ParseResult(ParseCode.ErrorUnexpectedToken, "Expected {", this.lexAnalyzer.Position);
            }

            // consume the { token
            this.lexAnalyzer.ConsumeToken();

            // try parsing H , i.e get the param names
            List<string> fieldParamList = new List<string>();
            ParseResult parseRes = this.J(ref identifierList);

            if (parseRes.ParseCode != ParseCode.Success)
            {
                return parseRes;
            }

            // check for }
            token = this.lexAnalyzer.GetCurrentToken();

            if (token.Id != TokenId.CloseCurlyBraces)
            {
                return new ParseResult(ParseCode.ErrorUnexpectedToken, "Expected }", this.lexAnalyzer.Position);
            }

            // consume the } token
            this.lexAnalyzer.ConsumeToken();

            return new ParseResult(ParseCode.Success);
        }

        // this method implements the G production for the grammar
        // G -> , A | \epsilon
        private ParseResult G(ref TraceAggregationConfig telConfigEntry)
        {
            // check for , first
            Token token = this.lexAnalyzer.GetCurrentToken();

            // \epsilon case
            if (token.Id != TokenId.Comma)
            {
                return new ParseResult(ParseCode.Success);
            }

            // consume the , token
            this.lexAnalyzer.ConsumeToken();

            return this.A(ref telConfigEntry);
        }

        // this method implements the H production for the grammar
        // H -> identifier I
        private ParseResult H(Field.Kind fieldKind, TraceAggregationConfig telConfigEntry, ref List<Token> identifierList)
        {
            // check for argument name first
            Token token = this.lexAnalyzer.GetCurrentToken();

            if (token.Id != TokenId.Identifier)
            {
                return new ParseResult(ParseCode.ErrorUnexpectedToken, "Expected field name", this.lexAnalyzer.Position);
            }

            // checking semantics against manifest file
            ParseResult parseRes = this.semanticsAnalyzer.CheckManifestFieldKindAndDefinition(telConfigEntry.TaskName, telConfigEntry.EventName, token.Text, fieldKind, this.lexAnalyzer.Position);

            if (parseRes.ParseCode != ParseCode.Success)
            {
                return parseRes;
            }

            // consume the argument name token
            this.lexAnalyzer.ConsumeToken();

            identifierList.Add(token);

            // try finding more aggregation kinds
            return this.I(fieldKind, telConfigEntry, ref identifierList);
        }

        // this method implements the I production for the grammar
        // I -> , H | \epsilon
        private ParseResult I(Field.Kind fieldKind, TraceAggregationConfig telConfigEntry, ref List<Token> identifierList)
        {
            // check for , first
            Token token = this.lexAnalyzer.GetCurrentToken();

            // \epsilon case
            if (token.Id != TokenId.Comma)
            {
                return new ParseResult(ParseCode.Success, string.Empty, this.lexAnalyzer.Position);
            }

            // consume the , token
            this.lexAnalyzer.ConsumeToken();

            return this.H(fieldKind, telConfigEntry, ref identifierList);
        }

        // this method implements the E production for the grammar
        // J -> identifier K
        private ParseResult J(ref List<Token> identifierList)
        {
            // check for argument name first
            Token token = this.lexAnalyzer.GetCurrentToken();

            if (token.Id != TokenId.Identifier)
            {
                return new ParseResult(ParseCode.ErrorUnexpectedToken, "Expected field name", this.lexAnalyzer.Position);
            }

            // consume the argument name token
            this.lexAnalyzer.ConsumeToken();

            identifierList.Add(token);

            // try finding more aggregation kinds
            return this.K(ref identifierList);
        }

        // this method implements the I production for the grammar
        // K -> , J | \epsilon
        private ParseResult K(ref List<Token> identifierList)
        {
            // check for , first
            Token token = this.lexAnalyzer.GetCurrentToken();

            // \epsilon case
            if (token.Id != TokenId.Comma)
            {
                return new ParseResult(ParseCode.Success, string.Empty, this.lexAnalyzer.Position);
            }

            // consume the , token
            this.lexAnalyzer.ConsumeToken();

            return this.J(ref identifierList);
        }
    }
}