// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Helpers
{
    using System;
    using System.Globalization;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Json;
    using System.Security;
    using System.Text;

    /// <summary>
    /// Just a bunch of helpers
    /// </summary>
    internal class InsightsUtil
    {
        private const string DateTimeFormat = "yyyy MM dd HH:mm:ss.fffffff";

        #region StringUtils

        /// <summary>
        /// Convert given char array to a secure string
        /// </summary>
        /// <param name="charArray"></param>
        /// <param name="start"></param>
        /// <param name="end"></param>
        /// <returns></returns>
        internal static SecureString SecureStringFromCharArray(char[] charArray, int start, int end)
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
        internal static string ConvertToUnsecureString(SecureString securePassword)
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

        #endregion StringUtils

        #region SerializationRelated

        /// <summary>
        /// Serialize a given object to string using JSON serializer
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="objectToBeSerialized"></param>
        /// <returns></returns>
        internal static string Serialize<T>(T objectToBeSerialized)
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
        internal static T DeSerialize<T>(string jsonSerializedStr)
        {
            using (var ms = new MemoryStream(Encoding.UTF8.GetBytes(jsonSerializedStr)))
            {
                DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(T), new DataContractJsonSerializerSettings { DateTimeFormat = new DateTimeFormat(DateTimeFormat), EmitTypeInformation = EmitTypeInformation.AsNeeded });
                return (T)deserializer.ReadObject(ms);
            }
        }

        #endregion SerializationRelated

        #region Validations

        /// <summary>
        /// Validate that a value lies between given range
        /// </summary>
        /// <typeparam name="T">Type of the value concerned</typeparam>
        /// <param name="value">value</param>
        /// <param name="minValue">Bottom of range</param>
        /// <param name="maxValue">Top end of range</param>
        /// <param name="optionalMsg"></param>
        internal static void ValidateInRange<T>(T value, T minValue, T maxValue, string optionalMsg = null) where T : IComparable<T>
        {
            if (value.CompareTo(minValue) < 0 || value.CompareTo(maxValue) > 0)
            {
                throw new ArgumentOutOfRangeException(
                          string.Format(CultureInfo.InvariantCulture, "Value '{0}' Not Between Min:'{1} and Max:'{2}'. Error '{3}'", value, minValue, maxValue, optionalMsg ?? string.Empty));
            }
        }

        #endregion Validations
    }
}