// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Diagnostics.Tracing;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography.X509Certificates;
    using System.Security.Principal;
    using System.Text;
    using System.Xml;
    using System.Xml.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Threading.Tasks;
    using System.Net.NetworkInformation;
    using System.Security;
    using System.Net.Sockets;

    internal static class FabricValidatorUtility
    {
        internal class HostNameComparer : IEqualityComparer<string>
        {

            public bool Equals(string hostName1, string hostName2)
            {
                IdnMapping mapping = new IdnMapping();
                string hostName1Ascii = mapping.GetAscii(hostName1);
                string hostName2Ascii = mapping.GetAscii(hostName2);
                return hostName1Ascii.Equals(hostName2Ascii, StringComparison.OrdinalIgnoreCase);
            }

            public int GetHashCode(string hostName)
            {
                if (hostName == null)
                {
                    throw new ArgumentNullException("hostName");
                }
                return hostName.ToUpperInvariant().GetHashCode();
            }
        }

        const uint PKCS_7_ASN_ENCODING = 0x00010000;
        const uint X509_ASN_ENCODING = 0x00000001;
        const uint CERT_STORE_PROV_SYSTEM = 0x0000000A;
        const uint CERT_STORE_READONLY_FLAG = 0x00008000;
        const uint CERT_SYSTEM_STORE_LOCAL_MACHINE = 0x00020000;

        public static string TraceTag = "FabricDeployer";

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public static void UpdateTraceFile(string traceFileName)
        {
            TraceTextFileSink.SetPath(traceFileName);
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public static void CloseTraceFile()
        {
            TraceTextFileSink.SetPath("");
        }

        private static HashSet<char> InvalidFileNameChars = new HashSet<char>(Path.GetInvalidFileNameChars().ToList<char>());

        private static HashSet<string> CurrentMachineNames = FabricValidatorUtility.GetCurrentMachineNames();

        private static HashSet<IPAddress> CurrentMachineAddresses = FabricValidatorUtility.GetCurrentMachineAddresses();

        static FabricValidatorUtility()
        {
            // Print host details for debugging purposes
            PrintHostDetails();
        }

        private static HashSet<string> GetCurrentMachineNames()
        {
            HashSet<string> currentMachineNames = new HashSet<string>(new HostNameComparer());
            IPHostEntry entry = Helpers.GetHostEntryFromName(Dns.GetHostName()).Result;
            string domainName = System.Net.NetworkInformation.IPGlobalProperties.GetIPGlobalProperties().DomainName;
            if (!entry.HostName.Contains(domainName))
            {
                currentMachineNames.Add(entry.HostName);  //add computername
                currentMachineNames.Add(entry.HostName + "." + domainName);  //add FQDN
            }
            else
            {
                currentMachineNames.Add(entry.HostName);  //addFQDN
                string[] strs = entry.HostName.Split('.');
                if (strs.Length > 0)
                {
                    currentMachineNames.Add(strs[0]);   //add computername
                }
            }

            currentMachineNames.Add("localhost");            

            return currentMachineNames;
        }

        private static void PrintHostDetails()
        {
            try
            {
                FabricValidator.TraceSource.WriteInfo(
                    FabricValidatorUtility.TraceTag,
                    "Host name: {0}.", Dns.GetHostName());

                foreach (NetworkInterface ni in NetworkInterface.GetAllNetworkInterfaces())
                {
                    foreach (UnicastIPAddressInformation ip in ni.GetIPProperties().UnicastAddresses)
                    {
                        FabricValidator.TraceSource.WriteInfo(
                            FabricValidatorUtility.TraceTag,
                            "Network interface ip address: {0}.", ip.Address);
                    }
                }
            }
            catch (SocketException se)
            {
                FabricValidator.TraceSource.WriteError(
                    FabricValidatorUtility.TraceTag,
                    "Failed to get host name. Exception: {0}", se);
            }
            catch (NetworkInformationException ne)
            {
                FabricValidator.TraceSource.WriteError(
                    FabricValidatorUtility.TraceTag,
                    "Failed to get network interface information. Exception: {0}", ne);
            }
        }

        private static HashSet<IPAddress> GetCurrentMachineAddresses()
        {
#if !DotNetCoreClr
            IPHostEntry entry = Helpers.GetHostEntryFromName(Dns.GetHostName()).Result;
            HashSet<IPAddress> currentMachineAddresses = new HashSet<IPAddress>(entry.AddressList);
            currentMachineAddresses.Add(IPAddress.Loopback);
            currentMachineAddresses.Add(IPAddress.IPv6Loopback);
#else
            //TODO: Remove above conditional code block as this logic should work for both FullFx and CoreCLR.
            HashSet<IPAddress> currentMachineAddresses = new HashSet<IPAddress>();

            foreach (NetworkInterface ni in NetworkInterface.GetAllNetworkInterfaces())
            {
                foreach (UnicastIPAddressInformation ip in ni.GetIPProperties().UnicastAddresses)
                {
                    currentMachineAddresses.Add(ip.Address);
                }
            }
#endif
            return currentMachineAddresses;
        }

        public static bool ValidateDomainUri(string domain, string nodeName)
        {
            if (domain != null)
            {
                try
                {
                    Uri domainUri = new Uri(domain);
                    if (!domainUri.IsAbsoluteUri)
                    {
                        FabricValidator.TraceSource.WriteError(
                            FabricValidatorUtility.TraceTag,
                            "The domain uri '{0}' in Node {1} is not valid. The uri has to be an absolute Uri.",
                            domain,
                            nodeName);

                        return false;
                    }
                    if (string.IsNullOrEmpty(domainUri.PathAndQuery.Trim('/')))
                    {
                        FabricValidator.TraceSource.WriteError(
                            FabricValidatorUtility.TraceTag,
                            "The domain uri '{0}' in Node {1} is not valid. Uri needs to contain a path as we only use the path as the constraint.",
                            domain,
                            nodeName);

                        return false;
                    }
                }
                catch (UriFormatException exception)
                {
                    FabricValidator.TraceSource.WriteError(
                        FabricValidatorUtility.TraceTag,
                        "The domain uri '{0}' in Node {1} is not valid. The uri format is invalid. Exception: {2}",
                        domain,
                        nodeName,
                        exception);

                    return false;
                }
            }

            return true;
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public static T ReadXml<T>(string fileName)
        {
#if !DotNetCoreClr
            using (XmlReader reader = XmlReader.Create(fileName, new XmlReaderSettings() { XmlResolver = null }))
#else
            using (XmlReader reader = XmlReader.Create(fileName))
#endif
            {
                return FabricValidatorUtility.ReadXml<T>(reader);
            }
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public static T ReadXml<T>(XmlReader reader)
        {
            XmlSerializer serializer = new XmlSerializer(typeof(T));
            T obj = (T)serializer.Deserialize(reader);

            return obj;
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public static void WriteXml<T>(string fileName, T value)
        {
            XmlWriterSettings settings = new XmlWriterSettings();
            settings.Indent = true;
            using (FileStream filestream = new FileStream(fileName, FileMode.Create, FileAccess.Write, FileShare.Read))
            {
                using (XmlWriter writer = XmlWriter.Create(filestream, settings))
                {
                    FabricValidatorUtility.WriteXml<T>(writer, value);
                }
            }
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public static void WriteXml<T>(XmlWriter writer, T value)
        {
            XmlSerializer serializer = new XmlSerializer(typeof(T));
            serializer.Serialize(writer, value);
        }

        public static bool EqualsIgnoreCase(string string1, string string2)
        {
            return string.Equals(string1, string2, System.StringComparison.OrdinalIgnoreCase);
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public static bool NotEquals(string string1, string string2)
        {
            return !FabricValidatorUtility.EqualsIgnoreCase(string1, string2);
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public static SettingsTypeSection ToSettingsTypeSection(SettingsOverridesTypeSection overridesSection)
        {
            List<SettingsTypeSectionParameter> parameters = new List<SettingsTypeSectionParameter>();

            if ((overridesSection != null) && (overridesSection.Parameter != null))
            {
                foreach (SettingsOverridesTypeSectionParameter parameter in overridesSection.Parameter)
                {
                    parameters.Add(new SettingsTypeSectionParameter() { Name = parameter.Name, Value = parameter.Value, MustOverride = false, IsEncrypted = parameter.IsEncrypted });
                }
            }

            return new SettingsTypeSection() { Name = overridesSection.Name, Parameter = parameters.ToArray() };
        }

        public static bool IsValidFileName(string value)
        {
            foreach (char c in value)
            {
                if (InvalidFileNameChars.Contains(c))
                {
                    return false;
                }
            }

            return true;
        }

        public static bool IsNodeForThisMachine(InfrastructureNodeType node)
        {
            IPAddress ipAddress;
            if (IPAddress.TryParse(node.IPAddressOrFQDN, out ipAddress))
            {
                return (FabricValidatorUtility.CurrentMachineAddresses.Contains(ipAddress) ||
                         IsAddressLoopback(node.IPAddressOrFQDN));
            }
            else
            {
                return FabricValidatorUtility.CurrentMachineNames.Contains(node.IPAddressOrFQDN);
            }
        }

        public static bool IsNodeListScaleMin(List<InfrastructureNodeType> nodeList)
        {
            bool nodeFound = false;
            foreach (var node in nodeList)
            {
                if (FabricValidatorUtility.IsNodeForThisMachine(node))
                {
                    if (nodeFound)
                    {
                        return true;
                    }

                    nodeFound = true;
                }
            }

            return false;
        }

        public static bool IsAddressLoopback(string address)
        {
            IPAddress ipAddress;
            if (IPAddress.TryParse(address, out ipAddress))
            {
                return IPAddress.IsLoopback(ipAddress);
            }
            else
            {
                IdnMapping mapping = new IdnMapping();
                string inputHostName = mapping.GetAscii(address);
                string localhost = mapping.GetAscii("localhost");
                return inputHostName.Equals(localhost, StringComparison.OrdinalIgnoreCase);
            }
        }

        public static bool IsNodeAddressLoopback(InfrastructureNodeType node)
        {
            return IsAddressLoopback(node.IPAddressOrFQDN);
        }

        internal static void WriteWarningInVSOutputFormat(string type, string format, params object[] args)
        {
            FabricValidator.TraceSource.WriteWarning(
                type,
                "DeploymentValidator: warning: " + format,
                args);
            return;
        }

        internal static void ValidateExpression(Dictionary<string, WindowsFabricSettings.SettingsValue> settingsForThisSection, string parameterName)
        {
            string booleanExpression = settingsForThisSection[parameterName].Value;
            if (!string.IsNullOrEmpty(booleanExpression))
            {
                if (!InteropHelpers.ExpressionValidatorHelper.IsValidExpression(booleanExpression))
                {
                    throw new ArgumentException(string.Format(StringResources.Error_ExpressionValidationError, parameterName, booleanExpression));
                }
            }
        }

        public static bool StartsWithIgnoreCase(char[] secureSource, string text)
        {
            if (secureSource == null || secureSource == null)
            {
                return false;
            }

            if (secureSource.Length >= text.Length)
            {
                for (int i = 0; i < text.Length; i++)
                {
                    if (char.ToLowerInvariant(secureSource[i]) != char.ToLowerInvariant(text[i]))
                    {
                        return false;
                    }
                }

                return true;
            }

            return false;
        }

        public static bool EqualsIgnoreCase(char[] secureChars, string string1)
        {
            if (secureChars == null && string1 == null)
            {
                return true;
            }

            if (secureChars == null || string1 == null)
            {
                return false;
            }

            bool isEqual = false;
            if (secureChars.Length == string1.Length)
            {
                int i = 0;
                isEqual = secureChars.All(c => char.ToLowerInvariant(string1[i++]) == char.ToLowerInvariant(c));
            }

            return isEqual;
        }

        public static bool Equals(char[] array1, char[] array2)
        {
            if (array1 == null && array2 == null)
            {
                return true;
            }

            if (array1 == null || array2 == null)
            {
                return false;
            }

            bool isEqual = false;
            if (array1.Length == array2.Length)
            {
                int i = 0;
                isEqual = array1.All(c => c == array2[i++]);
            }

            return isEqual;
        }

        internal static char[] SecureStringToCharArray(SecureString secureString)
        {
            if (secureString == null)
            {
                return null;
            }

            char[] charArray = new char[secureString.Length];
#if DotNetCoreClr
            IntPtr ptr = SecureStringMarshal.SecureStringToGlobalAllocUnicode(secureString);
#else
            IntPtr ptr = Marshal.SecureStringToGlobalAllocUnicode(secureString);            
#endif
            try
            {
                Marshal.Copy(ptr, charArray, 0, secureString.Length);
            }
            finally
            {
#if DotNetCoreClr
                Marshal.FreeHGlobal(ptr);
#else
                Marshal.ZeroFreeGlobalAllocUnicode(ptr);	
#endif
            }
            return charArray;
        }

        internal static SecureString CharArrayToSecureString(char[] charArray)
        {
            if (charArray == null)
            {
                return null;
            }

            SecureString secureString = new SecureString();
            foreach (char c in charArray)
            {
                secureString.AppendChar(c);
            }

            secureString.MakeReadOnly();

            return secureString;
        }

        internal static SecureString DecryptValue(string certificateStoreName, string encryptedString)
        {
            IntPtr localMachineStoreHandle = IntPtr.Zero;
            SecureString decryptedValue = new SecureString();
            byte[] decryptedBuffer = null;
            char[] decryptedChars = null;
            CRYPT_DECRYPT_MESSAGE_PARA decryptMessageParameter = new CRYPT_DECRYPT_MESSAGE_PARA();

            try
            {
                localMachineStoreHandle = CertOpenStore(CERT_STORE_PROV_SYSTEM,
                    PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                    IntPtr.Zero,
                    CERT_STORE_READONLY_FLAG | CERT_SYSTEM_STORE_LOCAL_MACHINE,
                    certificateStoreName);
                if (localMachineStoreHandle == IntPtr.Zero)
                {
                    throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, StringResources.Error_CertOpenStoreReturnsError, Marshal.GetLastWin32Error()));
                }

                decryptMessageParameter.cbSize = Marshal.SizeOf(decryptMessageParameter);
                decryptMessageParameter.dwMsgAndCertEncodingType = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;
                decryptMessageParameter.cCertStore = 1;
                decryptMessageParameter.rghCertStore = Marshal.AllocHGlobal(IntPtr.Size);
                Marshal.WriteIntPtr(decryptMessageParameter.rghCertStore, localMachineStoreHandle);

                byte[] encryptedBuffer = Convert.FromBase64String(encryptedString);
                int decryptedBufferSize = 0;

                bool decryptSuccess = CryptDecryptMessage(ref decryptMessageParameter, encryptedBuffer, encryptedBuffer.Length, null, ref decryptedBufferSize, IntPtr.Zero);
                int errorCode = Marshal.GetLastWin32Error();
                if (decryptSuccess)
                {
                    decryptedBuffer = new byte[decryptedBufferSize];
                    decryptSuccess = CryptDecryptMessage(ref decryptMessageParameter, encryptedBuffer, encryptedBuffer.Length, decryptedBuffer, ref decryptedBufferSize, IntPtr.Zero);
                    errorCode = Marshal.GetLastWin32Error();
                    if (decryptSuccess)
                    {                        
                        decryptedChars = Encoding.Unicode.GetChars(decryptedBuffer, 0, decryptedBufferSize);
                        return CharArrayToSecureString(decryptedChars);
                    }
                }

                if (!decryptSuccess)
                {
                    throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, StringResources.Error_CryptDecryptMessageReturnsError, errorCode));
                }
            }
            catch (Exception e)
            {
                FabricValidator.TraceSource.WriteError(
                    FabricValidatorUtility.TraceTag,
                    "Decryption failed. StoreLocation: {0}, EncryptedString: {1}, Exception: {2}",
                    certificateStoreName,
                    encryptedString,
                    e);

                throw;
            }
            finally
            {
                decryptedValue.MakeReadOnly();

                if (decryptedBuffer != null)
                {
                    Array.Clear(decryptedBuffer, 0, decryptedBuffer.Length);
                }

                if (decryptedChars != null)
                {
                    Array.Clear(decryptedChars, 0, decryptedChars.Length);
                }

                if (decryptMessageParameter.rghCertStore != IntPtr.Zero)
                {
                    Marshal.FreeHGlobal(decryptMessageParameter.rghCertStore);
                }

                if (localMachineStoreHandle != IntPtr.Zero)
                {
                    CertCloseStore(localMachineStoreHandle, 0);
                }
            }

            return null;
        }

        public static bool IsValidIPAddressOrFQDN(string ipAddressOrFQDN)
        {
            IPAddress address;
            if (IPAddress.TryParse(ipAddressOrFQDN, out address))
            {
                return true;
            }
            IdnMapping maping = new IdnMapping();
            string asciiHostName = maping.GetAscii(ipAddressOrFQDN);
            return System.Uri.CheckHostName(asciiHostName) != System.UriHostNameType.Unknown;
        }

        [DllImport("crypt32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        internal static extern IntPtr CertOpenStore(
            uint storeProvider,
            uint encodingType,
            IntPtr hcryptProv,
            uint flags,
            String pvPara);

        [DllImport("crypt32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        internal static extern bool CertCloseStore(
            IntPtr storeProvider,
            uint flags);

        [DllImport("crypt32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        internal static extern bool CryptDecryptMessage(
            ref CRYPT_DECRYPT_MESSAGE_PARA pDecryptPara,
            byte[] pbEncryptedBlob,
            int cbEncryptedBlob,
            [In, Out] byte[] pbDecrypted,
            ref int pcbDecrypted,
            IntPtr ppXchgCert);
        
        [StructLayout(LayoutKind.Sequential)]
        internal struct CRYPT_DECRYPT_MESSAGE_PARA
        {
            public int cbSize;
            public uint dwMsgAndCertEncodingType;
            public int cCertStore;
            public IntPtr rghCertStore;
        }
    }
}