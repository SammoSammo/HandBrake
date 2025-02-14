﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AutoNameHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the AutoNameHelper type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System;
    using System.IO;
    using System.Linq;

    using HandBrake.ApplicationServices.Extensions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// The Destination AutoName Helper
    /// </summary>
    public class AutoNameHelper
    {
        /// <summary>
        /// Backing field for the user setting service
        /// </summary>
        private static IUserSettingService userSettingService;

        /// <summary>
        /// Initializes a new instance of the <see cref="AutoNameHelper"/> class.
        /// </summary>
        /// <param name="userSetting">
        /// The user Setting.
        /// </param>
        public AutoNameHelper(IUserSettingService userSetting)
        {
            userSettingService = userSetting;
        }

        /// <summary>
        /// Function which generates the filename and path automatically based on 
        /// the Source Name, DVD title and DVD Chapters
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        /// <param name="sourceOrLabelName">
        /// The Source or Label Name
        /// </param>
        /// <returns>
        /// The Generated FileName
        /// </returns>
        public static string AutoName(EncodeTask task, string sourceOrLabelName)
        {
            string autoNamePath = string.Empty;
            if (task.Title != 0) // TODO check.
            {
                // Get the Source Name and remove any invalid characters
                string sourceName = Path.GetInvalidFileNameChars().Aggregate(sourceOrLabelName, (current, character) => current.Replace(character.ToString(), string.Empty));
                sourceName = Path.GetFileNameWithoutExtension(sourceName);

                // Remove Underscores
                if (userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameRemoveUnderscore))
                    sourceName = sourceName.Replace("_", " ");

                // Switch to "Title Case"
                if (userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameTitleCase))
                    sourceName = sourceName.ToTitleCase();

                // Get the Selected Title Number

                string dvdTitle = task.Title.ToString();

                // Get the Chapter Start and Chapter End Numbers
                string chapterStart = task.StartPoint.ToString();
                string chapterFinish = task.EndPoint.ToString();
                string combinedChapterTag = chapterStart;
                if (chapterFinish != chapterStart && chapterFinish != string.Empty)
                    combinedChapterTag = chapterStart + "-" + chapterFinish;

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
                if (task.OutputFormat == OutputFormat.Mp4 || task.OutputFormat == OutputFormat.M4V)
                {
                    switch (userSettingService.GetUserSetting<int>(UserSettingConstants.UseM4v))
                    {
                        case 0: // Automatic
                            destinationFilename += task.IncludeChapterMarkers || task.RequiresM4v ? ".m4v" : ".mp4";
                            break;
                        case 1: // Always MP4
                            destinationFilename += ".mp4";
                            break;
                        case 2: // Always M4V
                            destinationFilename += ".m4v";
                            break;
                    }
                }
                else if (task.OutputFormat == OutputFormat.Mkv)
                    destinationFilename += ".mkv";

                /*
                 * File Destination Path
                 */

                // If there is an auto name path, use it...
                if (userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Trim().StartsWith("{source_path}") && !string.IsNullOrEmpty(task.Source))
                {
                    string savedPath = userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Trim().Replace("{source_path}\\", string.Empty).Replace("{source_path}", string.Empty);

                    string directory = Directory.Exists(task.Source)
                                           ? task.Source
                                           : Path.GetDirectoryName(task.Source);
                    string requestedPath = Path.Combine(directory, savedPath);

                    autoNamePath = Path.Combine(requestedPath, destinationFilename);
                    if (autoNamePath == task.Source)
                    {
                        // Append out_ to files that already exist or is the source file
                        autoNamePath = Path.Combine(Path.GetDirectoryName(task.Source), "output_" + destinationFilename);
                    }
                }
                else if (userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Contains("{source_folder_name}") && !string.IsNullOrEmpty(task.Source))
                {
                    // Second Case: We have a Path, with "{source_folder}" in it, therefore we need to replace it with the folder name from the source.
                    string path = Path.GetDirectoryName(task.Source);
                    if (!string.IsNullOrEmpty(path))
                    {
                        string[] filesArray = path.Split(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);
                        string sourceFolder = filesArray[filesArray.Length - 1];

                        autoNamePath = Path.Combine(userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Replace("{source_folder_name}", sourceFolder), destinationFilename);
                    }
                }
                else if (!task.Destination.Contains(Path.DirectorySeparatorChar.ToString()))
                {
                    // Third case: If the destination box doesn't already contain a path, make one.
                    if (userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Trim() != string.Empty &&
                        userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath).Trim() != "Click 'Browse' to set the default location")
                    {
                        autoNamePath = Path.Combine(userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath), destinationFilename);
                    }
                    else // ...otherwise, output to the source directory
                        autoNamePath = null;
                }
                else // Otherwise, use the path that is already there.
                {
                    // Use the path and change the file extension to match the previous destination
                    autoNamePath = Path.Combine(Path.GetDirectoryName(task.Destination), destinationFilename);
                }
            }

            return autoNamePath;
        }
    }
}
