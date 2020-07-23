// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.WindowsAzure.Security.Authentication;
using System.IdentityModel.Tokens;
using System.Fabric.Strings;

namespace System.Fabric.dSTSClient
{
    /// <summary>
    /// Implements the <see cref="ISecurityTokenSerializer"/> to serialize
    /// the <see cref="GenericXmlSecurityToken"/>.
    /// </summary>
    /// <remarks>
    /// This class is used by the client to serialize a security token and put it 
    /// in the authorization header.
    /// </remarks>
    internal class GenericXmlSecurityTokenSerializer : ISecurityTokenSerializer
    {
        /// <summary>
        /// Determines whether the specified security token can be serialized.
        /// </summary>
        /// <param name="token">The security token to serialize.</param>
        /// <returns>true if the token can be serialized; otherwise, false.</returns>
        public bool CanSerialize(SecurityToken token)
        {
            if (token == null)
            {
                return false;
            }

            if (!(token is GenericXmlSecurityToken))
            {
                return false;
            }

            GenericXmlSecurityToken xmlToken = (GenericXmlSecurityToken)token;
            if (xmlToken.TokenXml == null || xmlToken.TokenXml.OuterXml == null)
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Serializes a security token to a string.
        /// </summary>
        /// <param name="token">The security token to serialize.</param>
        /// <returns>A string representing the serialized token.</returns>
        public string Serialize(SecurityToken token)
        {
            if (token == null)
            {
                throw new ArgumentNullException();
            }

            if (CanSerialize(token))
            {
                GenericXmlSecurityToken xmlToken = (GenericXmlSecurityToken)token;
                return xmlToken.TokenXml.OuterXml;
            }
            else
            {
                throw new NotSupportedException(StringResources.Error_SecurityTokenSerializationFailure);
            }
        }

        /// <summary>
        /// Determines whether the specified string that represents a serialized token
        /// can be deserialized into a token.
        /// </summary>
        /// <param name="serializedToken">The string representing a serialized token.</param>
        /// <returns>true if the string can be deserialized into a token; otherwise, false.</returns>
        public bool CanDeserialize(string serializedToken)
        {
            //
            // Not implemented because the client does not need to deserialize a string 
            // to a generic XML token.
            //
            return false;
        }

        /// <summary>
        /// Deserialize a string into a security token.
        /// </summary>
        /// <param name="serializedToken">The string representing a serialized token.</param>
        /// <returns>The security token.</returns>
        public SecurityToken Deserialize(string serializedToken)
        {
            //
            // Not implemented because the client does not need to deserialize a string 
            // to a generic XML token.
            //
            throw new NotImplementedException();
        }
    }
}