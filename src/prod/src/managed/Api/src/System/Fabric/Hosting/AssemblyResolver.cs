// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Globalization;
    using System.IO;
    using System.Reflection;

    internal sealed class AssemblyResolver : IDisposable
    {
        private static readonly char[] Separators = new char[] { ',' };

        private readonly Dictionary<string, List<AssemblyResolutionEntry>> resolutionEntries = new Dictionary<string, List<AssemblyResolutionEntry>>();

        private bool disposed;

        public AssemblyResolver()
        {
            AppDomain.CurrentDomain.AssemblyResolve += new ResolveEventHandler(this.OnResolveAssembly);
        }

        [SuppressMessage(FxCop.Category.Globalization, FxCop.Rule.NormalizeStringsToUppercase, Justification = "Lowercase required.")]
        public void AddApplicationBinariesPath(string path)
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException("AssemblyResolver");
            }

            foreach (string file in Directory.EnumerateFiles(path, "*.*", SearchOption.AllDirectories))
            {
                string extension = Path.GetExtension(file);
                if (string.Compare(extension, ".dll", StringComparison.OrdinalIgnoreCase) == 0 ||
                    string.Compare(extension, ".exe", StringComparison.OrdinalIgnoreCase) == 0)
                {
                    TextInfo textInfo = CultureInfo.InvariantCulture.TextInfo;
                    string key = textInfo.ToLower(Path.GetFileNameWithoutExtension(file));

                    AssemblyResolutionEntry thisEntry = new AssemblyResolutionEntry()
                    {
                        AssemblyPath = file.ToLowerInvariant(),
                        PerformedResolution = false
                    };

                    List<AssemblyResolutionEntry> entries;
                    if (this.resolutionEntries.TryGetValue(key, out entries))
                    {
                        // this function may be called multiple times - say application 1 gets loaded with root path E:\node\applications
                        // and then application 2 gets loaded and the root path is E:\node\applications
                        // avoid adding duplicate entries for files we have already seen in such a case
                        if (!entries.Exists(c => string.Compare(c.AssemblyPath, file, StringComparison.OrdinalIgnoreCase) == 0))
                        {
                            entries.Add(thisEntry);
                            AppTrace.TraceSource.WriteNoise("AssemblyResolver.AddApplicationBinariesPath", "Adding assembly entry for {0}", thisEntry.AssemblyPath);
                        }
                    }
                    else
                    {
                        entries = new List<AssemblyResolutionEntry>();
                        entries.Add(thisEntry);
                        this.resolutionEntries.Add(key, entries);
                        AppTrace.TraceSource.WriteNoise("AssemblyResolver.AddApplicationBinariesPath", "Adding assembly entry for {0}", thisEntry.AssemblyPath);
                    }
                }
            }
        }

        #region IDisposable Members

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing)
        {
            if (!this.disposed && disposing)
            {
                AppDomain.CurrentDomain.AssemblyResolve -= this.OnResolveAssembly;
                this.resolutionEntries.Clear();
                this.disposed = true;
            }
        }

        #endregion

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.AvoidCallingProblematicMethods, Justification = "See the comment around Assembly.LoadFrom() call.")]
        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotCatchGeneralExceptionTypes,
                Justification = "The logic of assembly resolution has to suppress exceptions.")]
        [SuppressMessage(FxCop.Category.Globalization, FxCop.Rule.NormalizeStringsToUppercase, Justification = "Lowercase required.")]
        private Assembly OnResolveAssembly(object sender, ResolveEventArgs args)
        {
            if (string.IsNullOrWhiteSpace(args.Name))
            {
                return null;
            }

            string[] assemblyNameParts = args.Name.Split(AssemblyResolver.Separators, StringSplitOptions.RemoveEmptyEntries);
            if (assemblyNameParts == null ||
                assemblyNameParts.Length == 0 ||
                string.IsNullOrWhiteSpace(assemblyNameParts[0]))
            {
                return null;
            }

            TextInfo textInfo = CultureInfo.InvariantCulture.TextInfo;
            string assemblyName = textInfo.ToLower(assemblyNameParts[0]);

            Assembly resolvedAssembly = null;
            List<AssemblyResolutionEntry> entries = null;
            if (this.resolutionEntries.TryGetValue(assemblyName, out entries))
            {
                AppTrace.TraceSource.WriteNoise("AssemblyResolver.OnResolveAssembly", "Trying to load {0}", args.Name);
                foreach (AssemblyResolutionEntry entry in entries)
                {
                    if (!entry.PerformedResolution)
                    {
                        try
                        {
                            string assemblyPath = entry.AssemblyPath;
                            if (File.Exists(assemblyPath))
                            {
                                // Even when using LoadFrom this assembly is loaded into the default load context,
                                // because it is returned from AppDomain.AssemblyResolve as long as args.RequestingAssembly != null
                                // and the args.RequestingAssembly wasn't loaded without context
                                entry.ResolvedAssembly = Assembly.LoadFrom(assemblyPath);
                            }
                        }
                        catch (Exception e)
                        {
                            // Eat assembly load exception
                            AppTrace.TraceSource.WriteNoise("AssemblyResolver.OnResolveAssembly", "Assembly.LoadFrom({0}) failed with exception {1}", entry.AssemblyPath, e.ToString());
                        }
                        finally
                        {
                            entry.PerformedResolution = true;
                        }
                    }

                    if (entry.ResolvedAssembly != null)
                    {
                        if (args.Name.Equals(entry.ResolvedAssembly.FullName, StringComparison.OrdinalIgnoreCase))
                        {
                            AppTrace.TraceSource.WriteNoise("AssemblyResolver.OnResolveAssembly", "Assembly {0} loaded", entry.ResolvedAssembly);
                            resolvedAssembly = entry.ResolvedAssembly;
                            break;
                        }
                    }
                }
            }
            else
            {
                AppTrace.TraceSource.WriteNoise("AssemblyResolver.OnResolveAssembly", "Unknown assembly {0}", args.Name);
            }

            return resolvedAssembly;
        }

        private sealed class AssemblyResolutionEntry
        {
            internal string AssemblyPath
            {
                get;
                set;
            }

            internal bool PerformedResolution
            {
                get;
                set;
            }

            internal Assembly ResolvedAssembly
            {
                get;
                set;
            }
        }
    }
}