   M S B A M L   `   `   `  ����  HandBrakeWPF"    HandBrakeWPF.Views.VideoView   5      \ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35R NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35D 9http://schemas.microsoft.com/winfx/2006/xaml/presentation    5      R NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml     5      Jmc;http://schemas.openxmlformats.org/markup-compatibility/2006     5      @d2http://schemas.microsoft.com/expression/blend/2008     5      .��5      � $��10,5,0,0q�6     � RowDefinitions  5   
   �� 5	      $
��Auto �6   �� 5
      $��* �6   .+�5   
   � $��1��6    � ColumnDefinitions 5      �� 5      $��* �6#   �� 5      $��* �6#   .+�5      ��  �� Orientation$ Vertical=�6   $��0��60   .+�5      �� $��Video��6   $
��Bold"�6)   $��0,0,0,10q�6;   �� 5      $ 
Horizontal=�6   $��0,0,0,10q�66   .+�5      �� $��Video Codec:��6    $	��100��64   �� 5      $	��120��6   5      �� 5      $ 
Horizontal=�6   .+�5      �� $��Franerate (FPS):��6    $	��100��68   �� 5      $ Vertical=�6!   .+�5       �� $	��120��6#   	� 5!      $��Constant Framerate��6&   	� 5"      $��Variable Framerate��6&   	� 5#      $��Peak Framerate��6&   5$      5%      5&      �� 5)      $ Vertical=�6   $��1��60   .+�5*      �� $��Quality��6   $
��Bold"�6+   $��0,0,0,10q�6=   �� 5,      $ 
Horizontal=�6   $��0,0,0,10q�66   .+�5-      	� $��Constant Quality:��6"   $��0,0,10,0q�6>   �� 5.      $��0��6    $��25��6)   �� 5/      $��RF��6    $
��Bold"�6*   50      �� 52      $	��240��6   $��0,0,0,20q�6%   �� 54      $ 
Horizontal=�6   $��0,0,0,10q�66   .+�55      	� $��Avg Bitrate (kbps):��6"   $��0,0,10,0q�6@   �� 56      $��75��6   57      �� 59      $ 
Horizontal=�6   $��30,0,0,0q�66   .+�5:      �� $��2-Pass Encoding��6   $��0,0,10,0q�69   �� 5;      $��Turbo first pass��6   5<      5>      5A      5C      5D      