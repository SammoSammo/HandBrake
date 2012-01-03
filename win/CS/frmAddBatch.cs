using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using HandBrake.ApplicationServices.Parsing;
using HandBrake.ApplicationServices.Model;
using HandBrake.ApplicationServices.Services.Interfaces;
using HandBrake.ApplicationServices;
using HandBrake.ApplicationServices.Extensions;
using System.IO;

namespace Handbrake
{
    public partial class frmAddBatch : Form
    {
        private readonly Source discSource;
        private readonly frmMain mainWindow;
        private IUserSettingService userSettingService = ServiceManager.UserSettingService;

        protected BatchTitle[] BatchTitles { get; set; }

        public string OutputPath
        {
            get
            {
                return text_destination.Text;   
            }
        }
        private List<string> titles = null;

        TimeSpan minTimespan;
        TimeSpan maxTimespan;

        public frmAddBatch(frmMain mw, Source discSource)
        {
            InitializeComponent();

            if (this.DesignMode)
                return;

            this.discSource = discSource;
            this.mainWindow = mw;

            minTimespan = TimeSpan.Parse(ServiceManager.UserSettingService.GetUserSetting<string>(UserSettingConstants.BatchMinDuration));
            maxTimespan = TimeSpan.Parse(ServiceManager.UserSettingService.GetUserSetting<string>(UserSettingConstants.BatchMaxDuration));
        }

        public IEnumerable<BatchTitle> GetIncludedBatchTitles()
        {
            return from bt in BatchTitles
                   where bt.Include
                   select bt;
        }

        private void btn_Ok_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void btn_Cancel_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void frmAddBatch_Load(object sender, EventArgs e)
        {
            if (titles == null)
            {
                RefreshTitles();
            }
        }

        private void RefreshTitles()
        {
            var titles = from t in discSource.Titles
                              let include = t.Duration >= minTimespan && t.Duration < maxTimespan
                              let fileName = GetAutoNameTitle(t)
                              select new BatchTitle(fileName, t, include);

            dgv_DiscTitles.AutoGenerateColumns = false;

            BatchTitles = titles.ToArray();
            dgv_DiscTitles.DataSource = BatchTitles;

            text_destination.Text = GetAutoNamePath();
        }

        private string GetAutoNameTitle(Title title)
        {
                // Get the Source Name and remove any invalid characters
                string sourceName = Path.GetInvalidFileNameChars().Aggregate(mainWindow.SourceName, (current, character) => current.Replace(character.ToString(), string.Empty));
                sourceName = Path.GetFileNameWithoutExtension(sourceName);

                // Remove Underscores
                if (userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameRemoveUnderscore))
                    sourceName = sourceName.Replace("_", " ");

                // Switch to "Title Case"
                if (userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameTitleCase))
                    sourceName = sourceName.ToTitleCase();

                string dvdTitle = title.TitleNumber.ToString();

                // Get the Chapter Start and Chapter End Numbers
                string chapterStart = string.Empty;
                string chapterFinish = string.Empty;
                string combinedChapterTag = chapterStart;
              
                /*
                 * File Name
                 */ 
                string destinationFilename;
                if (userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat) != string.Empty)
                {
                    destinationFilename = userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat);
                    destinationFilename = destinationFilename.Replace("{source}", sourceName)
                                                             .Replace("{title}", dvdTitle)
                                                             .Replace("{chapters}", combinedChapterTag)
                                                             .Replace("{date}", DateTime.Now.Date.ToShortDateString().Replace('/', '-'));
                }
                else
                    destinationFilename = sourceName + "_T" + dvdTitle + "_C" + combinedChapterTag;

                /*
                 * File Extension
                 */ 
                if (mainWindow.drop_format.SelectedIndex == 0)
                {
                    switch (userSettingService.GetUserSetting<int>(UserSettingConstants.UseM4v))
                    {
                        case 0: // Automatic
                            destinationFilename += mainWindow.Check_ChapterMarkers.Checked ||
                                           mainWindow.AudioSettings.RequiresM4V() || mainWindow.Subtitles.RequiresM4V()
                                               ? ".m4v"
                                               : ".mp4";
                            break;
                        case 1: // Always MP4
                            destinationFilename += ".mp4";
                            break;
                        case 2: // Always M4V
                            destinationFilename += ".m4v";
                            break;
                    }
                }
                else if (mainWindow.drop_format.SelectedIndex == 1)
                    destinationFilename += ".mkv";

                return destinationFilename;
        }

        private string GetAutoNamePath()
        {
            string autoNamePath = userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath);
            return autoNamePath;
        }
    }
}
