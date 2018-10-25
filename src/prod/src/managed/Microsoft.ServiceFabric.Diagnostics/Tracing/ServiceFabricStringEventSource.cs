// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Diagnostics.Tracing
{
    using System.Diagnostics.Tracing;
    using System.Globalization;

    [EventSource(Guid = "74CF0846-E6A3-4a3e-A10F-80FD527DA5FD", Name = "StringEvent")]
    internal sealed class ServiceFabricStringEventSource : ServiceFabricEventSource
    {
        internal static readonly ServiceFabricStringEventSource Instance = new ServiceFabricStringEventSource();

        private ServiceFabricStringEventSource()
        {
        }

        #region NonEvents

        [NonEvent]
        internal void WriteError(string type, string format, params object[] args)
        {
            this.WriteErrorWithId(type, string.Empty, format, args);
        }

        [NonEvent]
        internal void WriteErrorWithId(string type, string id, string format, params object[] args)
        {
            if (null == args || 0 == args.Length)
            {
                Instance.ErrorText(id, type, format);
            }
            else
            {
                Instance.ErrorText(id, type, string.Format(CultureInfo.InvariantCulture, format, args));
            }
        }

        [NonEvent]
        internal void WriteWarning(string type, string format, params object[] args)
        {
            this.WriteWarningWithId(type, string.Empty, format, args);
        }

        [NonEvent]
        internal void WriteWarningWithId(string type, string id, string format, params object[] args)
        {
            if (args == null || args.Length == 0)
            {
                Instance.WarningText(id, type, format);
            }
            else
            {
                Instance.WarningText(id, type, string.Format(CultureInfo.InvariantCulture, format, args));
            }
        }

        [NonEvent]
        internal void WriteInfo(string type, string format, params object[] args)
        {
            this.WriteInfoWithId(type, string.Empty, format, args);
        }

        [NonEvent]
        internal void WriteInfoWithId(string type, string id, string format, params object[] args)
        {
            if (null == args || 0 == args.Length)
            {
                Instance.InfoText(id, type, format);
            }
            else
            {
                Instance.InfoText(id, type, string.Format(CultureInfo.InvariantCulture, format, args));
            }
        }

        [NonEvent]
        internal void WriteNoise(string type, string format, params object[] args)
        {
            this.WriteNoiseWithId(type, string.Empty, format, args);
        }

        [NonEvent]
        internal void WriteNoiseWithId(string type, string id, string format, params object[] args)
        {
            if (null == args || 0 == args.Length)
            {
                Instance.NoiseText(id, type, format);
            }
            else
            {
                Instance.NoiseText(id, type, string.Format(CultureInfo.InvariantCulture, format, args));
            }
        }

        #endregion

        #region Events
        [Event(1, Message = "{2}", Level = EventLevel.Informational, Keywords = Keywords.Default)]
        private void InfoText(string id, string type, string message)
        {
            this.WriteEvent(1, id, type, message);
        }

        [Event(2, Message = "{2}", Level = EventLevel.Warning, Keywords = Keywords.Default)]
        private void WarningText(string id, string type, string message)
        {
            this.WriteEvent(2, id, type, message);
        }

        [Event(3, Message = "{2}", Level = EventLevel.Error, Keywords = Keywords.Default)]
        private void ErrorText(string id, string type, string message)
        {
            this.WriteEvent(3, id, type, message);
        }

        [Event(4, Message = "{2}", Level = EventLevel.Verbose, Keywords = Keywords.Default)]
        private void NoiseText(string id, string type, string message)
        {
            this.WriteEvent(4, id, type, message);
        }
        #endregion

        #region Keywords / Tasks / Opcodes

        public static class Keywords
        {
            public const EventKeywords Default = (EventKeywords)0x0001;
        }

        #endregion
    }
}