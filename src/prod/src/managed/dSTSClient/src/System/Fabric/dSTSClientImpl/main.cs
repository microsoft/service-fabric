// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Reflection;
using System.Diagnostics;
using System.IdentityModel.Tokens;
using System.Xml;
using System.IO;

namespace System.Fabric.dSTSClient 
{
    class Program
    {
        private static readonly string MetadataEndpoint = "/MetadataEndpoint";
        private static readonly string ServerCommonName = "/ServerCommonName";
        private static readonly string ServerThumbprint = "/ServerThumbprint";

        static void Main(string[] args)
        {
            if (args.Length != 2)
            {
                ArgumentHelper();
                return;
            }

            string endpoint;
            string commonName = null;
            string thumbprint = null;

            if (!ParseArguments(args, out endpoint, out commonName, out thumbprint))
            {
                ArgumentHelper();
                return;
            }

            string[] commonNames = null;
            string[] thumbprints = null;
            if (!String.IsNullOrEmpty(commonName))
            {
                commonNames = new string[] {commonName};
            }
            else if (!String.IsNullOrEmpty(thumbprint))
            {
                thumbprints = new string[] {thumbprint};
            }
            else
            {
                ArgumentHelper();
                return;
            }

            dSTSClientImpl client = dSTSClientImpl.CreatedSTSClient(endpoint, commonNames, thumbprints, true);
            if (client == null)
            {
                Console.WriteLine("Client creation failed");
                return;
            }

            GenericXmlSecurityToken token;
            try
            {
                token = client.GetSecurityTokenInternal() as GenericXmlSecurityToken;
            }
            catch (Exception e)
            {
                Console.WriteLine("Exception: {0}", e.Message);
                return;
            }

            if (token == null)
            {
                Console.WriteLine("Token - invalid format");
                return;
            }

            DumpToken(token.TokenXml.OuterXml);
        }

        private static void ArgumentHelper()
        {
            Console.WriteLine("dSTSClient.exe /MetadataEndpoint:<Endpoint> /ServerCommonName:<CommonName>");
        }

        private static bool ParseArguments(string[] args, out string endpoint, out string commonName, out string thumbprint)
        {
            endpoint = "";
            commonName = "";
            thumbprint = "";

            foreach (string arg in args)
            {
                int delimiterIndex = arg.IndexOf(":", StringComparison.OrdinalIgnoreCase);
                if (delimiterIndex == -1)
                {
                    Console.WriteLine("Argument {0} is invalid", arg);
                    return false;
                }

                string key = arg.Substring(0, delimiterIndex);
                string value = arg.Substring(delimiterIndex + 1);

                if (key == MetadataEndpoint)
                {
                    endpoint = value;
                }
                else if (key == ServerCommonName)
                {
                    commonName = value;
                }
                else if (key == ServerThumbprint)
                {
                    thumbprint = value;
                }
                else
                {
                    Console.WriteLine("Argument {0} is invalid", arg);
                    return false;
                }
            }

            return true;
        }

        private static void DumpToken(string tokenXml)
        {
            XmlReaderSettings settings = new XmlReaderSettings()
            {
                XmlResolver = null,
                DtdProcessing = DtdProcessing.Prohibit
            };

            XmlReader reader = XmlReader.Create(new StringReader(tokenXml), settings);
            XmlDocument doc = new XmlDocument();
            doc.XmlResolver = null;
            doc.Load(reader);
            StringBuilder formattedString = new StringBuilder();
            StringWriter writer = new StringWriter(formattedString);

            XmlTextWriter xmlWriter = new XmlTextWriter(writer);
            xmlWriter.Formatting = Formatting.Indented;
            doc.WriteTo(xmlWriter);

            Console.WriteLine("Token is :");
            Console.WriteLine("{0}", formattedString.ToString());
        }
    }
}
