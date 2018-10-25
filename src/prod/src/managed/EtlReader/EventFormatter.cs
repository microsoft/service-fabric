// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Text;

    public class EventFormatter
    {
        public const string StringEventTaskName = "String";
        private const ushort EventHeaderExtTypeRelatedActivityId = 0x1;

        private static int ExtendedDataSize = Marshal.SizeOf(typeof(EventHeaderExtendedData));

        internal enum FormatVersion
        {
            TaskBasedFormatting = 0,
            EventSourceBasedFormatting = 1,
        }

        internal static string FormatStringEvent(int formatVersion, EventRecord eventRecord, out string eventType, out string eventText)
        {
            ApplicationDataReader reader = new ApplicationDataReader(eventRecord.UserData, eventRecord.UserDataLength);
            string text = reader.ReadUnicodeString();
            string levelString;
            switch (eventRecord.EventHeader.EventDescriptor.Level)
            {
                case 2:
                    levelString = "Error";
                    break;
                case 3:
                    levelString = "Warning";
                    break;
                case 4:
                    levelString = "Informational";
                    break;
                case 5:
                    levelString = "Verbose";
                    break;
                default:
                    levelString = "Unknown";
                    break;
            }

            eventText = text;

            string taskName;
            switch (formatVersion)
            {
                case 0:
                    {
                        eventType = eventRecord.EventHeader.ProviderId.ToString(string.Empty);
                        taskName = StringEventTaskName;
                    }
                    break;

                default:
                    {
                        eventType = String.Empty;
                        taskName = String.Empty;
                    }
                    break;
            }

            return EventFormatter.FormatEvent((EventFormatter.FormatVersion)formatVersion, CultureInfo.InvariantCulture, eventRecord, null, levelString, taskName, String.Empty, String.Empty, eventType, text);
        }

        internal static string FormatEvent(FormatVersion formatVersion, IFormatProvider provider, EventRecord eventRecord, string eventSourceName, string eventLevel, string eventTaskName, string opcodeName, string keywordsName, string type, string text)
        {
            switch (formatVersion)
            {
                case FormatVersion.TaskBasedFormatting:
                    {
                        return string.Format(
                            provider,
                            "{0},{1},{2},{3},{4}.{5},{6}",
                            EventFormatter.FormatTimeStamp(provider, eventRecord.EventHeader.TimeStamp),
                            eventLevel,
                            eventRecord.EventHeader.ThreadId,
                            eventRecord.EventHeader.ProcessId,
                            eventTaskName,
                            type,
                            text);
                    }

                case FormatVersion.EventSourceBasedFormatting:
                    {
                        return string.Format(
                            provider,
                            "{0},{1},{2},{3},{4}.{5},{6}",
                            EventFormatter.FormatTimeStamp(provider, eventRecord.EventHeader.TimeStamp),
                            eventLevel,
                            eventRecord.EventHeader.ThreadId,
                            eventRecord.EventHeader.ProcessId,
                            eventSourceName,
                            type,
                            text);
                    }

                default:
                    {
                        return string.Format(
                            provider,
                            "{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11}",
                            EventFormatter.FormatTimeStamp(provider, eventRecord.EventHeader.TimeStamp),
                            eventRecord.EventHeader.ThreadId,
                            eventRecord.EventHeader.ProcessId,
                            eventRecord.EventHeader.EventDescriptor.Id,
                            eventRecord.EventHeader.ActivityId,
                            GetRelatedActivityId(eventRecord),
                            eventLevel,
                            String.IsNullOrEmpty(eventTaskName) ? 
                              eventRecord.EventHeader.EventDescriptor.Task.ToString(provider) : 
                              eventTaskName,
                            String.IsNullOrEmpty(opcodeName) ?
                              eventRecord.EventHeader.EventDescriptor.Opcode.ToString(provider) :
                              opcodeName,
                            String.IsNullOrEmpty(keywordsName) ?
                              eventRecord.EventHeader.EventDescriptor.Keyword.ToString("X", provider) :
                              keywordsName,
                            type,
                            text);
                    }
            }
        }

        private static string FormatTimeStamp(IFormatProvider provider, long fileTime)
        {
            DateTime timestamp = DateTime.FromFileTimeUtc(fileTime);

            return EventFormatter.FormatTimeStamp(provider, timestamp);
        }

        private static string FormatTimeStamp(IFormatProvider provider, DateTime timeStamp)
        {
            return timeStamp.ToString("yyyy-M-d HH:mm:ss.fff", provider);
        }

        private static Guid GetRelatedActivityId(EventRecord eventRecord)
        {
            Guid relatedActivityId = Guid.Empty;
            for (int i=0; i < eventRecord.ExtendedDataCount; i++)
            {
                IntPtr extendedDataPtr = new IntPtr(eventRecord.ExtendedData.ToInt64() + (i * ExtendedDataSize));
                Guid candidate;
                if (GetRelatedActivityId(extendedDataPtr, out candidate))
                {
                    relatedActivityId = candidate;
                    break;
                }
            }
            return relatedActivityId;
        }

        private static bool GetRelatedActivityId(IntPtr extendedDataPtr, out Guid relatedActivityId)
        {
            relatedActivityId = Guid.Empty;

            ApplicationDataReader reader = new ApplicationDataReader(extendedDataPtr, (ushort) ExtendedDataSize);
            ushort reserved1 = reader.ReadUInt16();
            ushort extType = reader.ReadUInt16();
            if (EventHeaderExtTypeRelatedActivityId != extType)
            {
                return false;
            }
            ushort reserved2 = reader.ReadUInt16();
            ushort dataSize = reader.ReadUInt16();
            ulong dataPtr = reader.ReadUInt64();

            ApplicationDataReader relatedActivityIdReader = new ApplicationDataReader(
                                                                    new IntPtr((long) dataPtr), 
                                                                    dataSize);
            relatedActivityId = relatedActivityIdReader.ReadGuid();
            return true;
        }
    }
}