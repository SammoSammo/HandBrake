   M S B A M L   `   `   `  ����\  XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35X@clr-namespace:System.Windows.Data;assembly=PresentationFrameworkSystem.Windows.Data  5   @   G CHandBrakeWPF, Version=0.9.5.0, Culture=neutral, PublicKeyToken=nullA%clr-namespace:HandBrakeWPF.ConvertersHandBrakeWPF.Converters 5   	   U QCaliburn.Micro, Version=1.2.0.0, Culture=neutral, PublicKeyToken=8e5891231f2ed21fG4clr-namespace:Caliburn.Micro;assembly=Caliburn.MicroCaliburn.Micro 5   	   !   HandBrakeWPF.Views.MainView   5   	   W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35R NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35D 9http://schemas.microsoft.com/winfx/2006/xaml/presentation     5   	   R NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml      5   	   QData@clr-namespace:System.Windows.Data;assembly=PresentationFramework      6@   <
Converters%clr-namespace:HandBrakeWPF.Converters      5   	   FMicro4clr-namespace:Caliburn.Micro;assembly=Caliburn.Micro      5   	   $	��655��5   1   $
��1015��6>   $��11(�6K   ���	#FFF0F0F0  � Title  6	   ��  �� Path$ WindowTitle� � 	Resources 5       �� 
TargetType �� Property �� Value   textBlockOrangeStyle:  4HandBrakeWPF.Converters.BooleanToVisibilityConverter  boolToVisConverter��%�  '��       &	  �     &	 �    �� 5	   
   " ��6   .�5
      ��  � ��
 � ���6+   �� 5       � ��$
 Bold"�6+   �� 5       � ��$	 5,1q�6(   �� 5       � ��$
 11.5(�6)   �� 5       � ��$ Center=�62   5      �� 5   
   " ��6-   .�5      ��  � ��
 � ���6+   �� 5       � ��$
 Bold"�6+   �� 5       �@��Padding$	 5,5q�6(   5        5   
   62   5      .��5      � .+�5   
   ��  �� Orientation$ Vertical=�6   $��Stretch=�6,   $��Stretch=�6H   .+�5      �� $ Vertical=�6   $��Stretch=�60   $��Stretch=�6L   .+�5      o� $��23��6   $	��Top=�6#   $��Stretch=�6;   .@�5       m� $
��File��6   .@�5!      m� $��Cancel Scan��6#     Caliburn.Micro.Message   Attach$) #[Event Click] = [Action CancelScan]��68   �� 5"      m� 5#      $
��Exit��6#   $. ([Event Click] = [Action ExitApplication]��61   5$      m� 5&      $��Tools��6   .@�5'      m� $��
Show Queue��6#   $. ([Event Click] = [Action OpenQueueWindow]��67   m� 5(      $��Activity Window��6#   $, &[Event Click] = [Action OpenLogWindow]��6<   �� 5)      m� 5*      $��Options��6#   $0 *[Event Click] = [Action OpenOptionsWindow]��64   5+      m� 5-      $
��Help��6   .@�5.      m� $��HandBrake User Guide��6#   $) #[Event Click] = [Action LaunchHelp]��6A   �� 5/      m� 50      $��Check for Updates��6#   $. ([Event Click] = [Action CheckForUpdates]��6>   �� 51      m� 52      $��About...��6#   $3 -[Event Click] = [Action OpenAboutApplication]��65   53      54      c� 57      -   5        $��mainToolBar��57      $��Stretch=�6-   $��Stretch=�6K    J� SnapsToDevicePixels .  .@�59      o� 
������ .@�5:      m� ��5;      �� 5<   "   $ 
Horizontal=�6-   .+�5=   &   �� $��Images/Movies.png��6,   $��32��6G   $��32��6S   �� 5>   &   $��Source��6,   $��8,0,0,0q�6>   $��Center=�6O   5?   #   .@�5B      m� $) #[Event Click] = [Action FolderScan]��6'   ��5C   "   �� 5D   &   $ 
Horizontal=�61   .+�5E   *   �� $��Images/folder.png��60   $��20��6K   $��20��6W   �� 5F   *   $��Open Folder��60   $��8,0,0,0q�6G   $��Center=�6X   5G   '   5I      m� 5J      $' ![Event Click] = [Action FileScan]��6'   ��5K   "   �� 5L   &   $ 
Horizontal=�61   .+�5M   *   �� $��Images/Movies.png��60   $��20��6K   $��20��6W   �� 5N   *   $��	Open File��60   $��8,0,0,0q�6E   $��Center=�6V   5O   '   5Q      5R      5S      �� 5U      �� 5W      -   5        $��Start��5W      $* $[Event Click] = [Action StartEncode]��6*   y�6f   �� *
IsEncoding+	 �� 	Converter#	 [ 
 �� ConverterParameter$

 true��.��5X      �� $ 
Horizontal=�6%   .+�5Y      �� $��Images/Play.png��6$   $��32��6=   $��32��6I   �� 5Z      $��Start��6$   $��8,0,0,0q�65   $��Center=�6F   5[      5\      �� 5^      -   5        $
��Stop��5^      $) #[Event Click] = [Action StopEncode]��6)   y�6d   �� *
IsEncoding+#	 [ $
 false��.��5_      �� $ 
Horizontal=�6%   .+�5`      �� $��Images/stop.png��6$   $��32��6=   $��32��6I   �� 5a      $
��Stop��6$   $��8,0,0,0q�64   $��Center=�6E   5b      5c      �� 5e      -   5        $��
AddToQueue��5e      $) #[Event Click] = [Action AddToQueue]��60   .��5f      �� $ 
Horizontal=�6%   .+�5g      �� $��Images/AddToQueue.png��6$   $��32��6C   $��32��6O   �� 5h      $��Add To Queue��6$   $��8,0,0,0q�6<   $��Center=�6M   5i      5j      �� 5l      -   5        $��	ShowQueue��5l      $. ([Event Click] = [Action OpenQueueWindow]��6/   .��5m      �� $ 
Horizontal=�6%   .+�5n      �� $��Images/Queue.png��6$   $��32��6>   $��32��6J   �� 5o      $��
Show Queue��6$   $��8,0,0,0q�6:   $��Center=�6K   5p      5q      �� 5s      �� 5u      -   5        $��Preview��5u      $0 *[Event Click] = [Action OpenPreviewWindow]��6,   .��5v      �� $ 
Horizontal=�6%   .+�5w      �� $��Images/window.png��6$   $��32��6?   $��32��6K   �� 5x      $��Preview��6$   $��8,0,0,0q�67   $��Center=�6H   5y      5z      �� 5|      -   5        $��ActivityWindow��5|      $, &[Event Click] = [Action OpenLogWindow]��63   .��5}      �� $ 
Horizontal=�6%   .+�5~      �� $��Images/ActivityWindow.png��6$   $��32��6G   $��32��6S   �� 5      $��Activity Window��6$   $��8,0,0,0q�6?   $��Center=�6P   5�      5�      5�      5�      �� 5�      $ 
Horizontal=�6   .+�5�      �� $ Vertical=�6   .+�5�      �� $��	10,5,10,5q�6!   $	��725��64   $	��725��6C   $
��Left=�6O   .+�5�      �� $ 
Horizontal=�6%   .+�5�      �� $��Source��6$   $
��Bold"�65   �� 5�      ��6$   �� $ SourceLabel�5�      �� 5�      $ 
Horizontal=�6%   .+�5�      �� $��Title��6$   $��8,0,0,0q�64   �� 5�      -   5        $��Titles��5�   '   $��8,0,0,0q�65   $	��100��6F   ��6U   �� *ScannedSource.Titles+ �� SelectedItem 6�   �� $ SelectedTitle��� 5�      $��Angle��6$   $��8,0,0,0q�64   �� 5�      -	   5        $��Angles��5�   '   $��8,0,0,0q�65   $��60��6F   ��6T   �� *Angles+ 6t   �� *SelectedAngle+�� 5�      -
   5        $��PointToPointMode��5�   '   $��8,0,0,0q�6?   $��80��6P   ��6^   �� *	RangeMode+ 6�   �� *SelectedPointToPoint+�� 5�      -   5        $��
