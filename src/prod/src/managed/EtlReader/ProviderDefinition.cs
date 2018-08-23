// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Linq;
    using System.Xml;
    using System.Xml.XPath;

    public class ProviderDefinition
    {
        private readonly EventStack stack;
        private readonly ProviderDefinitionDescription description;
        private readonly Dictionary<int, EventDefinition> events = new Dictionary<int, EventDefinition>();

        public ProviderDefinition(ProviderDefinitionDescription description)
        {
            if (description == null)
            {
                throw new ArgumentNullException("description");
            }

            this.description = description;
            this.stack = new EventStack();

            this.events = new Dictionary<int, EventDefinition>();
            this.stack = new EventStack();

            foreach (var eventDefinitionDescription in description.Events)
            {
                this.events.Add(eventDefinitionDescription.Key, new EventDefinition(eventDefinitionDescription.Value, this.stack));
            }
        }

        internal ProviderDefinition(ProviderDefinitionDescription description, Dictionary<int, EventDefinition> events)
        {
            this.description = description;
            this.events = events;
        }

        public ProviderDefinitionDescription Description
        {
            get { return this.description; }
        }

        public Guid Guid
        {
            get
            {
                return this.description.Guid;
            }
        }

        public string Name
        {
            get
            {
                return this.description.Name;
            }
        }

        public string FormatEvent(EventRecord eventRecord)
        {
            string unusedEventType;
            string unusedEventText;

            return this.FormatEvent(eventRecord, out unusedEventType, out unusedEventText);
        }

        public string FormatEvent(EventRecord eventRecord, out string eventType, out string eventText, int formatVersion=0)
        {
            eventType = null;
            eventText = null;

            EventDefinition eventDef;
            if (!this.events.TryGetValue(eventRecord.EventHeader.EventDescriptor.Id, out eventDef))
            {
                return null;
            }

            return eventDef.FormatEvent(eventRecord, this.description.ValueMaps, out eventType, out eventText, formatVersion);
        }

        public EventDefinition GetEventDefinition(EventRecord eventRecord)
        {
            //
            // Using the event key, find the matching event definition
            //
            EventDefinition eventDef;
            if (!this.events.TryGetValue(eventRecord.EventHeader.EventDescriptor.Id, out eventDef))
            {
                //
                // No matching event definition found
                //
                return null;
            }

            return eventDef;
        }
    }
}