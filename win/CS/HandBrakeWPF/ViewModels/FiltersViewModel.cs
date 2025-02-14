﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="FiltersViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Filters View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System.Collections.Generic;
    using System.ComponentModel.Composition;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.Interop.Model.Encoding;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Filters View Model
    /// </summary>
    [Export(typeof(IFiltersViewModel))]
    public class FiltersViewModel : ViewModelBase, IFiltersViewModel
    {
        #region Constants and Fields

        /// <summary>
        /// Backing field for the deblock value
        /// </summary>
        private int deblockValue;

        /// <summary>
        /// Backing field for the selected deinterlace value
        /// </summary>
        private Deinterlace selectedDeInterlace;

        /// <summary>
        /// Backing field for the selected decomb value
        /// </summary>
        private Decomb selectedDecomb;

        /// <summary>
        /// Backing field for the selected denoise value
        /// </summary>
        private Denoise selectedDenoise;

        /// <summary>
        /// Backing field for the selected detelecine value
        /// </summary>
        private Detelecine selectedDetelecine;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="FiltersViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public FiltersViewModel(IWindowManager windowManager, IUserSettingService userSettingService)
        {
            this.CurrentTask = new EncodeTask();
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets CurrentTask.
        /// </summary>
        public EncodeTask CurrentTask { get; private set; }

        /// <summary>
        /// Gets DeInterlaceOptions.
        /// </summary>
        public IEnumerable<string> DeInterlaceOptions
        {
            get
            {
                return EnumHelper<Deinterlace>.GetEnumDisplayValues(typeof(Deinterlace));
            }
        }

        /// <summary>
        /// Gets DeblockText.
        /// </summary>
        public string DeblockText
        {
            get
            {
                return this.DeblockValue == 4 ? "Off" : this.DeblockValue.ToString();
            }
        }

        /// <summary>
        /// Gets or sets DeblockValue.
        /// </summary>
        public int DeblockValue
        {
            get
            {
                return this.deblockValue;
            }
            set
            {
                this.deblockValue = value;
                this.NotifyOfPropertyChange("DeblockValue");
                this.NotifyOfPropertyChange("DeblockText");
            }
        }

        /// <summary>
        /// Gets DecombOptions.
        /// </summary>
        public IEnumerable<string> DecombOptions
        {
            get
            {
                return EnumHelper<Decomb>.GetEnumDisplayValues(typeof(Decomb));
            }
        }

        /// <summary>
        /// Gets DenoiseOptions.
        /// </summary>
        public IEnumerable<string> DenoiseOptions
        {
            get
            {
                return EnumHelper<Denoise>.GetEnumDisplayValues(typeof(Denoise));
            }
        }

        /// <summary>
        /// Gets DetelecineOptions.
        /// </summary>
        public IEnumerable<string> DetelecineOptions
        {
            get
            {
                return EnumHelper<Detelecine>.GetEnumDisplayValues(typeof(Detelecine));
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether Grayscale.
        /// </summary>
        public bool Grayscale { get; set; }

        /// <summary>
        /// Gets or sets SelectedDeInterlace.
        /// </summary>
        public string SelectedDeInterlace
        {
            get
            {
                return EnumHelper<Deinterlace>.GetDisplay(this.selectedDeInterlace);
            }

            set
            {
                this.selectedDeInterlace = EnumHelper<Deinterlace>.GetValue(value);
                if (this.selectedDeInterlace != Deinterlace.Off)
                {
                    this.SelectedDecomb = EnumHelper<Decomb>.GetDisplay(Decomb.Off);
                }
                this.NotifyOfPropertyChange("SelectedDeInterlace");

                // Show / Hide the Custom Control
                this.ShowDeinterlaceCustom = this.selectedDeInterlace == Deinterlace.Custom;
                this.NotifyOfPropertyChange("ShowDeinterlaceCustom");
            }
        }

        /// <summary>
        /// Gets or sets SelectedDecomb.
        /// </summary>
        public string SelectedDecomb
        {
            get
            {
                return EnumHelper<Decomb>.GetDisplay(this.selectedDecomb);
            }

            set
            {
                this.selectedDecomb = EnumHelper<Decomb>.GetValue(value);
                if (this.selectedDecomb != Decomb.Off)
                {
                    this.SelectedDeInterlace = EnumHelper<Deinterlace>.GetDisplay(Deinterlace.Off);
                }

                this.NotifyOfPropertyChange("SelectedDecomb");

                // Show / Hide the Custom Control
                this.ShowDecombCustom = this.selectedDecomb == Decomb.Custom;
                this.NotifyOfPropertyChange("ShowDecombCustom");
            }
        }

        /// <summary>
        /// Gets or sets SelectedDenoise.
        /// </summary>
        public string SelectedDenoise
        {
            get
            {
                return EnumHelper<Denoise>.GetDisplay(this.selectedDenoise);
            }

            set
            {
                this.selectedDenoise = EnumHelper<Denoise>.GetValue(value);
                this.NotifyOfPropertyChange("SelectedDenoise");

                // Show / Hide the Custom Control
                this.ShowDenoiseCustom = this.selectedDenoise == Denoise.Custom;
                this.NotifyOfPropertyChange("ShowDenoiseCustom");
            }
        }

        /// <summary>
        /// Gets or sets SelectedDetelecine.
        /// </summary>
        public string SelectedDetelecine
        {
            get
            {
                return EnumHelper<Detelecine>.GetDisplay(this.selectedDetelecine);
            }

            set
            {
                this.selectedDetelecine = EnumHelper<Detelecine>.GetValue(value);
                this.NotifyOfPropertyChange("SelectedDetelecine");

                // Show / Hide the Custom Control
                this.ShowDetelecineCustom = this.selectedDetelecine == Detelecine.Custom;
                this.NotifyOfPropertyChange("ShowDetelecineCustom");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether ShowDecombCustom.
        /// </summary>
        public bool ShowDecombCustom { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether ShowDelelecineCustom.
        /// </summary>
        public bool ShowDeinterlaceCustom { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether ShowDenoiseCustom.
        /// </summary>
        public bool ShowDenoiseCustom { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether ShowDetelecineCustom.
        /// </summary>
        public bool ShowDetelecineCustom { get; set; }

        #endregion
    }
}