   M S B A M L   `   `   `  ����G  CHandBrakeWPF, Version=0.9.5.0, Culture=neutral, PublicKeyToken=null;"clr-namespace:HandBrakeWPF.HelpersHandBrakeWPF.Helpers  5   	   $    HandBrakeWPF.Views.OptionsView   5   	   \ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35R NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35D 9http://schemas.microsoft.com/winfx/2006/xaml/presentation    5   	   R NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml     5   	   U QCaliburn.Micro, Version=1.2.0.0, Culture=neutral, PublicKeyToken=8e5891231f2ed21f(calhttp://www.caliburnproject.org  6@   0Helpers"clr-namespace:HandBrakeWPF.Helpers  5   	   $	��620��6T   $	��550��6c   $	��620��6s   $	��550��6     � Title  6<   �� *Title+ � 	Resources 5       �� 
TargetType �� Property �� Value��%�  '��       '�� �     '�� 2    �� 5   
   " ��6   .�5      ��  � ��
 � ���6+   �� 5	       � ��$
 Bold"�6+   �� 5
       � ��$ 5,0.5q�6(   �� 5       � ��$
 11.5(�6)   �� 5       � ��$ Center=�62   5      �� 5   
   " ��6   .�5      ��  � ��$ 0,0,5,0q�6'   5      �� 5   
   " ��6   .�5      ��  � ��$ 0,0,0,5q�6'   5      5      .��5      ��  �� Orientation$ Vertical=�6   ���	#FFF1F0EF.+�5   
   �� $ 
Horizontal=�6   
�������$��50��6B   .+�5      �� $��Images/Preferences.png��6   $��10,0,5,0q�64   $��32��6F   $��32��6Q   $��Center=�6]   �� 5      $ Vertical=�6   $��Center=�60   .+�5       �� $��Options��6   $
��Bold"�6+   �� 5!      $:��4Preferences to control various features in HandBrake��6   5"      5#      �� 5&   
   $��10,10,10,10q�6   $	��410��6*   .@�5'      �� $��General��6   .��5(      �� $ Vertical=�6   .+�5)      � $��
10,10,0,10q�6    � ColumnDefinitions 5*      �� 5+      $	��100 �6/   �� 5,      $��* �6/   .+�5/      �� $��
At Launch:��6$   $��0��66   $
��Bold"�6F   �� 51      $ Vertical=�6%   $��1��6<   .+�52      �� $��Check for Updates��6'    d� 	IsChecked 6C   �� *CheckForUpdates+�� 53      -   5        $��checkForUpdateFrequency��53   '   $��25,0,0,5q�6�   $
��Left=�6�   $	��120��6�   ��6F   �� *CheckForUpdatesFrequencies+ �� SelectedIndex 6y   �� *CheckForUpdatesFrequency+6�   �� 55      $��Enable Tooltips��6'    6A   �� *EnableGuiTooltips+56      57      � 59      $��
10,10,0,10q�6    5:      �� 5;      $	��100 �6/   �� 5<      $��* �6/   .+�5?      �� $��
When Done:��6$   $��0��66   $
��Bold"�6F   �� 5A      $ Vertical=�6%   $��1��6<   .+�5B      �� -   5        $��whenDone��5B   '   $	��120��6�   $
��Left=�6�   ��67   �� *WhenDoneOptions+	 �� SelectedItem	 6_   �� *
WhenDone+6�   �� 5D      $ 
Horizontal=�6)   $��0,5,0,0q�6B   .+�5E   "   �� $!��Growl after Queue Completes��6+   $��0,0,5,0q�6x    6Q   �� *GrowlAfterEncode+6x   �� 5F   "   $"��Growl after Encode Completes��6+    6R   �� *GrowlAfterQueue+5G      �� 5I      $ 
Horizontal=�6)   $��0,5,0,0q�6B   .+�5J   "   �� $��Send file to:��6+   $��Center=�6C    6^   �� *SendFileAfterEncode+�� 5K   "   $��5,0,5,5q�6,   $��Center=�6=   ��6X   �� *
SendFileTo+�� 5L   "   $��Browse��6)     Caliburn.Micro.Message
   Attach$/
 )[Event Click] = [Action BrowseSendFileTo]��6:   $��55��6y   5M      �� 5O      $ 
Horizontal=�6)   $��0,5,0,0q�6B   .+�5P   "   �� $��Center=�6,   $��25,0,5,0q�6H   $��
Arguments:��6Z   �� 5Q   "   -   5        $��SendToArguments��5Q   *   $	��180��6\   ��6A   �� *	Arguments+6\   5R      5T      5U      5V      5W      �� 5Y      $��Output Files��6   .��5Z      �� $ Vertical=�6   .+�5[      � $��
10,10,0,10q�6    5\      �� 5]      $	��100 �6/   �� 5^      $��* �6/   .+�5a      �� $��Output Files:��6$   $��0��69   $
��Bold"�6I   �� 5c      $ Vertical=�6%   $��1��6<   .+�5d      �� $%��Automatically name output files��6'    6Q   �� *AutomaticallyNameFiles+�� 5f      $ 
Horizontal=�6)   $��0,5,0,0q�6B   .+�5g   "   �� $��Center=�6,   $��Arguments: ��6G   �� 5h   "   -   5        $��autoNameOutputPath��5h   *   $	��180��6i   ��6D   �� *AutoNameDefaultPath+6i   �� 5i   "   $��Browse��6)   $��5,0,0,0q�6:   $1
 +[Event Click] = [Action BrowseAutoNamePath]��6K   $��55��6�   5j      �� 5k      $I��CAvailable Options: {source_path} or {source_folder_name} (Not Both)��6(   �� 5m      $ 
Horizontal=�6)   $��0,10,0,0q�6B   .+�5n   "   �� $��Center=�6,   $��Format:��6G   �� 5o   "   -   5        $��autoNameFormat��5o   *   $	��180��6`   ��6@   �� *AutonameFormat+6`   5p      �� 5q      $;��5Available Options: {source} {title} {chapters} {date}��6(   �� 5s      $ 
Horizontal=�6)   $��0,15,0,0q�6B   .+�5t   "   �� $"��Remove underscores from name��6+    6R   �� *RemoveUnderscores+�� 5u   "   $��Change case to Title Case��6+   $��5,0,0,0q�6w    6O   �� *ChangeToTitleCase+6w   5v      �� 5x      $ 
Horizontal=�6)   $��0,15,0,0q�6B   .+�5y   "   �� $��Center=�6,   $��MP4 File Extension:��6G   �� 5z   "   -   5        $��mp4FileExtension��5z   +   $	��120��6C   $
��Left=�6�   ��6O   �� *Mp4ExtensionOptions+ 6{   �� *SelectedMp4Extension+6�   5{      5|      5}      5~      5      �� 5�      $��Preview��6   .��5�      �� $ Vertical=�6   .+�5�      � $��
10,10,0,10q�6    5�      �� 5�      $	��100 �6/   �� 5�      $��* �6/   .+�5�      �� $��	VLC Path:��6$   $��0��65   $
��Bold"�6E   �� 5�      $ Vertical=�6%   $��1��6<   .+�5�      �� $ 
Horizontal=�6)   .+�5�   "   �� $��Center=�6,   $��Path:��6G   �� 5�   "   -   5        $��vlcPath��5�   *   $	��180��6S   ��69   �� *	VLCPath+6S   �� 5�   "   $��Browse��6)   $,
 &[Event Click] = [Action BrowseVlcPath]��6:   $��5,0,0,0q�6v   $��55��6�   5�      �� 5�      $;��5This path is used for the video preview feature only.��6(   5�      5�      5�      5�      �� 5�      $��Audio and Subtitles��6   .��5�      �� $ Vertical=�6   .+�5�      �� $��Automatic Selection��6    $��0��6;   $��	10,10,0,0q�6K   $
��Bold"�6^   � 5�      $��	10,10,0,0q�6    5�      �� 5�      $	��140 �6/   �� 5�      $��* �6/   .+�5�      �� $��Preferred Language:��6$   $��0��6?   �� 5�      $ 
Horizontal=�6%   $��1��6>   .+�5�      �� $��Center=�6(   $��Audio: ��6D   $��10(�6S   �� 5�      -   5        $��primaryAudioLanguage��5�   '   $��Center=�6C   $	��120��6�   ��6_   �� *PreferredLanguages+	 6�   �� *SelectedPreferredLangauge+6�   �� 5�      $��Center=�6(   $��Subtitles: ��6D   $��10(�6W   $��10,0,0,0q�6e   �� 5�      -	   5        $��primarySubtitleLanguage��5�   '   $��Center=�6F   $	��120��6�   ��6b   �� *PreferredLanguages+	 6�   �� *#!SelectedPreferredSubtitleLangauge+6�   5�      5�      � 5�      $��	10,10,0,0q�6    5�      �� 5�      $	��100 �6/   �� 5�      $��* �6/   .+�5�      �� $��Additional Tracks:��6$   $��0��6>   �� 5�      $ 
Horizontal=�6%   $��1��6>   .+�5�      � $��
10,10,0,10q�6#    5�   "   �� 5�   &   $��* �67   �� 5�   &   $��* �67   �� 5�   &   $��* �67   �� 5�   &   $��* �67   .+�5�   "   �� $ Vertical=�6-   $��0��6D   .+�5�   &   �� $��Available Langauges��60   $��0,0,0,5q�6K   �� 5�   &   -
   5        $��availableLanguages��5�   .    �� SelectionMode$ Extended=�5�   .   $	��140��6G   $	��160��6S   ��5�   H   �� *AvailableLanguages+(   "HandBrakeWPF.Helpers.ListBoxHelper   SelectedItems 6s   �� *SelectedAvailableToMove+5�   b   5�   #   ]� 5�   "   $��1��6,   $��	10,0,10,0q�6<   .+�5�   &   �� $ Vertical=�61   $��Center=�6H   .+�5�   *   �� $��Center=�61   $��0,0,0,5q�6L   $/
 )[Event Click] = [Action LanguageMoveLeft]��6]   $��60��6�   .��5�   .   �� $��Images/arrow_right.png��64   $��24��6T   $��24��6_   5�   +   �� 5�   *   $��Center=�61   $��0,0,0,5q�6L   $0
 *[Event Click] = [Action LanguageMoveRight]��6]   $��60��6�   .��5�   .   �� $��Images/arrow_left.png��64   $��24��6S   $��24��6^   5�   +   �� 5�   *   $��Clear��61   $��Center=�6A   $��0,0,0,5q�6\   $/
 )[Event Click] = [Action LanguageClearAll]��6m   $��60��6�   5�   '   5�   #   �� 5�   "   $ Vertical=�6-   $��2��6D   .+�5�   &   �� $��Selected Langauges��60   $��0,0,0,5q�6K   �� 5�   &   -   5        $��selectedLangauges��5�   .   $ Extended=�5�   .   $	��140��6G   $	��160��6S   ��5�   G   �� *SelectedLangauges+ 6q   �� *SelectedLangaugesToMove+5�   S   5�   #   ]� 5�   "   $��3��6,   $��	10,0,10,0q�6<   .+�5�   &   �� $ Vertical=�61   $��Center=�6H   .+�5�   *   �� $��Center=�61   $��0,0,0,5q�6L   $-
 '[Event Click] = [Action LanguageMoveUp]��6]   .��5�   .   �� $��Images/arrow_up.png��64   $��16��6Q   $��16��6\   5�   +   �� 5�   *   $��Center=�61   $��0,0,0,5q�6L   $/
 )[Event Click] = [Action LanguageMoveDown]��6]   .��5�   .   �� $��Images/arrow_down.png��64   $��16��6S   $��16��6^   5�   +   5�   '   5�   #   5�      5�      5�      �� 5�      $��Add Additional Tracks��6   .��5�      �� $ Vertical=�6%   .+�5�      �� $ 
Horizontal=�6)   .+�5�   "   �� $��Audio:��6,   $��Center=�6:   $��15,0,5,0q�6U   $	��100��6g   �� 5�   "   -   5        $��autoAudioMode��5�   +   $	��120��6�   $��0,0,5,0q�6�   ��6@   �� *AddAudioModeOptions+ 6l   �� *SelectedAddAudioMode+6�   �� 5�   "   $+��%Add only one audio track per langauge��6+   $��Center=�6[    6v   �� *!AddOnlyOneAudioTrackPerLanguage+5�      �� 5�      $ 
Horizontal=�6)   $��0,5,0,0q�6B   .+�5�   "   �� $��	Subtitle:��6,   $��Center=�6=   $��15,0,5,0q�6X   $	��100��6j   �� 5�   "   -   5        $��autoSubtitleMode��5�   +   $	��120��6�   $��0,0,5,0q�6�   ��6C   �� *AddSubtitleModeOptions+ 6r   �� *SelectedAddSubtitleMode+6�   5�      �� 5�      $(��"Add Closed Captions when available��6'   $��	120,5,0,0q�6T    6g   �� *AddClosedCaptions+5�      5�      5�      5�      �� 5�      $��System and Logging��6   .��5�      �� $ Vertical=�6   .+�5�      � $��
10,10,0,10q�6    5�      �� 5�      $	��100 �6/   �� 5�      $��* �6/   .+�5     �� $
��CLI:��6$   $��0��60   $
��Bold"�6@   �� 5     $ 
Horizontal=�6%   $��1��6>   .+�5     �� $��Priority Level:��6(   $��Center=�6?   �� 5     -   5        $��processPriorityLevel��5  '   $	��120��6�   ��6C   �� *PriorityLevelOptions+	 6p   �� *SelectedPriority+6�   5     5     � 5     $��
10,10,0,10q�6    5	     �� 5
     $	��100 �6/   �� 5     $��* �6/   .+�5     �� $��System:��6$   $��0��63   $
��Bold"�6C   �� 5     $ 
Horizontal=�6%   $��1��6>   .+�5     �� $5��/Prevent the system from sleeping while encoding��6'    6a   �� *PreventSleep+5     5     � 5     $��
10,10,0,10q�6    5     �� 5     $	��100 �6/   �� 5     $��* �6/   .+�5     �� $��Logs:��6$   $��0��61   $
��Bold"�6A   �� 5     $ Vertical=�6%   $��1��6<   .+�5     �� $ 
Horizontal=�6)   $��1��6B   .+�5  "   �� $��Log Verbosity Level:��6,   $��Center=�6H   �� 5  "   -   5        $��logVerbosityLevel��5  +   $	��120��6�   ��6D   �� *LogVerbosityOptions+	 6p   �� *SelectedVerbosity+6�   5     �� 5!     $T��NPut a copy of individual encode logs in the same location as the encoded video��6'    6�   �� *CopyLogToEncodeDirectory+�� 5"     $D��>Put a copy of individual encode logs in a specified location: ��6'    6p   �� *CopyLogToSepcficedLocation+�� 5#     $ 
Horizontal=�6)   $��0,10,0,0q�6B   $��1��6T   .+�5$  "   �� $��	Log Path:��6,   $��Center=�6=   �� 5%  "   $	��120��6*   ��66   �� *LogDirectory+�� 5&  "   $��Browse��6)   $��5,0,0,0q�6:   $,
 &[Event Click] = [Action BrowseLogPath]��6K   5'     �� 5(     $ 
Horizontal=�6)   $��0,10,0,0q�6B   $��1��6T   .+�5)  "   �� $��View Log Directory��6)   $/
 )[Event Click] = [Action ViewLogDirectory]��6F   $��0,0,5,0q�6�   �� 5*  "   $��Clear Log History��6)   $.
 ([Event Click] = [Action ClearLogHistory]��6E   5+     �� 5-     $)��#Clear Log files older than 30 days ��6'   $��0,5,0,0q�6U    6f   �� *ClearOldOlgs+5.     50     51     52     �� 54     $��Advanced��6   .��55     �� $ Vertical=�6   .+�56     � $��
10,10,0,10q�6    57     �� 58     $	��100 �6/   �� 59     $��* �6/   .+�5<     �� $
��GUI:��6$   $��0��60   $
��Bold"�6@   �� 5=     $ Vertical=�6%   $��1��6<   .+�5>     �� $0��*Minimize to system tray (Requires Restart)��6'    6\   �� *MinimiseToTray+�� 5?     $=��7Display status messages from tray icon (balloon popups)��6'    6i   �� *DisplayStatusMessagesTrayIcon+�� 5@     $2��,Enable 'Query Editor' tab (Requires Restart)��6'    6^   �� *EnableQueryEditor+�� 5A     $<��6Prompt when a manual query does not match GUI settings��6'   $��10,0,0,5q�6h    6z   �� *PromptOnDifferentQuery+�� 5B     $1��+Disable built-in preset update notification��6'    6]   �� *&$DisablePresetUpdateCheckNotification+�� 5C     $E��?Show CLI window (Allows you to cleanly exit encode with ctrl-c)��6'    6q   �� *ShowCliWindow+�� 5D     $ 
Horizontal=�6)   $��0,10,0,0q�6B   .+�5E  "   �� $)��#Number of picture previews to scan:��6,   $��Center=�6W   $	��250��6r   �� 5F  "   -   5        $��numberOfPreviews��5F  +   $	��120��6�   ��6C   �� *PreviewPicturesToScan+	 6q   �� *SelectedPreviewCount+6�   5G     �� 5H     $ 
Horizontal=�6)   $��0,5,0,0q�6B   .+�5I  "   �� $0��*Minimum length of title to scan (seconds):��6,   $��Center=�6^   $	��250��6y   �� 5J  "   -   5        $��MinTitleLength��5J  *   $	��120��6\   ��6@   �� *	MinLength+6\   5L     5N     5O     � 5Q     $��	10,0,0,10q�6    5R     �� 5S     $	��100 �6/   �� 5T     $��* �6/   .+�5V     �� $��x264:��6$   $��0��61   $
��Bold"�6A   �� 5W     $ 
Horizontal=�6%   $��1��6>   .+�5X     �� $.��(Constant quality fractional granularity:��6(   $��Center=�6X   $	��250��6s   �� 5Y     -   5        $��x264Granularity��5Y  '   $	��120��6�   ��6>   �� *ConstantQualityGranularity+	 6q   �� *SelectedGranulairty+6�   5Z     5[     � 5]     $��	10,0,0,10q�6    5^     �� 5_     $	��100 �6/   �� 5`     $��* �6/   .+�5c     �� $
��DVD:��6$   $��0��60   $
��Bold"�6@   �� 5e     $ Vertical=�6%   $��1��6<   .+�5f     �� $:��4Disable LibDVDNav. (libdvdread will be used instead)��6'    6f   �� *DisableLibdvdNav+5g     5i     5j     5k     5l     �� 5n  
   $��Close��6    �� 	IsDefault . $$
 [Event Click] = [Action Close]��62   $��Right=�5o     $
��10,2q�61   $��0,0,10,0q�6@   5r     5s     