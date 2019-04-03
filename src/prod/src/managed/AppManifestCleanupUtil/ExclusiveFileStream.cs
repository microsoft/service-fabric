// ------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT License (MIT).See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace AppManifestCleanupUtil
{
    using System;
    using System.IO;
    using System.Threading;

    internal class ExclusiveFileStream : IDisposable
    {
        private const int MaxAttempts = 60;
        private const int MaxRetryIntervalMillis = 1000;
        private const int MinRetryIntervalMillis = 100;
        private static readonly Random Rand = new Random();

        private ExclusiveFileStream(FileStream stream)
        {
            this.Value = stream;
        }

        public FileStream Value
        {
            get;
            private set;
        }

        public static ExclusiveFileStream Acquire(
            string path,
            FileMode fileMode,
            FileShare fileShare,
            FileAccess fileAccess)
        {
            var numAttempts = 0;
            while (true)
            {
                numAttempts++;
                try
                {
                    var fileStream = File.Open(
                        path,
                        fileMode,
                        fileAccess,
                        fileShare);

                    return new ExclusiveFileStream(fileStream);
                }
                catch (IOException)
                {
                    Thread.Sleep(Rand.Next(MinRetryIntervalMillis, MaxRetryIntervalMillis));
                    if (numAttempts > MaxAttempts)
                    {
                        throw;
                    }
                }
            }
        }

        public void Dispose()
        {
            this.Value.Dispose();
        }
    }
}
