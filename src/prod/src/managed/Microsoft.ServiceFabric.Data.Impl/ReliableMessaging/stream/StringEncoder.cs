// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    #region using directives

    using System.IO;
    using System.Text;

    #endregion

    /// <summary>
    /// String Encoder and Decoder functionality
    /// </summary>
    internal class StringEncoder
    {
        #region fields

        private static readonly UnicodeEncoding StaticEncoder = new UnicodeEncoding();
        private readonly UnicodeEncoding encoder;

        /// <summary>
        /// String to encode and its byte length
        /// </summary>
        private string content;

        #endregion

        #region ctor

        /// <summary>
        /// Constructor. Instantiates the object.
        /// Note: Since content string is null, this object is typically used for decoding content.
        /// </summary>
        internal StringEncoder()
        {
            this.encoder = new UnicodeEncoding();
            this.ContentByteLength = 0;
            this.content = null;
        }

        /// <summary>
        /// Constructor. Instantiates the object
        /// </summary>
        /// <param name="content">String to encode</param>
        internal StringEncoder(string content)
        {
            this.encoder = new UnicodeEncoding();
            this.content = content;

            // number of string bytes
            this.ContentByteLength = string.IsNullOrEmpty(content) ? 0 : this.encoder.GetByteCount(content);
        }

        #endregion

        #region properties

        /// <summary>
        /// Gets the targetted string to be encoded.
        /// </summary>
        internal string Content
        {
            get
            {
                if (string.IsNullOrEmpty(this.content))
                {
                    return string.Empty;
                }

                return this.content;
            }
        }

        /// <summary>
        /// Gets the targetted string byte length
        /// </summary>
        internal int ContentByteLength { get; private set; }

        /// <summary>
        /// Gets the targeted string encoding byte count
        /// </summary>
        internal int EncodingByteCount
        {
            get { return this.ContentByteLength + sizeof(int); }
        }

        /// <summary>
        ///  Gets the given string encoding byte count
        /// </summary>
        /// <param name="contentString">Given content string</param>
        /// <returns>String encoded byte count</returns>
        internal int GetEncodingByteCount(string contentString)
        {
            // Integer length + number of string bytes
            return sizeof(int) + (string.IsNullOrEmpty(contentString) ? 0 : this.encoder.GetByteCount(contentString));
        }

        #endregion

        #region methods

        /// <summary>
        /// Encode and write the object content string to the given memory stream
        /// </summary>
        /// <param name="writer">BinaryWriter to serialize to.</param>
        internal void Encode(BinaryWriter writer)
        {
            writer.Write(this.content);
        }

        /// <summary>
        /// Decode the string from the source byte array
        /// </summary>
        /// <param name="reader">BinaryReader to deserialize from</param>
        /// <returns>Decoded string</returns>
        internal string Decode(BinaryReader reader)
        {
            this.content = reader.ReadString();
            return this.content;
        }

        #endregion
    }
}