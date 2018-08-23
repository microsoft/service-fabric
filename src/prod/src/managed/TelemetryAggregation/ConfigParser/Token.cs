// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TelemetryConfigParser
{
    // Types of token returned by the lexical analyzer
    public enum TokenId
    {
        Identifier,
        Comma,
        Colon,
        Semicolon,
        OpenParenthesis,
        CloseParenthesis,
        OpenCurlyBraces,
        CloseCurlyBraces,
        Error,
        EOL,
        Comment
    }

    // This class allows the lexical analyzer to provide the syntax analyzer with extra information
    // for instance the name of the identifier token.
    public class Token
    {
        public string Text { get; set; }

        public TokenId Id { get; set; }

        public string ErrorMsg { get; set; }

        public int ColumnPos { get; set; }
    }
}