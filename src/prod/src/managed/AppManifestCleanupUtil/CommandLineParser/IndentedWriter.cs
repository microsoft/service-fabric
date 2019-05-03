// ------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT License (MIT).See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace CommandLineParser
{
    using System.IO;
    using System.Text;

    /// <summary>
    /// Stream which writes at different indents.
    /// </summary>
    internal class IndentedWriter : StreamWriter
    {
        private int indent;
        private string newLine;

        /// <summary>
        /// Initializes a new instance of the <see cref="IndentedWriter"/> class.
        /// </summary>
        /// <param name="stream">The stream on which write the indented output.</param>
        public IndentedWriter(Stream stream)
            : base(stream)
        {
            this.indent = 0;
            this.newLine = GetNewLine(this.indent);
        }

        /// <summary>
        /// Gets the value of NewLine that includes the space for the current indent level.
        /// </summary>
        public override string NewLine
        {
            get
            {
                return this.newLine;
            }
        }

        /// <summary>
        /// Gets or sets the number of spaces to indent.
        /// </summary>
        public int Indent
        {
            get
            {
                return this.indent;
            }

            set
            {
                this.indent = value;
                this.newLine = GetNewLine(value);
            }
        }

        private static string GetNewLine(int indent)
        {
            var s = new StringBuilder();
            s.Append(CommandLineUtility.NewLine);
            s.Append(' ', indent);
            return s.ToString();
        }
    }
}
