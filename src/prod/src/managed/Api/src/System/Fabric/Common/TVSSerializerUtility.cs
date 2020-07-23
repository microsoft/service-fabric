// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.IO;
using System.Xml;
using System.ServiceModel.Channels;

namespace System.Fabric.Common
{
    internal class TVSSerializerUtility
    {
        public static Message Deserialize(string obj)
        {
            byte[] buffer = Convert.FromBase64String(obj);
            MemoryStream stream = new MemoryStream(buffer);
            XmlDictionaryReader reader = XmlDictionaryReader.CreateTextReader(stream, XmlDictionaryReaderQuotas.Max);

            Message m = Message.CreateMessage(MessageVersion.Default, "", reader);
            return m;
        }

        public static string Serialize(Message m)
        {
            MemoryStream stream = new MemoryStream();
            XmlDictionaryWriter writer = XmlDictionaryWriter.CreateTextWriter(stream);
            m.WriteBodyContents(writer);
            writer.Flush();
            return Convert.ToBase64String(stream.ToArray());
        }
    }
}