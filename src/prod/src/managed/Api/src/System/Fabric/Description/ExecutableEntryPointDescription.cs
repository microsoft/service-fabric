// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Provides information about the executable entry point.</para>
    /// </summary>
    public sealed class ExeHostEntryPointDescription : EntryPointDescription
    {
        private const long DefaultConsoleRedirectionFileRetentionCount = 2;
        private const long DefaultConsoleRedirectionFileMaxSizeInKb = 20480;

        private long periodicInterval;

        internal ExeHostEntryPointDescription()
            : base(CodePackageEntryPointKind.Exe)
        {
            this.Arguments = null;
            this.WorkingFolder = ExeHostWorkingFolder.Work;
            this.IsExternalExecutable = false;
            this.PeriodicInterval = 0;
            this.ConsoleRedirectionEnabled = false;
            this.ConsoleRedirectionFileRetentionCount = DefaultConsoleRedirectionFileRetentionCount;
            this.ConsoleRedirectionFileMaxSizeInKb = DefaultConsoleRedirectionFileMaxSizeInKb;
        }

        /// <summary>
        /// <para>Gets or sets the executable name as specified in the service manifest.</para>
        /// </summary>
        /// <value>
        /// <para>The executable name as specified in the service manifest.</para>
        /// </value>
        public string Program { get; internal set; }

        /// <summary>
        /// <para>Gets or sets the arguments passed to the executable as specified in the service manifest. </para>
        /// </summary>
        /// <value>
        /// <para>The arguments passed to the executable as specified in the service manifest.</para>
        /// </value>
        public string Arguments { get; internal set; }

        /// <summary>
        /// <para>Gets or sets the working folder for the executable as specified in the service manifest.</para>
        /// </summary>
        /// <value>
        /// <para>The working folder for the executable as specified in the service manifest.</para>
        /// </value>
        public ExeHostWorkingFolder WorkingFolder { get; internal set; }

        /// <summary>
        /// <para>Gets a value that indicates whether the program is an external executable outside of the code package.</para>
        /// </summary>
        /// <value>
        /// <para><languageKeyword>true</languageKeyword> if the program is an external executable outside of the code package. 
        /// Windows cluster loads external executables from the sequence of: working folder, Windows directory, and PATH environment variable.
        /// Linux cluster loads external executables from the standard directory. By default, the directory is "/usr/bin". It can be modifed from "LinuxExternalExecutablePath" config.</para>
        /// </value>
        public bool IsExternalExecutable { get; internal set; }

        /// <summary>
        /// <para>Gets or sets a value that indicates whether to enable or disable console redirection for executables. Default is <languageKeyword>false</languageKeyword>.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> to enable console redirection for executables; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        public bool ConsoleRedirectionEnabled { get; internal set; }

        /// <summary>
        /// <para>Gets or sets the maximum number of files used for console redirection before overwriting content in circular way. </para>
        /// </summary>
        /// <value>
        /// <para>The maximum number of files used for console redirection before overwriting content in circular way.</para>
        /// </value>
        public long ConsoleRedirectionFileRetentionCount { get; internal set; }

        /// <summary>
        /// <para>Gets or sets the maximum size in KB for console redirection file.</para>
        /// </summary>
        /// <value>
        /// <para>The maximum size in KB for console redirection file.</para>
        /// </value>
        public long ConsoleRedirectionFileMaxSizeInKb { get; internal set; }

        /// <summary>
        /// <para>Gets or sets the time period, if executable needs to be activated periodically. </para>
        /// </summary>
        /// <value>
        /// <para>The time period the executable needs to be activated periodically.</para>
        /// </value>
        public long PeriodicInterval
        {
            get
            {
                return this.periodicInterval;
            }

            internal set
            {
                // underlying com layer requires this to be a uint
                // a public type cannot have uint as it is not cls-compliant
                if (value > uint.MaxValue || value < 0)
                {
                    throw new ArgumentOutOfRangeException("value");
                }

                this.periodicInterval = value;
            }
        }

        /// <summary>
        /// <para>Gets the string representation of this entry point.</para>
        /// </summary>
        /// <returns>
        /// <para>The string representation of this entry point.</para>
        /// </returns>
        public override string ToString()
        {
            return string.Format(
                System.Globalization.CultureInfo.InvariantCulture,
                @"ExeHost(Program={0}, Arguments={1}, WorkingFolder={2}, PeriodicInterval={3}, ConsoleRedirectionEnabled={4}, ConsoleRedirectionFileRetentionCount={5}, ConsoleRedirectionFileMaxSizeInKb={6})",
                this.Program,
                this.Arguments,
                this.WorkingFolder,
                this.PeriodicInterval,
                this.ConsoleRedirectionEnabled,
                this.ConsoleRedirectionFileRetentionCount,
                this.ConsoleRedirectionFileMaxSizeInKb);
        }

        internal static new unsafe ExeHostEntryPointDescription CreateFromNative(IntPtr entryPointDescriptionPtr)
        {
            NativeTypes.FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION* nativeEntryPointDescription = (NativeTypes.FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION*)entryPointDescriptionPtr;

            ExeHostEntryPointDescription entryPointDescription = new ExeHostEntryPointDescription
            {
                Program = NativeTypes.FromNativeString(nativeEntryPointDescription->Program),
                Arguments = NativeTypes.FromNativeString(nativeEntryPointDescription->Arguments),
                WorkingFolder = (ExeHostWorkingFolder)nativeEntryPointDescription->WorkingFolder
            };

            if (nativeEntryPointDescription->Reserved != IntPtr.Zero)
            {
                NativeTypes.FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION_EX1* nativeEntryPointDescriptionEx1 = (NativeTypes.FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION_EX1*)nativeEntryPointDescription->Reserved;
                entryPointDescription.PeriodicInterval = nativeEntryPointDescriptionEx1->PeriodicIntervalInSeconds;
                entryPointDescription.ConsoleRedirectionEnabled = NativeTypes.FromBOOLEAN(nativeEntryPointDescriptionEx1->ConsoleRedirectionEnabled);

                // XSD has types as int for the following fields. Also minimum limit has been set to value > 0. Hence the cast is safe.
                entryPointDescription.ConsoleRedirectionFileRetentionCount = (long)nativeEntryPointDescriptionEx1->ConsoleRedirectionFileRetentionCount;
                entryPointDescription.ConsoleRedirectionFileMaxSizeInKb = (long)nativeEntryPointDescriptionEx1->ConsoleRedirectionFileMaxSizeInKb;

                if (nativeEntryPointDescriptionEx1->Reserved != IntPtr.Zero)
                {
                    NativeTypes.FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION_EX2* nativeEntryPointDescriptionEx2 = (NativeTypes.FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION_EX2*)nativeEntryPointDescriptionEx1->Reserved;
                    entryPointDescription.IsExternalExecutable = NativeTypes.FromBOOLEAN(nativeEntryPointDescriptionEx2->IsExternalExecutable);
                }
            }

            return entryPointDescription;
        }
    }
}