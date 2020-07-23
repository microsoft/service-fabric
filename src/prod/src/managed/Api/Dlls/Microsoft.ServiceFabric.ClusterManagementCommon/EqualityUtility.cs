// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.IO;
    using System.Security.Cryptography.Xml;
    using System.Xml.Serialization;

    internal class EqualityUtility
    {
        public static bool IsEqual<T>(T param1, T param2)
            where T : class
        {
            if (param1 == null && param2 == null)
            {
                return true;
            }

            if (param1 == null || param2 == null)
            {
                return false;
            }

            XmlSerializer serializer = new XmlSerializer(typeof(T));
            using (MemoryStream stream1 = new MemoryStream(), stream2 = new MemoryStream())
            {
                serializer.Serialize(stream1, param1);
                serializer.Serialize(stream2, param2);

                stream1.Seek(0, SeekOrigin.Begin);
                stream2.Seek(0, SeekOrigin.Begin);
                Stream canonicalizedStream1 = null, canonicalizedStream2 = null;
                try
                {
                    XmlDsigC14NTransform xmlCanonicalizer = new XmlDsigC14NTransform();
                    xmlCanonicalizer.LoadInput(stream1);
                    canonicalizedStream1 = (Stream)xmlCanonicalizer.GetOutput(typeof(Stream));

                    xmlCanonicalizer.LoadInput(stream2);
                    canonicalizedStream2 = (Stream)xmlCanonicalizer.GetOutput(typeof(Stream));

                    canonicalizedStream1.Seek(0, SeekOrigin.Begin);
                    canonicalizedStream2.Seek(0, SeekOrigin.Begin);

                    return Equals(new StreamReader(canonicalizedStream1).ReadToEnd(), new StreamReader(canonicalizedStream2).ReadToEnd());
                }
                finally
                {
                    if (canonicalizedStream1 != null)
                    {
                        canonicalizedStream1.Close();
                    }

                    if (canonicalizedStream2 != null)
                    {
                        canonicalizedStream2.Close();
                    }
                }
            }
        }

        public static bool IsNotEqual<T>(T param1, T param2)
            where T : class
        {
            return !IsEqual(param1, param2);
        }

        public static bool IsEqual<T>(T[] param1, T[] param2)
            where T : class
        {
            bool isEqual = IsArrayItemCountEqual(param1, param2);

            for (int i = 0; isEqual && (param2 != null) && i < param2.Length; i++)
            {
                isEqual = IsEqual(param1[i], param2[i]);
            }

            return isEqual;
        }

        public static bool IsNotEqual<T>(T[] param1, T[] param2)
            where T : class
        {
            return !IsEqual(param1, param2);
        }

        public static bool IsArrayItemCountEqual<T>(T[] param1, T[] param2)
            where T : class
        {
            if (param1 == null && param2 == null)
            {
                return true;
            }

            if (param1 == null || param2 == null || param1.Length != param2.Length)
            {
                return false;
            }
            else
            {
                return true;
            }
        }

        public static bool Equals(string string1, string string2)
        {
            return string.Equals(string1, string2, System.StringComparison.OrdinalIgnoreCase);
        }

        public static bool NotEquals(string string1, string string2)
        {
            return !Equals(string1, string2);
        }
    }
}