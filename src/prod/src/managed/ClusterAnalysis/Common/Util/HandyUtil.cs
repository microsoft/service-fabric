// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Util
{
    using System;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Json;
    using System.Security;
    using System.Text;

    /// <summary>
    /// Collection of handy utility functions.
    /// </summary>
    public sealed class HandyUtil
    {
        private const string DateTimeFormat = "yyyy MM dd HH:mm:ss.fffffff";

        /// <summary>
        /// Convert given string to a secure string
        /// </summary>
        /// <param name="unsecureString"></param>
        /// <returns></returns>
        public static SecureString SecureStringFromCharArray(string unsecureString)
        {
            Assert.IsNotNull(unsecureString, "unsecureString != null");

            var array = unsecureString.ToCharArray();
            return SecureStringFromCharArray(array, 0, array.Length - 1);
        }

        /// <summary>
        /// Convert given char array to a secure string
        /// </summary>
        /// <param name="charArray"></param>
        /// <param name="start"></param>
        /// <param name="end"></param>
        /// <returns></returns>
        public static SecureString SecureStringFromCharArray(char[] charArray, int start, int end)
        {
            SecureString secureString = new SecureString();
            for (int i = start; i <= end; i++)
            {
                secureString.AppendChar(charArray[i]);
            }

            secureString.MakeReadOnly();
            return secureString;
        }

        /// <summary>
        /// convert given secure string to a normal string
        /// </summary>
        /// <param name="securePassword"></param>
        /// <returns></returns>
        public static string ConvertToUnsecureString(SecureString securePassword)
        {
            IntPtr unmanagedString = IntPtr.Zero;
            try
            {
                unmanagedString = Marshal.SecureStringToGlobalAllocUnicode(securePassword);
                return Marshal.PtrToStringUni(unmanagedString);
            }
            finally
            {
                Marshal.ZeroFreeGlobalAllocUnicode(unmanagedString);
            }
        }

        /// <summary>
        /// </summary>
        /// <param name="exp"></param>
        /// <returns></returns>
        public static string ExtractInformationFromException(AggregateException exp)
        {
            StringBuilder details = new StringBuilder();
            if (exp == null)
            {
                return details.ToString();
            }

            details.AppendFormat("Exception Type: {0}, Message: {1}, Stack: {2}", exp.GetType(), exp.Message, exp.StackTrace);

            exp = exp.Flatten();
            if (exp.InnerExceptions == null)
            {
                return details.ToString();
            }

            foreach (var oneExp in exp.Flatten().InnerExceptions)
            {
                details.AppendFormat("Inner Exception Details");
                details.AppendFormat("Exception Type: {0}, Message: {1}, Stack: {2}", oneExp.GetType(), oneExp.Message, oneExp.StackTrace);
                var enumerator = oneExp.Data.GetEnumerator();
                while (enumerator.MoveNext())
                {
                    details.AppendFormat("{0}={1}", enumerator.Key, enumerator.Value);
                }
            }

            return details.ToString();
        }

        #region SerializationRelated

        /// <summary>
        /// Serialize a given object to string using JSON serializer
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="objectToBeSerialized"></param>
        /// <returns></returns>
        public static string Serialize<T>(T objectToBeSerialized)
        {
            string serializedProcessCrashDetails;
            var serializer = new DataContractJsonSerializer(typeof(T), new DataContractJsonSerializerSettings { DateTimeFormat = new DateTimeFormat(DateTimeFormat), EmitTypeInformation = EmitTypeInformation.AsNeeded });

            using (var ms = new MemoryStream())
            {
                serializer.WriteObject(ms, objectToBeSerialized);

                // TODO: Investigate why UTF16 is causing crashes and then switch to it
                serializedProcessCrashDetails = Encoding.UTF8.GetString(ms.ToArray());
            }

            return serializedProcessCrashDetails;
        }

        /// <summary>
        /// Deserialize given string to object to requested type
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="jsonSerializedStr"></param>
        /// <returns></returns>
        public static T DeSerialize<T>(string jsonSerializedStr)
        {
            using (var ms = new MemoryStream(Encoding.UTF8.GetBytes(jsonSerializedStr)))
            {
                DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(T), new DataContractJsonSerializerSettings { DateTimeFormat = new DateTimeFormat(DateTimeFormat), EmitTypeInformation = EmitTypeInformation.AsNeeded });
                return (T)deserializer.ReadObject(ms);
            }
        }

        #endregion SerializationRelated
    }
}