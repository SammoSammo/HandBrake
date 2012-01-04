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
            foreach (var title in BatchTitles)
            {
                title.OutputFolder = text_destination.Text;
            }

            this.Close();
        }

        private void btn_Cancel_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void frmAddBatch_Load(object sender, EventArgs e)
        {
            RefreshTitles();
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

        private void dgv_DiscTitles_CellMouseClick(object sender, DataGridViewCellMouseEventArgs e)
        {
            if (e.RowIndex > -1)
            {
                BeginEditingFileName(e.RowIndex);
            }
        }

        private void dgv_DiscTitles_CellEnter(object sender, DataGridViewCellEventArgs e)
        {
            if (e.RowIndex > -1 && e.RowIndex < dgv_DiscTitles.Rows.Count)
            {
                BeginEditingFileName(e.RowIndex);
            }
        }

        private void BeginEditingFileName(int rowNum)
        {
            dgv_DiscTitles.Rows[rowNum].Cells["dgvTitles_FileName"].Selected = true;

            dgv_DiscTitles.BeginEdit(false);

            var editControl = dgv_DiscTitles.EditingControl as DataGridViewTextBoxEditingControl;

            if (editControl != null && !string.IsNullOrEmpty(editControl.Text))
            {
                //select just the file name bit, this makes it quicker to edit.
                var fileNameWithoutExtension = Path.GetFileNameWithoutExtension(editControl.Text);
                editControl.Select(0, fileNameWithoutExtension.Length);
            }
        }

        private void dgv_DiscTitles_CellValueChanged(object sender, DataGridViewCellEventArgs e)
        {
            //if we're leaving the title cell, ensure the text is a valid file name.
            if (e.RowIndex > -1 && e.ColumnIndex == dgv_DiscTitles.Columns["dgvTitles_FileName"].Index)
            {
                var cell = dgv_DiscTitles.Rows[e.RowIndex].Cells[e.ColumnIndex] as DataGridViewTextBoxCell;

                if (cell != null && cell.Value != null && !string.IsNullOrEmpty(cell.Value.ToString()))
                {
                    string originalValue = cell.Value.ToString();
                    string value = originalValue;

                    int initLength = value.Length;

                    foreach (var badChar in Path.GetInvalidFileNameChars())
                    {
                        value = value.Replace(badChar.ToString(), string.Empty);
                    }

                    if (value.Length < originalValue.Length)
                    {
                        string message = string.Format("The filename you have entered has invalid characters. Would you like to replace it with the name '{0}'?", value);

                        if (MessageBox.Show(message, "Invalid character in file name", MessageBoxButtons.YesNo, MessageBoxIcon.Exclamation) == DialogResult.Yes)
                        {
                              cell.Value = value;
                        }
                    }
                }
            }
        }
    }
}
