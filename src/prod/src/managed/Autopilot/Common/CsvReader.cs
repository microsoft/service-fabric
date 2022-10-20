// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Autopilot
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Globalization;
    using System.IO;
    using System.Text;

    // <summary>
    // Simple .csv file format reader which implements field mapping.
    // 
    // Requirements: 
    // Requires the following lines at the beginning of the stream for parsing the fields
    // #Version: 1.0
    // #Fields: Field1,Field2,....
    //
    // Comments:
    // It uses ; and # characters as comment characters 
    // 
    // Pattern:
    // It follows a pattern similar to the System.Data.DataReader
    // 
    // using(CsvReader csvReader = new CsvReader(@"D:\app\machinelist.csv"))
    // {
    //     while (csvReader.Read())
    //     {
    //         Console.WriteLine(csvReader["MachineName"], csvReader["ScaleUnit"]);
    //     }
    // }
    // </summary>

    public class CsvReader : IDisposable
    {
        private readonly TextReader _reader;
        private readonly bool _acceptQuotedFields = true;

        private IDictionary<string, int> _columnMapping;
        private string[] _currentValues;
        private IDictionary<string, string> _headerAttributes;
        private string _currentLine;
        private int _columnCount;
        private int _lineNumber = 0;

        /// <summary>
        /// Initializes a new instance of the CsvReader class using a file path.
        /// </summary>        
        public CsvReader(string path)
            : this(path, true)
        {
        }

        /// <summary>
        /// Initializes a new instance of the CsvReader class using a TextReader
        /// </summary>        
        public CsvReader(TextReader reader)
            : this(reader, true)
        {
        }

        /// <summary>
        /// Initializes a new instance of the CsvReader class using a TextReader
        /// </summary>        
        public CsvReader(TextReader reader, bool acceptQuoted)
        {
            _reader = reader;
            _columnMapping = null;
            _acceptQuotedFields = acceptQuoted;
            try
            {
                InitializeFieldMapping();
                InitializeHeaderAttributes();
                reader = null;
            }
            finally
            {
                if (reader != null)
                {
                    reader.Dispose();
                }
            }
        }

        /// <summary>
        /// IInitializes a new instance of the CsvReader class using a path.
        /// </summary>        
        public CsvReader(string path, bool acceptQuoted)
            : this(new StreamReader(new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read)), acceptQuoted)
        {
            // We create a StreamReader in this constructor and calling another constructor to work with this StreamReader.
            // We'll dispose the reader in Dispose() method of the class since CsvReader has public classes that use the reader.
        }

        /// <summary>
        /// Access a column of the CsvReader by name. 
        /// Returns value, or throws if column name does not exist.
        /// </summary>        
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Globalization", "CA1308:NormalizeStringsToUppercase", Justification = "Automatic suppression")]
        public string this[string columnName]
        {
            get
            {
                if (_currentValues == null)
                {
                    throw new InvalidOperationException("Reader not positioned. Call Read() prior to using.");
                }

                if (!_columnMapping.ContainsKey(columnName.ToLowerInvariant()))
                {
                    throw new InvalidOperationException(String.Format(CultureInfo.InvariantCulture, "Unrecognized column {0}. Available columns: {1}", columnName, GetFieldsString()));
                }

                return _currentValues[_columnMapping[columnName.ToLowerInvariant()]].Trim();
            }
        }

        /// <summary>
        /// Return the value of a header attribute
        /// </summary>
        /// <param name="attributeName">
        /// The attribute name
        /// </param>
        /// <returns>
        /// NULL if the attribute is not specific, otherwise the attribute
        /// </returns>
        public string GetHeaderAttribute(string attributeName)
        {
            if (_headerAttributes == null || !_headerAttributes.ContainsKey(attributeName))
            {
                return null;
            }
            else
            {
                return _headerAttributes[attributeName];
            }
        }

        private string AdvanceLine()
        {
            string line = null;
            line = _reader.ReadLine();

            if (line != null)
            {
                _lineNumber++;
            }

            return line;
        }

        private string GetFieldsString()
        {
            StringBuilder fieldsString = new StringBuilder();

            foreach (KeyValuePair<string, int> key in _columnMapping)
            {
                fieldsString.Append(key.Key);
                fieldsString.Append(", ");
            }

            return fieldsString.ToString();
        }

        /// <summary>
        /// Parse the header fields following headers, format like:
        /// <para></para> 
        /// #Version: 1.0 <p/>
        /// #Fields: Field1, Field2, Field3 <p/>
        /// #HeaderField1: Value1 <p/>
        /// #HeaderField2: Value2 <p/>
        /// </summary>
        private void InitializeHeaderAttributes()
        {
            string line;
            string originalLine;
            do
            {
                originalLine = AdvanceLine();
                line = originalLine;
                if (line == null || !line.StartsWith("#", StringComparison.OrdinalIgnoreCase))
                {
                    break;
                }

                line = line.Substring(1).Trim();

                int colonPosition = line.IndexOf(':');

                // if : does not exist or : is the last character, then it is an invalid header
                if (colonPosition == -1 || line.IndexOf(':') >= line.Length - 1)
                {
                    break;
                }

                if (_headerAttributes == null)
                {
                    _headerAttributes = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
                }

                _headerAttributes[line.Substring(0, colonPosition).Trim()] = line.Substring(colonPosition + 1, line.Length - (colonPosition + 1)).Trim();

            } while (true); //Read past comments

            // We have read one more line and determine that it's not header fields
            // This line could be real data, so we store it
            if (line != null)
            {
                _currentLine = originalLine;
            }
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Globalization", "CA1308:NormalizeStringsToUppercase", Justification = "Automatic suppression")]
        private void InitializeFieldMapping()
        {
            _columnMapping = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
            string line;

            do
            {
                line = AdvanceLine();
                if (line == null)
                {
                    break;
                }
            }
            while (IsEmpty(line)); // Read past comments

            if (line == null ||
                !(line.StartsWith("#Version:1.0", StringComparison.OrdinalIgnoreCase) ||
                  line.StartsWith("#Version: 1.0", StringComparison.OrdinalIgnoreCase) ||
                  line.StartsWith("#Version: 1.00", StringComparison.OrdinalIgnoreCase) ||
                  line.StartsWith("#Version:1.00", StringComparison.OrdinalIgnoreCase) ||
                  line.StartsWith("#Version 1.00", StringComparison.OrdinalIgnoreCase)))
            {
                throw new InvalidOperationException(
                    String.Format(CultureInfo.InvariantCulture, "Unrecognized format. Expecting #Version:1.0  Actual: {0} Line:{1}", line ?? "<null>", _lineNumber));
            }

            do
            {
                line = AdvanceLine();
                if (line == null)
                {
                    break;
                }
            }
            while (IsEmpty(line)); // Read past comments

            if (line == null || !line.StartsWith("#Fields:", StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidOperationException(
                    String.Format(CultureInfo.InvariantCulture, "Unrecognized format. Expecting #Fields:  Actual: {0} Line:{1}", line ?? "<null>", _lineNumber));
            }

            string[] header = line.Split(new[] { ':' }, 2);
            string[] fields = (header.Length > 1) ? header[1].Split(new[] { ',' }) : new string[] { };

            for (int i = 0; i < fields.Length; i++)
            {
                _columnMapping[fields[i].Trim().ToLowerInvariant()] = i;
                _columnCount++;
            }
        }

        private static bool IsEmpty(string line)
        {
            foreach (char t in line)
            {
                // If we are still here, we havent seen any non-whitespace yet
                if (Char.IsWhiteSpace(t))
                {
                    continue;
                }
                else
                {
                    return false;
                }
            }

            // Line is entirely whitespace because we never found a comment character
            return true;
        }

        private static bool IsCommentOrEmpty(string line)
        {
            foreach (char t in line)
            {
                // If we are still here, we havent seen any non-whitespace yet
                if (Char.IsWhiteSpace(t))
                {
                    continue;
                }

                if (t == ';' || t == '#')
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }

            // Line is entirely whitespace because we never found a comment character
            return true;
        }

        /// <summary>
        /// Split string by commas, ingoring the ones between two matched "s
        /// </summary>
        private string[] SplitString(string source)
        {
            if (source == null)
            {
                throw new ArgumentException("source is null", "source");
            }

            string[] returnValue;

            if (TrySplitString(source, out returnValue))
            {
                return returnValue;
            }
            else
            {
                throw new ArgumentException("One '\"' doesn't have a matched one. Line:" + _lineNumber, "source");
            }
        }

        /// <summary>
        /// Split string by commas, ingoring the ones between two matched "s
        /// </summary>
        private static bool TrySplitString(string source, out string[] returnValue)
        {
            if (source == null)
            {
                returnValue = null;
                return false;
            }

            bool ifInQuote = false;
            List<string> returnList = new List<string>();
            int closed, open;
            for (closed = 0, open = 0; open < source.Length; open++)
            {
                if (source[open] == '"')
                {
                    ifInQuote = !ifInQuote;
                }
                else if (source[open] == ',' && !ifInQuote)
                {
                    returnList.Add(source.Substring(closed, open - closed));
                    closed = open + 1;
                }
            }

            if (ifInQuote)
            {
                returnValue = null;
                return false;
            }

            if (closed == source.Length)
            {
                returnList.Add(string.Empty);
            }
            else
            {
                returnList.Add(source.Substring(closed, source.Length - closed));
            }

            returnValue = returnList.ToArray();
            return true;
        }

        /// <summary>
        /// Advances the reader to the next line
        /// </summary>
        /// <param name="numLineSkipped">
        /// How many lines skipped
        /// </param>
        /// <returns>
        /// True if the reader was able to read a valid line.
        /// </returns>
        public bool TryRead(out int numLineSkipped)
        {
            string currentLine;
            numLineSkipped = 0;
            do
            {
                if (_currentLine != null)
                {
                    currentLine = _currentLine;
                    _currentLine = null;
                }
                else
                {
                    currentLine = AdvanceLine();
                }

                if (currentLine == null)
                {
                    return false;
                }

                if (!IsCommentOrEmpty(currentLine))
                {
                    if (_acceptQuotedFields)
                    {
                        if (!TrySplitString(currentLine, out _currentValues) || _currentValues.Length != _columnCount)
                        {
                            numLineSkipped++;
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        _currentValues = currentLine.Split(',');
                        if (_currentValues == null || _currentValues.Length != _columnCount)
                        {
                            numLineSkipped++;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
            while (true);
            return true;
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Globalization", "CA1308:NormalizeStringsToUppercase", Justification = "Automatic suppression")]
        public bool ContainsColumn(string column)
        {
            return _columnMapping.ContainsKey(column.ToLowerInvariant());
        }

        /// <summary>
        /// Advances the reader to the next line
        /// </summary>
        /// <returns>
        /// True if the reader was able to read a valid line.
        /// </returns>
        public bool Read()
        {
            string currentLine;
            do
            {
                if (_currentLine != null)
                {
                    currentLine = _currentLine;
                    _currentLine = null;
                }
                else
                {
                    currentLine = AdvanceLine();
                }

                if (currentLine == null)
                {
                    return false;
                }
            }
            while (IsCommentOrEmpty(currentLine)); // Read past comments

            if (_acceptQuotedFields)
            {
                _currentValues = SplitString(currentLine);
            }
            else
            {
                _currentValues = currentLine.Split(',');
            }

            if (_currentValues.Length != _columnCount)
            {
                throw new InvalidOperationException("Field count does not match values in current row. Line:" + _lineNumber);
            }

            return true;
        }

        /// <summary>
        /// Get a list of columns
        /// </summary>
        /// <returns>Columns</returns>
        [SuppressMessage("Microsoft.Design", "CA1024:UsePropertiesWhereAppropriate")]
        public IEnumerable<string> GetColumns()
        {
            return _columnMapping.Keys;
        }

        /// <summary>
        /// Disposes the reader releasing any file resources. 
        /// </summary>
        protected virtual void Dispose(bool isDisposing)
        {
            if (isDisposing)
            {
                // Clean managed resources
                if (_reader != null)
                {
                    _reader.Dispose();
                }
            }

            // Clean native resources.
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
    }
}