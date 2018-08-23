// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Fabric.Interop;

    internal class CabFileOperations
    {
        public static string GetCabVersion(string cabPath)
        {
            string tempPath = Helpers.GetNewTempPath();
            string fileToExtract = Path.Combine(Constants.PathToFabricCode, Constants.FabricExe);
            string tempFile = Path.Combine(tempPath, fileToExtract);
            ExtractFiltered(cabPath, tempPath, new string[] { fileToExtract }, true);
            if (!File.Exists(tempFile)) // Package is WU
            {
                fileToExtract = Constants.FabricExe;
                tempFile = Path.Combine(tempPath, fileToExtract);
                ExtractFiltered(cabPath, tempPath, new string[] { fileToExtract }, true);
                if (!File.Exists(tempFile))
                {
                    throw new FileNotFoundException(string.Format("{0} was not extracted from CAB package.", Constants.FabricExe));
                }
            }
            
            try
            {
                var fileVersion = FileVersionInfo.GetVersionInfo(tempFile);
                return string.Format(
                    CultureInfo.InvariantCulture,
                    "{0}.{1}.{2}.{3}",
                    fileVersion.FileMajorPart,
                    fileVersion.FileMinorPart,
                    fileVersion.FileBuildPart,
                    fileVersion.FilePrivatePart);
            }
            finally
            {
                if (Directory.Exists(tempPath))
                {
                    try
                    {
                        FabricDirectory.Delete(tempPath, true, true);
                    }
                    catch { }
                }
            }
        }

        public static void ExtractFiltered(string cabPath, string extractPath, string[] filters, bool inclusive)
        {
            var sb = new Text.StringBuilder();
            if(filters != null && filters.Length > 0)
            {
                sb.Append(filters[0]);
            }
            for(int i=1; i<filters.Length; i++)
            {
                sb.Append(',');
                sb.Append(filters[i]);
            }

            using (var pin = new PinCollection())
            {
                NativeCommon.CabExtractFiltered(
                    pin.AddBlittable(cabPath),
                    pin.AddBlittable(extractPath),
                    pin.AddBlittable(sb.ToString()),
                    NativeTypes.ToBOOLEAN(inclusive));
            }
        }

        public static bool IsCabFile(string cabPath)
        {
            if (string.IsNullOrWhiteSpace(cabPath))
            {
                return false;
            }

            using (var pin = new PinCollection())
            {
                return NativeTypes.FromBOOLEAN(NativeCommon.IsCabFile(pin.AddBlittable(cabPath)));
            }
        }
    }
}