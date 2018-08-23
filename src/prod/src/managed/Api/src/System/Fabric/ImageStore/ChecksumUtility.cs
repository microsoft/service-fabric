// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageStore
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Globalization;
    using System.IO;
    using System.Security.Cryptography;
#if !DotNetCoreClr
    using System.Security.Cryptography.Xml;
#endif
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    internal class ChecksumUtility
    {
        public static string ComputeHash(Stream stream)
        {
            byte[] hash = ComputeHashOnStream(stream);

            StringBuilder hashStringBuilder = new StringBuilder();
            for (int i = 0; i < hash.Length; i++)
            {
                hashStringBuilder.Append(string.Format(CultureInfo.InvariantCulture, "{0:X2}", hash[i]));
            }

            return hashStringBuilder.ToString();

        }

        public static string ComputeHash(string resourcePath, bool imageStoreServiceEnabled)
        {
            byte[] hash = null;

            if (FabricFile.Exists(resourcePath))
            {
                hash = ComputeHashOnFile(resourcePath);
            }
            else if (FabricDirectory.Exists(resourcePath))
            {
                hash = ComputeHashOnDirectory(resourcePath, imageStoreServiceEnabled);
            }
            else
            {
                System.Fabric.Interop.Utility.ReleaseAssert(true, "{0} is not found.");
            }

            StringBuilder hashStringBuilder = new StringBuilder();
            for (int i = 0; i < hash.Length; i++)
            {
                hashStringBuilder.Append(string.Format(CultureInfo.InvariantCulture, "{0:X2}", hash[i]));
            }

            return hashStringBuilder.ToString();
        }

        // ImageStoreService does not preserve file case when downloading. Hence the hash is always computed on
        // lower-cased dir/file name. This causes Test-ServiceFabricApplicationPackage to fail which has FileImageStore
        // as source but ImageStoreService as destination ImageStore. imageStoreServiceEnabled is passed in
        // so that we always use lowercase dir/file name to compute checksum when ImageStoreService is used and
        // support backward compatibility
        public static byte[] ComputeHashOnDirectory(string directory, bool imageStoreServiceEnabled)
        {
            List<Tuple<string, byte[], byte[]>> dirHashes = new List<Tuple<string, byte[], byte[]>>();
            List<Tuple<string, byte[], byte[]>> fileHashes = new List<Tuple<string, byte[], byte[]>>();

            Parallel.Invoke(() => Parallel.ForEach(FabricDirectory.GetDirectories(directory), subDirectory =>
            {
                var subDirName = Path.GetFileName(subDirectory.TrimEnd(new char[] { Path.DirectorySeparatorChar }));
                byte[] subDirectoryHash = ComputeHashOnDirectory(subDirectory, imageStoreServiceEnabled);
                string subDirectoryNameString = imageStoreServiceEnabled ? subDirName.ToLower() : subDirName;
                byte[] subDirectoryName = Encoding.GetEncoding(0).GetBytes(subDirectoryNameString);

                lock (dirHashes)
                {
                    dirHashes.Add(Tuple.Create(subDirectory, subDirectoryName, subDirectoryHash));
                }
            }),
                () => Parallel.ForEach(FabricDirectory.GetFiles(directory), file =>
                {
                    using (FileStream fileStream = FabricFile.Open(file, FileMode.Open, FileAccess.Read))
                    {
                        byte[] fileHash = ComputeHashOnStream(fileStream);
                        string fileNameString = imageStoreServiceEnabled ? Path.GetFileName(file).ToLower() : Path.GetFileName(file);
                        byte[] fileName = Encoding.GetEncoding(0).GetBytes(fileNameString);

                        lock (fileHashes)
                        {
                            fileHashes.Add(Tuple.Create(file, fileName, fileHash));
                        }
                    }
                }));

            using (Stream hashStream = new MemoryStream())
            {
                foreach (var t in dirHashes.OrderBy(t => t.Item1))
                {
                    hashStream.Write(t.Item2, 0, t.Item2.Length);
                    hashStream.Write(t.Item3, 0, t.Item3.Length);
                }

                foreach (var t in fileHashes.OrderBy(t => t.Item1))
                {
                    hashStream.Write(t.Item2, 0, t.Item2.Length);
                    hashStream.Write(t.Item3, 0, t.Item3.Length);
                }

                hashStream.Seek(0, SeekOrigin.Begin);

                return ComputeHashOnStream(hashStream);
            }
        }

        private static byte[] ComputeHashOnFile(string file)
        {
            using (FileStream fileStream = FabricFile.Open(file, FileMode.Open, FileAccess.Read))
            {
                return ComputeHashOnStream(fileStream);
            }
        }

        private static byte[] ComputeHashOnStream(Stream stream)
        {
#if DotNetCoreClr
            SHA256 hashAlgorithm = SHA256.Create();
            return hashAlgorithm.ComputeHash(stream);
#else
            using (var provider = new SHA256CryptoServiceProvider())
            {
                return provider.ComputeHash(stream);
            }
#endif
        }
    }
}