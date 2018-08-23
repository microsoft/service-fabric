// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.IO;
    using System.Text;
    using System.Xml;

    public class EventDefinitionDescription
    {
        private readonly FormatString formatString;
        private readonly int typeFieldIndex;
        private readonly string level;
        private readonly int id;
        private readonly int version;
        private readonly string taskName;
        private readonly string eventSourceName;
        private readonly string originalEventName;
        private readonly string eventName;
        private readonly string opcodeName;
        private readonly string keywordsName;
        private readonly List<FieldDefinition> fields;
        private readonly bool isChildEvent;
        private readonly bool isParentEvent;
        private readonly bool hasIdField;
        private readonly string unparsedEventName;
        private int firstContextFieldIndex;
        private const string TableInfoPrefix = "_";

        public EventDefinitionDescription(XmlElement eventDefinition, XmlElement template, StringTable strings, bool wellKnownEvent, string eventSourceName)
        {
            if (eventDefinition == null)
            {
                throw new ArgumentNullException("eventDefinition");
            }

            if (strings == null)
            {
                throw new ArgumentNullException("strings");
            }

            // 'version' may not be defined
            if (!int.TryParse(eventDefinition.GetAttribute("version"), NumberStyles.Integer, CultureInfo.InvariantCulture, out this.version))
            {
                this.version = 0;
            }

            this.id = int.Parse(eventDefinition.GetAttribute("value"), CultureInfo.InvariantCulture);
            this.level = eventDefinition.GetAttribute("level").Substring(4);
            string message = eventDefinition.GetAttribute("message");
            this.taskName = eventDefinition.GetAttribute("task");
            this.eventSourceName = eventSourceName;

            this.unparsedEventName = this.eventName = this.originalEventName = eventDefinition.GetAttribute("symbol");
            this.opcodeName = eventDefinition.GetAttribute("opcode");
            this.keywordsName = eventDefinition.GetAttribute("keywords");
            this.TableInfo = String.Empty;

            if (wellKnownEvent)
            {
                int index = this.eventName.IndexOf('_');
                if (index > 0 && index + 1 < this.eventName.Length)
                {
                    index = this.eventName.IndexOf('_', index + 1);
                }

                if (index > 0 && index + 1 < this.eventName.Length)
                {
                    this.originalEventName = this.eventName = this.eventName.Substring(index + 1);

                    string eventGroupName;
                    string symbolPrefix;
                    GetEventNameParts(this.originalEventName, out symbolPrefix, out eventGroupName);

                    if (!String.IsNullOrEmpty(eventGroupName))
                    {
                        this.eventName = eventGroupName;
                    }
                    if (symbolPrefix.StartsWith(TableInfoPrefix, StringComparison.OrdinalIgnoreCase))
                    {
                        this.TableInfo = symbolPrefix.Substring(TableInfoPrefix.Length);
                    }
                }
            }

            this.formatString = new FormatString(strings.GetString(message));

            this.fields = new List<FieldDefinition>();

            this.firstContextFieldIndex = -1;

            if (template != null)
            {
                int toSkip = 0;
                foreach (XmlElement field in template.GetElementsByTagName("data"))
                {
                    FieldDefinition fieldDef = new FieldDefinition(
                        field.GetAttribute("name"),
                        field.GetAttribute("inType"),
                        field.GetAttribute("length"),
                        field.GetAttribute("map"),
                        wellKnownEvent,
                        (toSkip-- > 0));

                    if (fieldDef.EncodedFieldWidth > 1)
                    {
                        toSkip = fieldDef.EncodedFieldWidth - 1;
                    }

                    if (fieldDef.IsContextField && this.firstContextFieldIndex < 0)
                    {
                        this.firstContextFieldIndex = fields.Count;
                    }

                    this.fields.Add(fieldDef);
                }
            }

            this.isChildEvent = (fields.Count > 0 && fields[0].IsContextId);
            this.isParentEvent = (firstContextFieldIndex >= 0);
            if (this.isChildEvent)
            {
                firstContextFieldIndex = -1;
            }

            this.typeFieldIndex = wellKnownEvent && this.fields.Count != 0 ? this.fields.FindIndex(e => e.Name == "type") : -1;
            this.hasIdField = wellKnownEvent && this.fields.Count != 0 && this.fields[0].Name == "id";
        }

        internal EventDefinitionDescription()
        {
        }

        private static void GetEventNameParts(string fullEventName, out string eventNamePrefix, out string eventGroupName)
        {
            eventNamePrefix = String.Empty;
            eventGroupName = String.Empty;

            int index = fullEventName.LastIndexOf('_');
            if (index > 0 && index + 1 < fullEventName.Length)
            {
                eventNamePrefix = fullEventName.Remove(index);
                eventGroupName = fullEventName.Substring(index + 1);
            }
        }

        public static string GetEventGroupName(string eventName)
        {
            string eventGroupName;
            string dontCare;
            GetEventNameParts(eventName, out dontCare, out eventGroupName);
            return eventGroupName;
        }

        public int Id
        {
            get { return this.id; }
        }

        public int Version
        {
            get { return this.version; }
        }

        internal string FormatEvent(EventStack stack, EventRecord eventRecord, ValueMaps valueMaps, out string eventType, out string eventText, int formatVersion)
        {
            try
            {
                // This inefficient code is only a placeholder
                // once we make it work on all primitive types this will be 
                // pre-compiled Linq Expression EventRecord->String
                ApplicationDataReader reader = new ApplicationDataReader(eventRecord.UserData, eventRecord.UserDataLength);

                eventType = this.eventName;
                string type = this.originalEventName;
                string id = string.Empty;

                string[] values = new string[fields.Count];
                for (int i = 0; i < this.fields.Count; i++)
                {
                    FieldDefinition fieldDef = this.fields[i];
                    values[i] = fieldDef.FormatField(CultureInfo.InvariantCulture, reader, valueMaps);
                }

                if (this.isParentEvent)
                {
                    for (int i = this.fields.Count - 1; i >= 0; i--)
                    {
                        if (this.fields[i].IsContextField)
                        {
                            values[i] = stack.Pop(eventRecord.EventHeader.ProcessId, eventRecord.EventHeader.ThreadId, values[i], this.firstContextFieldIndex == i);
                            if (values[i] == null)
                            {
                                values[i] = " !!!Context Data Not Found!!! ";
                            }
                        }
                    }
                }

                if (this.typeFieldIndex != -1)
                {
                    type = eventType = values[typeFieldIndex];
                }

                eventText = FormatWithFieldValues(values, out id);

                if (id.Length > 0)
                {
                    type += "@" + id;
                }

                if (this.isChildEvent)
                {
                    stack.Push(eventRecord.EventHeader.ProcessId, eventRecord.EventHeader.ThreadId, values[0], eventText);
                    return null;
                }

                return EventFormatter.FormatEvent(
                    string.IsNullOrEmpty(this.eventSourceName) ? (EventFormatter.FormatVersion)formatVersion : EventFormatter.FormatVersion.EventSourceBasedFormatting,
                    CultureInfo.InvariantCulture,
                    eventRecord,
                    this.eventSourceName,
                    this.level,
                    this.taskName,
                    this.opcodeName,
                    this.keywordsName,
                    type,
                    eventText);
            }
            catch (Exception e)
            {
                throw new InvalidDataException(
                    string.Format(CultureInfo.InvariantCulture, "Unable to parse event {0}.{1} id {2} eventsource {3}", this.TaskName, this.EventName, this.id, this.eventSourceName),
                    e);
            }
        }

        private string FormatWithFieldValues(string[] fieldValues, out string id)
        {
            id = string.Empty;

            if (this.hasIdField)
            {
                id = fieldValues[0];
            }

            return this.formatString.ApplyValues(fieldValues);
        }

        public string TaskName
        {
            get
            {
                return taskName;
            }
        }

        public string EventName
        {
            get
            {
                return eventName;
            }
        }

        public string UnparsedEventName
        {
            get
            {
                return unparsedEventName;
            }
        }

        // TODO - Following code will be removed once fully transitioned to structured traces in Linux
        // This is needed so we can get the complete taskName for using with TableEvents.config
        public string OriginalEventName
        {
            get
            {
                return originalEventName;
            }
        }

        public string Level
        {
            get
            {
                return level;
            }
        }

        public bool IsChildEvent
        {
            get
            {
                return isChildEvent;
            }
        }

        public List<FieldDefinition> Fields
        {
            get
            {
                return this.fields;
            }
        }

        public string TableInfo { get; private set; }

        /// <summary>
        /// This class abstracts away the format string and creating a trace from the format string
        /// </summary>
        internal class FormatString
        {
            private readonly IList<Token> tokens;

            public FormatString(string formatString)
            {
                this.tokens = Tokenize(ConvertFromManifestFormattedString(formatString));
            }

            public string ApplyValues(string[] fieldValues)
            {
                StringBuilder sb = new StringBuilder();

                foreach (var token in this.tokens)
                {
                    if (token.IsString)
                    {
                        sb.Append(token.StringValue);
                    }
                    else
                    {
                        // The format string uses 1 based indexing
                        // The fields are zero based indexing
                        sb.Append(fieldValues[token.FieldIndex - 1]);
                    }
                }

                return sb.ToString();
            }

            #region Format string parsing code
            // https://msdn.microsoft.com/en-us/library/windows/desktop/dd996906(v=vs.85).aspx
            private static string ConvertFromManifestFormattedString(string message)
            {
                // Message can only become shorter.
                var sb = new StringBuilder(message.Length);

                // Escape seq are 2 chars so stop 1 short.
                var i = 0;
                for (; i < message.Length - 1; i++)
                {
                    if (message[i] != '%')
                    {
                        sb.Append(message[i]);
                        continue;
                    }

                    if (message[i + 1] == '%')
                    {
                        sb.Append('%');
                        i++;
                        continue;
                    }

                    if (message[i + 1] == '!')
                    {
                        sb.Append('!');
                        i++;
                        continue;
                    }

                    if (message[i + 1] == 'n')
                    {
                        sb.Append("\r\n");
                        i++;
                        continue;
                    }

                    if (message[i + 1] == 'r')
                    {
                        i++;
                        continue;
                    }

                    // For now we ignore other escapes
                    sb.Append(message[i]);
                }

                // Append last character if it wasn't part of escape sequence
                if (i == message.Length - 1)
                {
                    sb.Append(message[i]);
                }

                return sb.ToString();
            }

            private static IList<Token> Tokenize(string format)
            {
                // the format string is of the format "Abc %1 %2"
                // split it into a list of tokens
                List<Token> l = new List<Token>();

                int index = 0;
                while (true)
                {
                    var token = ReadToken(format, ref index);
                    if (token == null)
                    {
                        break;
                    }

                    l.Add(token);
                }

                return l;
            }

            private static Token ReadToken(string format, ref int index)
            {
                if (index == format.Length)
                {
                    return null;
                }

                // peek char
                if (format[index] == '%')
                {
                    // now pointing at the next integer
                    index++;
                    if (index == format.Length || !char.IsDigit(format[index]))
                    {
                        // Not value specifier
                        return new Token { IsString = true, StringValue = "%" };
                    }

                    return ReadIntegerToken(format, ref index);
                }

                return ReadStringToken(format, ref index);
            }

            private static Token ReadStringToken(string format, ref int index)
            {
                StringBuilder sb = new StringBuilder();
                while (true)
                {
                    if (index == format.Length)
                    {
                        break;
                    }

                    if (format[index] != '%')
                    {
                        if (format[index] == '\n')
                        {
                            sb.Append('\t');
                        }
                        else
                        {
                            sb.Append(format[index]);
                        }
                        index++;
                    }
                    else
                    {
                        break;
                    }
                }

                return new Token
                {
                    FieldIndex = -1,
                    IsString = true,
                    StringValue = sb.ToString()
                };
            }

            private static Token ReadIntegerToken(string format, ref int index)
            {
                int currentNumber = 0;

                while (true)
                {
                    if (index == format.Length)
                    {
                        break;
                    }

                    if (!char.IsDigit(format[index]))
                    {
                        break;
                    }

                    currentNumber *= 10;
                    currentNumber += (format[index] - '0');
                    index++;
                }

                return new Token
                {
                    FieldIndex = currentNumber,
                    IsString = false,
                    StringValue = null
                };
            }

            class Token
            {
                public bool IsString { get; set; }

                public int FieldIndex { get; set; }

                public string StringValue { get; set; }
            }
            #endregion
        }

    }
}