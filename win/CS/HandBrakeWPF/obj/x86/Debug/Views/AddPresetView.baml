   M S B A M L   `   `   `  ����  HandBrakeWPF&     HandBrakeWPF.Views.AddPresetView   5   	   \ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35R NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35D 9http://schemas.microsoft.com/winfx/2006/xaml/presentation    5      R NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml     5      Jmc;http://schemas.openxmlformats.org/markup-compatibility/2006     5      @d2http://schemas.microsoft.com/expression/blend/2008     5      U QCaliburn.Micro, Version=1.2.0.0, Culture=neutral, PublicKeyToken=8e5891231f2ed21f(calhttp://www.caliburnproject.org  6K   $	��350��5      $	��310��6     � Title  5      �� *Title+.��5
      � $��Stretch=�6   $��Stretch=�6)   ���	#FFF1F0EF � RowDefinitions 5   
   �� 5      $
��Auto �6   �� 5      $
��Auto �6   �� 5      $��* �6   �� 5      $
��Auto �6   .+�5   
   ��  �� Orientation$ 
Horizontal=�6   $��0��6.   
�������$��30��6O   $��0,0,0,10q�6[   .+�5      �� $��Images/Add16.png��6   $��10,0,5,0q�6.   $��16��6@   $��16��6K   $��Center=�6W   �� 5      $ Vertical=�6   $��Center=�60   .+�5      �� $��Add a Preset��6   $
��Bold"�60   5      5      � 5   
   $��	10,0,10,0q�6   $��1��6"    � ColumnDefinitions 5      �� 5      $	��100 �6#   �� 5      $��* �6#    5       �� 5!      $
��Auto �6    �� 5"      $
��Auto �6    �� 5#      $
��Auto �6    �� 5$      $
��Auto �6    �� 5%      $
��Auto �6    �� 5&      $
��Auto �6    �� 5'      $��* �6    �� 5(      $
��Auto �6    .+�5,      �� $��Preset Name:��6   $��0��6,   $��0��69   �� 5-      $	��150��6   $��0��6?   $��1��6L   $
��Left=�6]   ��6"   �� *Preset.Name+6]   �� 50      $��Picture Settings:��6   $
��Bold"�61   $��1��6D   $��0��6Q   $��0,20,0,0q�6a   �� 52      $��Use Picture Size:��6   $��2��62   $��0��6?   �� 53      $	��100��6   $��2��6}   $��1��6�   $
��Left=�6�   ��6#   �� *PictureSettingModes+ �� SelectedItem 6O   �� *SelectedPictureMode+6�   �� 55      $ 
Horizontal=�6   $��0,10,0,0q�62   $��3��6E   $��1��6R   .+�56      �� $��50��6   ��6%   �� *CustomWidth+�� 57      $��X��6   $
��Bold"�6%   $��	10,0,10,0q�67   �� 58      $��50��6   ��6%   �� *CustomHeight+59      �� 5;      $��Use Picture Filters��6   $��0,10,0,0q�6d   $��4��6w   $��1��6�    d� 	IsChecked 65   �� *Preset.UsePictureFilters+6�   �� 5>      $��Description:��6   $
��Bold"�6,   $��5��6>   $��0��6K   �� 5?      $��6��6:   $��2��6G   $��Stretch=�6\   ��6   �� *Preset.Description+6\   5@      � 5C   
   
�������$��3��6&   $��0,20,0,0q�63    5D      �� 5E      $��* �6#   �� 5F      $
��Auto �6#   �� 5G      $
��Auto �6#   .+�5J      �� $��Cancel��6   $	��8,2q�6&   $��0,5,10,5q�64   $��1��6F     Caliburn.Micro.Message   Attach$% [Event Click] = [Action Cancel]��6V   �� 5K      $	��Add��6   $	��8,2q�6#   $��0,5,10,5q�61   $��2��6C   $" [Event Click] = [Action Add]��6S   5L      5N      5O      