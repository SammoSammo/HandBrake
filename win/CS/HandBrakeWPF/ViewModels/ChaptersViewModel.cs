﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ChaptersViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Chapters View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.ComponentModel.Composition;
    using System.IO;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Exceptions;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using HandBrakeWPF.ViewModels.Interfaces;

    using Ookii.Dialogs.Wpf;

    /// <summary>
    /// The Chapters View Model
    /// </summary>
    [Export(typeof(IChaptersViewModel))]
    public class ChaptersViewModel : ViewModelBase, IChaptersViewModel
    {
        /// <summary>
        /// Gets or sets SourceChapterList.
        /// </summary>
        private ObservableCollection<Chapter> SourceChapterList { get; set; }

        /// <summary>
        /// Initializes a new instance of the <see cref="ChaptersViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public ChaptersViewModel(IWindowManager windowManager, IUserSettingService userSettingService)
        {
            this.Chapters = new ObservableCollection<ChapterMarker>();
        }

        /// <summary>
        /// Gets or sets State.
        /// </summary>
        public ObservableCollection<ChapterMarker> Chapters { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether chapter markers are enabled.
        /// </summary>
        public bool IncludeChapterMarkers { get; set; }

        /// <summary>
        /// Set the Source Chapters List
        /// </summary>
        /// <param name="sourceChapters">
        /// The source chapters.
        /// </param>
        public void SetSourceChapters(IEnumerable<Chapter> sourceChapters)
        {
            // Cache the chapters in this screen
            this.SourceChapterList = new ObservableCollection<Chapter>(sourceChapters);
            this.Chapters.Clear();

            // Then Add new Chapter Markers.
            foreach (Chapter chapter in SourceChapterList)
            {
                ChapterMarker marker = new ChapterMarker(chapter.ChapterNumber, chapter.ChapterName);
                this.Chapters.Add(marker);
            }
        }

        /// <summary>
        /// Export the Chapter Markers to a CSV file
        /// </summary>
        /// <param name="filename">
        /// The filename.
        /// </param>
        /// <exception cref="GeneralApplicationException">
        /// Thrown when exporting fails.
        /// </exception>
        public void ExportChaptersToCSV(string filename)
        {
            try
            {
                string csv = string.Empty;

                foreach (ChapterMarker row in this.Chapters)
                {
                    csv += row.ChapterNumber.ToString();
                    csv += ",";
                    csv += row.ChapterName.Replace(",", "\\,");
                    csv += Environment.NewLine;
                }
                StreamWriter file = new StreamWriter(filename);
                file.Write(csv);
                file.Close();
                file.Dispose();
            }
            catch (Exception exc)
            {
                throw new GeneralApplicationException("Unable to save Chapter Makrers file! ", "Chapter marker names will NOT be saved in your encode.", exc);
            }
        }

        /// <summary>
        /// Import a CSV file
        /// </summary>
        private void Import()
        {
            VistaOpenFileDialog dialog = new VistaOpenFileDialog { Filter = "CSV files (*.csv)|*.csv", CheckFileExists = true };
            dialog.ShowDialog();
            string filename = dialog.FileName;

            if (string.IsNullOrEmpty(filename))
            {
                return;
            }

            IDictionary<int, string> chapterMap = new Dictionary<int, string>();
            try
            {
                StreamReader sr = new StreamReader(filename);
                string csv = sr.ReadLine();
                while (csv != null)
                {
                    if (csv.Trim() != string.Empty)
                    {
                        csv = csv.Replace("\\,", "<!comma!>");
                        string[] contents = csv.Split(',');
                        int chapter;
                        int.TryParse(contents[0], out chapter);
                        chapterMap.Add(chapter, contents[1].Replace("<!comma!>", ","));
                    }
                    csv = sr.ReadLine();
                }
            }
            catch (Exception)
            {
                // Do Nothing
            }

            // Now iterate over each chatper we have, and set it's name
            foreach (ChapterMarker item in Chapters)
            {
                string chapterName;
                chapterMap.TryGetValue(item.ChapterNumber, out chapterName);
                item.ChapterName = chapterName;
                // TODO force a fresh of this property
            }
        }

        /// <summary>
        /// Export a CSV file.
        /// </summary>
        private void Export()
        {
            VistaSaveFileDialog saveFileDialog = new VistaSaveFileDialog { Filter = "Csv File|*.csv", DefaultExt = "csv", CheckPathExists = true };
            saveFileDialog.ShowDialog();
            if (!string.IsNullOrEmpty(saveFileDialog.FileName))
            {
                this.ExportChaptersToCSV(saveFileDialog.FileName);
            }
        }
    }
}
