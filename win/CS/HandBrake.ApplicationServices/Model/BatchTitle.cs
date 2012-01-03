using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using HandBrake.ApplicationServices.Parsing;

namespace HandBrake.ApplicationServices.Model
{
    public class BatchTitle
    {
        public bool Include { get; set; }
        public string FileName { get; set; }

        public int TitleNumber { get { return Title.TitleNumber; } }
        public TimeSpan Duration { get { return Title.Duration; } }

        public Title Title { get; private set; }

        public BatchTitle(string fileName, Title title, bool include = false)
        {
            Title = title;
            Include = include;
            FileName = fileName;
        }
    }
}
