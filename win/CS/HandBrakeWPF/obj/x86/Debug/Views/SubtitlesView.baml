   M S B A M L   `   `   `  ����V  REagleBoost.Wpf.Presentation, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null�eclr-namespace:EagleBoost.Wpf.Presentation.Controls.NumericUpDown;assembly=EagleBoost.Wpf.Presentation2EagleBoost.Wpf.Presentation.Controls.NumericUpDown  5       HandBrakeWPF&    HandBrakeWPF.Views.SubtitlesView   5      \ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35R NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35D 9http://schemas.microsoft.com/winfx/2006/xaml/presentation    5      R NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml     5      Jmc;http://schemas.openxmlformats.org/markup-compatibility/2006     5      @d2http://schemas.microsoft.com/expression/blend/2008     5      U QCaliburn.Micro, Version=1.2.0.0, Culture=neutral, PublicKeyToken=8e5891231f2ed21f(calhttp://www.caliburnproject.org  6K   c _System.Windows.Interactivity, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35Bi:http://schemas.microsoft.com/expression/2010/interactivity  5      yNumericUpDowneclr-namespace:EagleBoost.Wpf.Presentation.Controls.NumericUpDown;assembly=EagleBoost.Wpf.Presentation  5      .��5	      �   � RowDefinitions  5
   
   �� 5      $
��Auto �6   �� 5      $
��Auto �6   �� 5      $��* �6   .+�5   
   �� $��	Subtitles��6   $
��Bold"�6%   $��10,5,0,0q�67   $��0��6I   6Y   � 5   
   $��1��6   $��	10,5,10,0q�6    � ColumnDefinitions 5      �� 5      $
��Auto �6#   �� 5      $
��Auto �6#   �� 5      $
��Auto �6#   .+�5      �� -   5        $��AddTrack��5   #   $	��Add��6   $��0��63   $��75��6C   $��0,0,10,0q�6N     Caliburn.Micro.Message   Attach$" [Event Click] = [Action Add]��6`   �� 5      -   5        $��RemoveTrack��5   &   $��Remove��6   $��1��69   $��75��6I   $��0,0,10,0q�6T   $% [Event Click] = [Action Remove]��6f   �� 5      $��
Import SRT��6   $��2��6*   $��75��6:   $% [Event Click] = [Action Import]��6E   5      �� 5   
   $��2��6    �� SelectionMode$ Extended=�5      
�������$��10,10,10,10q�6B   ��5      �� *SubtitleTracks+��5      �� 5        �� 
TargetType" ��6   .�5!      ��  �� Property � �� �� Value$ Stretch=�6C   6U   5"      ��5%      �� 5&      .R�5(      � $��Stretch=�6    5)      �� 5*      $
��Auto �6/   �� 5+      $��* �6/   �� 5,      $
��Auto �6/   .+�50      �� $��Images/Movies.png��6    $��16��6;   $��16��6F   $��0��6R   $��	10,0,10,0q�6b   � 53      $��1��6   $��Stretch=�6/    54      �� 55   "   $
��Auto �63   �� 56   "   $
��Auto �63   �� 57   "   $
��Auto �63   �� 58   "   $
��Auto �63   �� 59   "   $
��Auto �63   �� 5:   "   $
��Auto �63   �� 5;   "   $
��Auto �63   �� 5<   "   $
��Auto �63   �� 5=   "   $
��Auto �63   �� 5>   "   $
��Auto �63   �� 5?   "   $
��Auto �63     5B      �� 5C   "   $��28 �60   �� 5D   "   $��28 �60   .+�5I      �� $��Source��6(   $
��Bold"�66   $��0��6H   $��Center=�6X   �� 5J      $	��100��6'   $��1��63   $��5,0,5,0q�6C   $��22��6T   �� 5K      $��Forced Only��6(   $
��Bold"�6;   $��2��6M   $��Center=�6]   �� 5L      $��3��6'   $��Center=�67   $��5,0,5,0q�6R   �� 5M      $��Burn In��6(   $
��Bold"�67   $��4��6I   $��Center=�6Y   �� 5N      $��5��6'   $��Center=�67   $��5,0,5,0q�6R   �� 5O      $��Default��6(   $
��Bold"�67   $��6��6I   $��Center=�6Y   �� 5P      $��7��6'   $��Center=�67   $��5,0,5,0q�6R   �� 5S      $��Language��6(   $
��Bold"�68   $��0��6J   $��1��6Z   $��Center=�6g   �� 5T      $	��100��6'   $��1��63   $��5,0,5,0q�6C   $��1��6T   $��22��6a   �� 5U      $��	Char Code��6(   $
��Bold"�69   $��2��6K   $��1��6[   $��Center=�6h   �� 5V      $	��100��6'   $��3��63   $��1��6C   $��5,0,5,0q�6P   $��22��6a   �� 5W      $��Offset��6(   $
��Bold"�66   $��4��6H   $��1��6X   $��Center=�6e   F   @EagleBoost.Wpf.Presentation.Controls.NumericUpDown.NumericUpDown  5X      $��45��6:   $��1��6E   $��5��6R   $
��Left=�6b   $��5,0,5,0q�6~   5Z      �� 5]      $��Images/delete.png��6    $��16��6;   $��16��6F   $��2��6R   $��	10,0,10,0q�6b   .  (System.Windows.Interactivity.Interaction   Triggers 5^      /  )System.Windows.Interactivity.EventTrigger  5_   "      	EventName$ 	MouseDown��61   .  (System.Windows.Interactivity.TriggerBase	   Actions.	 5`   &   "  Caliburn.Micro.ActionMessage  
   
MethodName$
 	RemoveJob��68      
Parameters. 5a   *     Caliburn.Micro.Parameter     Value 68   �� 5b   '   5c   #   5e      5g      5j      5l      5o      5p      