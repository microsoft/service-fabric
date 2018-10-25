// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TelemetryConfigParser
{
    using System;
    using System.Text;

    // This class provide means of extracting tokens from strings
    // It implements a finite state machine
    public class LexicalAnalyzer
    {
        // string being broken into tokens
        private string line;

        public LexicalAnalyzer(string line)
        {
            this.line = line ?? string.Empty;
            this.Position = 0;
        }

        // Types of characters used by the
        //  finite-state machine to extract tokens.
        private enum CharType
        {
            Letter,
            Digit,
            Whitespace,
            Comma,
            Colon,
            Semicolon,
            OpenParenthesis,
            CloseParenthesis,
            OpenCurlyBraces,
            CloseCurlyBraces,
            Unsupported,
            EOL,
            Pound
        }

        // Current character Position at the line being parsed
        public int Position { get; private set; }

        // Gets the current token without consuming the token.
        // consecutive calls return the same token
        public Token GetCurrentToken()
        {
            int posTmp = this.Position;
            Token token = this.GetNextToken();
            this.Position = posTmp;
            return token;
        }

        // Returns the current token and moves the Position to the end of it.
        // It consumes the token, i.e., consecutive calls return consecutive tokens until end of line
        public Token ConsumeToken()
        {
            return this.GetNextToken();
        }

        // parses the string and returns the token
        private Token GetNextToken()
        {
            int posTmp = this.Position;
            Token token = this.StartParseState();

            // just setting the error column position in text to help find the error
            token.ColumnPos = posTmp;

            return token;
        }

        // initial state of the state-machine to extract tokens
        private Token StartParseState()
        {
            char c;
            CharType charType = this.CurrentChar(out c);

            while (charType != CharType.EOL)
            {
                switch (charType)
                {
                    case CharType.Whitespace:
                        // consumes white spaces
                        this.ConsumeCurrentChar();
                        break;
                    case CharType.Digit:
                        return new Token { Id = TokenId.Error, ErrorMsg = string.Format("Digit {0} not expected", c), Text = c.ToString() };
                    case CharType.Letter:
                        return this.ParseIdentifier();
                    case CharType.OpenParenthesis:
                        this.ConsumeCurrentChar();
                        return new Token { Id = TokenId.OpenParenthesis, ErrorMsg = string.Empty, Text = c.ToString() };
                    case CharType.CloseParenthesis:
                        this.ConsumeCurrentChar();
                        return new Token { Id = TokenId.CloseParenthesis, ErrorMsg = string.Empty, Text = c.ToString() };
                    case CharType.OpenCurlyBraces:
                        this.ConsumeCurrentChar();
                        return new Token { Id = TokenId.OpenCurlyBraces, ErrorMsg = string.Empty, Text = c.ToString() };
                    case CharType.CloseCurlyBraces:
                        this.ConsumeCurrentChar();
                        return new Token { Id = TokenId.CloseCurlyBraces, ErrorMsg = string.Empty, Text = c.ToString() };
                    case CharType.Comma:
                        this.ConsumeCurrentChar();
                        return new Token { Id = TokenId.Comma, ErrorMsg = string.Empty, Text = c.ToString() };
                    case CharType.Colon:
                        this.ConsumeCurrentChar();
                        return new Token { Id = TokenId.Colon, ErrorMsg = string.Empty, Text = c.ToString() };
                    case CharType.Semicolon:
                        this.ConsumeCurrentChar();
                        return new Token { Id = TokenId.Semicolon, ErrorMsg = string.Empty, Text = c.ToString() };
                    case CharType.Pound:
                        this.ConsumeCurrentChar();
                        return new Token { Id = TokenId.Comment, ErrorMsg = string.Empty, Text = c.ToString() };
                    default:
                        this.ConsumeCurrentChar();
                        return new Token { Id = TokenId.Error, ErrorMsg = string.Format("Character '{0}' not expected", c), Text = c.ToString() };
                }

                // only reaches here for white-space
                charType = this.CurrentChar(out c);
            }

            return new Token { Id = TokenId.EOL, Text = string.Empty, ErrorMsg = string.Empty };
        }

        // state of the state-machine to extract tokens which represent identifiers
        private Token ParseIdentifier()
        {
            char c;

            // lookahead
            CharType charType = this.CurrentChar(out c);

            // expects a letter or digit
            if (!(charType == CharType.Digit || charType == CharType.Letter))
            {
                return new Token { Id = TokenId.Error, ErrorMsg = "Expected digit, letter, or _", Text = string.Empty };
            }

            bool parsedName = false;
            StringBuilder sb = new StringBuilder();

            while (!parsedName)
            {
                switch (charType)
                {
                    case CharType.Letter:
                        sb.Append(c);
                        this.ConsumeCurrentChar();
                        break;
                    case CharType.Digit:
                        sb.Append(c);
                        this.ConsumeCurrentChar();
                        break;
                    default:
                        parsedName = true;
                        break;
                }

                // lookahead
                charType = this.CurrentChar(out c);
            }

            return new Token { Id = TokenId.Identifier, ErrorMsg = string.Empty, Text = sb.ToString() };
        }

        // consumes characters, i.e., change the index position in the string
        private void ConsumeCurrentChar()
        {
            // check whether current is EmptyLine
            char c;
            CharType charType = this.CurrentChar(out c);

            if (charType != CharType.EOL)
            {
                this.Position++;
            }
        }

        // Gets current character withour consuming it
        // i.e., consecutive calls return the same characters
        // does not change index position in the string
        private CharType CurrentChar(out char c)
        {
            // return EmptyLine if it is past the line size
            if (this.Position >= this.line.Length)
            {
                c = '\0';
                return CharType.EOL;
            }

            c = this.line[this.Position];

            if (char.IsLetter(c) || c == '_' || c == '.')
            {
                return CharType.Letter;
            }

            if (char.IsDigit(c))
            {
                return CharType.Digit;
            }

            if (char.IsWhiteSpace(c))
            {
                return CharType.Whitespace;
            }

            switch (c)
            {
                case '(': return CharType.OpenParenthesis;
                case ')': return CharType.CloseParenthesis;
                case '{': return CharType.OpenCurlyBraces;
                case '}': return CharType.CloseCurlyBraces;
                case ',': return CharType.Comma;
                case ':': return CharType.Colon;
                case ';': return CharType.Semicolon;
                case '#': return CharType.Pound;
                default: return CharType.Unsupported;
            }
        }
    }
}