// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.IO;

    public static class FileUtility
    {
        public static void CreateDirectoryIfNotExists(string path)
        {
            if (!Directory.Exists(path))
            {
                Directory.CreateDirectory(path);
            }
        }

        public static void RecursiveDelete(string directoryPath, IList<string> ignoredDirectories = null)
        {
            if (!Directory.Exists(directoryPath))
            {
                return;
            }

            string[] files = Directory.GetFiles(directoryPath);
            foreach (string file in files)
            {
                try
                {
                    if (Path.GetExtension(file).ToLowerInvariant() == ".dmp")
                    {
                        LogHelper.Log("Not deleting {0}", file);
                        continue;
                    }

                    File.Delete(file);
                }
                catch (Exception ex)
                {
                    LogHelper.Log("Unable to delete {0}. Error {1}", file, ex);
                }
            }

            string[] subDirectories = Directory.GetDirectories(directoryPath);
            foreach (string directory in subDirectories)
            {
                if (ignoredDirectories == null || !ignoredDirectories.Contains(directory))
                {
                    RecursiveDelete(directory, ignoredDirectories);
                    try
                    {
                        Directory.Delete(directory);
                    }
                    catch (Exception ex)
                    {
                        LogHelper.Log(string.Format(CultureInfo.InvariantCulture, "Unable to delete {0}. Error {1}", directory, ex));
                    }
                }
            }
        }

        public static void CopyDirectory(string sourcePath, string destinationPath)
        {
            FileUtility.CreateDirectoryIfNotExists(destinationPath);

            foreach (var file in Directory.EnumerateFiles(sourcePath))
            {
                string target = Path.Combine(destinationPath, Path.GetFileName(file));
                File.Copy(file, target, true);
            }

            foreach (var directory in Directory.EnumerateDirectories(sourcePath))
            {
                string target = Path.Combine(destinationPath, Path.GetFileName(directory));
                FileUtility.CopyDirectory(directory, target);
            }
        }

        public static void CopyFilesToDirectory(IEnumerable<string> sourcePaths, string destination)
        {
            FileUtility.CreateDirectoryIfNotExists(destination);

            foreach (var file in sourcePaths)
            {
                string target = Path.Combine(destination, Path.GetFileName(file));
                File.Copy(file, target, true);
            }
        }

        public static bool TryReadAllLines(string path, out string[] lines)
        {
            try
            {
                lines = File.ReadAllLines(path);
                return true;
            }
            catch (IOException)
            {
                lines = null;
                return false;
            }
        }
    }
}