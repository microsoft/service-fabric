// ------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT License (MIT).See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace AppManifestCleanupUtil
{
    using System.Collections.Generic;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Text;

    internal static class Utility
    {
        private const int FileAttributeDirectory = 0x10;
        private const int FileAttributeNormal = 0x80;

        public static StreamReader CreateStreamReader(FileStream fileStream)
        {
            return new StreamReader(fileStream, Encoding.UTF8, true, 4096, true);
        }

        public static StreamWriter CreateStreamWriter(FileStream fileStream)
        {
            return new StreamWriter(fileStream, Encoding.UTF8, 4096, true);
        }

        public static string LoadContents(string filePath)
        {
            if (!File.Exists(filePath))
            {
                return string.Empty;
            }

            using (var fileStream = ExclusiveFileStream.Acquire(
                filePath,
                FileMode.Open,
                FileShare.Read,
                FileAccess.Read))
            {
                fileStream.Value.Seek(0, SeekOrigin.Begin);
                using (var reader = CreateStreamReader(fileStream.Value))
                {
                    return reader.ReadToEnd();
                }
            }
        }

        public static void WriteContents(FileStream stream, string contents)
        {
            var sb = new StringBuilder(contents);
            sb.AppendLine();
            sb.Append(' ');
            contents = sb.ToString();

            stream.Seek(0, SeekOrigin.Begin);
            using (var writer = CreateStreamWriter(stream))
            {
                writer.Write(contents);
                writer.Flush();

                var contentLegnth = writer.Encoding.GetByteCount(contents);
                stream.SetLength(contentLegnth);
                stream.Flush();
            }
        }

        public static void WriteIfNeeded(string filePath, string existingContents, string newContents)
        {
            if (string.CompareOrdinal(newContents.Trim(), existingContents.Trim()) == 0)
            {
                return;
            }

            using (var fileStream = ExclusiveFileStream.Acquire(
                filePath,
                FileMode.OpenOrCreate,
                FileShare.Read,
                FileAccess.ReadWrite))
            {
                WriteContents(fileStream.Value, newContents);
            }
        }

        public static void PerformPlaceholderReplacements(
            StringBuilder template,
            IDictionary<string, string> placeHolders)
        {
            foreach (var p in placeHolders)
            {
                template.Replace(p.Key, p.Value);
            }
        }

        public static void WriteFile(string filePath, string fileContents)
        {
            EnsureFolder(Path.GetDirectoryName(filePath));
            File.WriteAllText(filePath, fileContents);
        }

        public static void EnsureParentFolder(string filePath)
        {
            var folderPath = Path.GetDirectoryName(filePath);
            if ((folderPath != null) && (!Directory.Exists(folderPath)))
            {
                Directory.CreateDirectory(folderPath);
            }
        }

        public static void EnsureFolder(string folderPath)
        {
            if (!Directory.Exists(folderPath))
            {
                Directory.CreateDirectory(folderPath);
            }
        }

        public static string GetRelativePath(string fromPath, string toPath)
        {
            var fromAttr = GetPathAttribute(fromPath);
            var toAttr = GetPathAttribute(toPath);

            var path = new StringBuilder(256);
            return PathRelativePathTo(
                path,
                fromPath,
                fromAttr,
                toPath,
                toAttr) == 0
                ? toPath
                : path.ToString();
        }

        private static int GetPathAttribute(string path)
        {
            var di = new DirectoryInfo(path);
            if (di.Exists)
            {
                return FileAttributeDirectory;
            }

            var fi = new FileInfo(path);
            if (fi.Exists)
            {
                return FileAttributeNormal;
            }

            throw new FileNotFoundException();
        }

        [DllImport("shlwapi.dll", SetLastError = true)]
        private static extern int PathRelativePathTo(
            StringBuilder pszPath,
            string pszFrom,
            int dwAttrFrom,
            string pszTo,
            int dwAttrTo);
    }
}