StartPoint��5�   '   $��8,0,0,0q�69   $��60��6J   ��6X   �� *StartEndRangeItems+ 6�   �� *SelectedStartPoint+�� 5�      $��through��6$   $��8,0,0,0q�66   �� 5�      -   5        $��EndPoint��5�   '   $��8,0,0,0q�67   $��60��6H   ��6V   �� *StartEndRangeItems+ 6�   �� *SelectedEndPoint+�� 5�      $��Duration��6$   $��8,0,0,0q�67   �� 5�      $��8,0,0,0q�6A   ��6$   �� *
Duration+6A   5�      5�      �� 5�      $��	10,5,10,5q�6!   $	��725��64   $	��725��6C   $
��Left=�6O   .+�5�      �� $��Destination��6    $
��Bold"�66   �� 5�      $ 
Horizontal=�6%   .+�5�      �� $
��File��6$   $��8,0,0,0q�63   �� 5�      -   5        $��Destination��5�   &   $��8,0,0,0q�69   $	��600��6J   ��6V   �� *CurrentTask.Destination+�� 5�      -   5        $��DestinationBrowser��5�   %   $��8,0,0,0q�6?   $��8,0,8,0q�6P   $��Browse��6b   $0 *[Event Click] = [Action BrowseDestination]��6s   5�      5�      �� 5�      $��	10,5,10,5q�6!   $	��725��64   $	��725��6C   $
��Left=�6O   .+�5�      �� $$��Output Settings (Preset: None)��6    $
��Bold"�6I   �� 5�      $ 
Horizontal=�6%   .+�5�      �� $��	Container��6$   $��8,0,0,0q�68   �� 5�      -   5        $��	Container��5�   '   $��8,0,0,0q�68   $	��100��6I   ��6X   �� *OutputFormats+ 6~   �� *SelectedOutputFormat+�� 5�      -   5        $��LargeFileMp4��5�   '   $��Large File Size��6;   $��Center=�5�   �   $��8,0,0,0q�6�    d� 	IsChecked 5�   U   �� $ CurrentTask.LargeFile�y�5�   '   �� *IsMkv+#	 [ $

 true��6�   �� 5�      -   5        $��WebOptimized��5�   '   $��Web Optimized��6;   $��Center=�5�   �   $��8,0,0,0q�6�    5�   S   �� $ CurrentTask.OptimizeMP4�y�5�   '   �� *IsMkv+#	 [ $

 true��6�   �� 5�      -   5        $��iPod5G��5�   '   $��iPod 5G Support��65   $��Center=�5�   �   $��8,0,0,0q�6�    5�   O   �� $ CurrentTask.IPod5GSupport�y�5�   '   �� *IsMkv+#	 [ $

 true��6�   5�      5�      �� 5�      -   5        $��
