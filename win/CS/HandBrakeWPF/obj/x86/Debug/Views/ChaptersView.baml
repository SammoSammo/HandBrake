   M S B A M L   `   `   `  ����  HandBrakeWPF%    HandBrakeWPF.Views.ChaptersView   5      \ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35R NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35D 9http://schemas.microsoft.com/winfx/2006/xaml/presentation    5      R NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml     5      Jmc;http://schemas.openxmlformats.org/markup-compatibility/2006     5      @d2http://schemas.microsoft.com/expression/blend/2008     5      U QCaliburn.Micro, Version=1.2.0.0, Culture=neutral, PublicKeyToken=8e5891231f2ed21f(calhttp://www.caliburnproject.org  6K   .��5      �   � RowDefinitions  5   
   �� 5	      $
��Auto �6   �� 5
      $
��Auto �6   �� 5      $��* �6   .+�5   
   �� $��Chapter Markers��6   $
��Bold"�6+   $��10,5,0,0q�6=   $��0��6O   6_   � 5   
   $��1��6   $��	10,5,10,0q�6    � ColumnDefinitions 5      �� 5      $
��Auto �6#   �� 5      $��* �6#   �� 5      $
��Auto �6#   �� 5      $
��Auto �6#   .+�5      �� $��Create chapter markers��6   $��0��6d   $��Center=�6t    d� 	IsChecked 68   �� *IncludeChapterMarkers+6t   �� 5      -   5        $��import��5   &   $��Import��6   $��2��64   $��75��6D   $��0,0,10,0q�6O     Caliburn.Micro.Message   Attach$% [Event Click] = [Action Import]��6a   �� 5      -   5        $��export��5   &   $��Export��6   $��3��64   $��75��6D   $% [Event Click] = [Action Export]��6O   5      &   System.Windows.Controls.DataGrid  5   
   $��2��6   $��10q�6    $��Stretch=�5      $��Stretch=�6/      AutoGenerateColumns .     CanUserSortColumns .     CanUserReorderColumns .     CanUserResizeColumns .     CanUserResizeRows .  ��5   ,   �� *
Chapters+	   Columns	 5      0  *System.Windows.Controls.DataGridTextColumn  5       ,  &System.Windows.Controls.DataGridColumn
   Header$
 Chapter Number��6%      Width5  /System.Windows.Controls.DataGridLengthConverter$	 150 6=      
IsReadOnly . 1  +System.Windows.Controls.DataGridBoundColumn   Binding 6I   �� *ChapterNumber+6k     5!      $
 Chapter Name��6%   $ * 6<    .   6F   �� *ChapterName+6f   5#      5'      5(      