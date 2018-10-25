// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Xml;

    public class EventDefinition
    {
        private readonly EventDefinitionDescription description;
        private readonly EventStack stack;

        internal EventDefinition(EventDefinitionDescription description, EventStack stack)
        {
            if (description == null)
            {
                throw new ArgumentNullException("description");
            }

            this.description = description;
            this.stack = stack;            
        }

        internal EventDefinition()
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
            get { return this.description.Id; }
        }

        public int Version
        {
            get { return this.description.Version; }
        }

        internal string FormatEvent(EventRecord eventRecord, ValueMaps valueMaps, out string eventType, out string eventText, int formatVersion)
        {
            return this.description.FormatEvent(this.stack, eventRecord, valueMaps, out eventType, out eventText, formatVersion);
        }

        public string TaskName
        {
            get
            {
                return this.description.TaskName;
            }
        }

        public string EventName
        {
            get
            {
                return this.description.EventName;
            }
        }

        // TODO - Following code will be removed once fully transitioned to structured traces in Linux
        // This is needed so we can get the complete taskName for using with TableEvents.config
        public string OriginalEventName
        {
            get
            {
                return this.description.OriginalEventName;
            }
        }

        public string Level
        {
            get
            {
                return this.description.Level;
            }
        }

        public bool IsChildEvent
        {
            get
            {
                return this.description.IsChildEvent;
            }
        }

        public List<FieldDefinition> Fields
        {
            get
            {
                return this.description.Fields;
            }
        }

        public string TableInfo
        {
            get
            {
                return this.description.TableInfo;
            }
        }
    }
}