tabControl��5�   �   $
��Left=�6!   $��Stretch=�6<   $	��725��6X   $	��330��6d   $��10,10,10,10q�6q   .@�5�      �� -   5        $��
pictureTab��5�   3   $��Picture��6"   .��5�      �� -   5         ��Name$ PictureSettingsViewModel��5�   -   6R   5�      �� 5�      -   5        $��
filtersTab��5�   9   $��Video Filters��6"   .��5�      �� -   5        $ FiltersViewModel��5�   -   6J   5�      �� 5�      -   5        $��videoTab��5�   1   $��Video��6"   .��5�      �� -   5        $ VideoViewModel��5�   -   6H   5�      �� 5�      -   5        $��audioTab��5�   1   $��Audio��6"   .��5�      �� -   5        $ AudioViewModel��5�   -   6H   5�      �� 5�      -   5        $��subtitlesTab��5�   5   $��	Subtitles��6"   .��5�      �� -   5        $ SubtitleViewModel��5�   -   6J   5�      �� 5�      -   5        $��chaptersTab��5�   4   $��Chapters��6"   .��5�      �� -   5        $ ChaptersViewModel��5�   -   6J   5�      �� 5�      -    5        $��advancedTab��5�   4   $��Advanced��6"   .��5�      �� -!   5        $ AdvancedViewModel��5�   -   6J   5�      5�      5�      �� 5�      $��5,5,5,5q�6   $ Vertical=�6.   .+�5�      �� $��Presets��6   $��Stretch=�60   $��Stretch=�6L   .��5�      �� $ Vertical=�6%   .+�5�      R� -"   5        $	��240��5�   G   $	��460��6S   ��6'   �� *	Presets+5�      c� 5�      -#   5        $��presetsToolBar��5�   &   $��Stretch=�6<   $��Stretch=�6Z    � UseLayoutRounding .  
������  .  .@�5�   "   �� $( "[Event Click] = [Action PresetAdd]��6)   ��5�   &   �� 5�   *   $ 
Horizontal=�65   .+�5�   .   �� $��Images/Add16.png��64   $��16��6N   $��16��6Z   �� 5�   .   $	��Add��68   #��[  6C   $��2,0,0,0q�6r   $��Center=�6�   5�   +   5�   #   �� 5�   "   $+ %[Event Click] = [Action PresetRemove]��6)   ��5�   &   �� 5�   *   $ 
Horizontal=�65   .+�5�   .   �� $��Images/Close.png��64   $��16��6N   $��16��6Z   �� 5�   .   $��Remove��68   #��[  6F   $��2,0,0,0q�6u   $��Center=�6�   5�   +   5�   #   o� 5�   "   
������ .@�5�   &   m�  c� OverflowMode$ Never=�6/   ��5�   *   �� 5�   .   $ 
Horizontal=�69   .+�5�   2   �� $��Images/Options24.png��68   $��16��6V   $��16��6b   �� 5�   2   $��Options��6<   #��[  6K   $��2,0,0,0q�6y   $��Center=�6�   5�   /   .@�5�   *   m� $��Set Default��63   $/ )[Event Click] = [Action PresetSetDefault]��6H   �� 5�   *   m� 5�   *   $��Import��63   $+ %[Event Click] = [Action PresetImport]��6C   m� 5�   *   $��Export��63   $+ %[Event Click] = [Action PresetExport]��6C   �� 5   *   m� 5  *   $��Reset Built-in Presets��63   $* $[Event Click] = [Action PresetReset]��6S   5  '   5  #   5     5     5     5	     5     �� 5     $��6��6   $��28��6%   $��2��61   $��Bottom=�6E   .@�5     �� $��10(�6D   $��Center=�6R   ��6   �� $ ProgramStatusLabel�6R   5     5     5     5     