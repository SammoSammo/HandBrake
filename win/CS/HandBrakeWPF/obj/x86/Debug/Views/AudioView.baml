   M S B A M L   `   `   `  ����V  REagleBoost.Wpf.Presentation, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null�eclr-namespace:EagleBoost.Wpf.Presentation.Controls.NumericUpDown;assembly=EagleBoost.Wpf.Presentation2EagleBoost.Wpf.Presentation.Controls.NumericUpDown  5       HandBrakeWPF"   HandBrakeWPF.Views.AudioView   5      \ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35R NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35D 9http://schemas.microsoft.com/winfx/2006/xaml/presentation    5      R NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml     5      Jmc;http://schemas.openxmlformats.org/markup-compatibility/2006     5      @d2http://schemas.microsoft.com/expression/blend/2008     5      c _System.Windows.Interactivity, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35Bi:http://schemas.microsoft.com/expression/2010/interactivity  5      U QCaliburn.Micro, Version=1.2.0.0, Culture=neutral, PublicKeyToken=8e5891231f2ed21f(calhttp://www.caliburnproject.org  5      yNumericUpDowneclr-namespace:EagleBoost.Wpf.Presentation.Controls.NumericUpDown;assembly=EagleBoost.Wpf.Presentation  5      .��5
      �   � RowDefinitions  5   
   �� 5      $
��Auto �6   �� 5      $
��Auto �6   �� 5      $��* �6   .+�5   
   �� $��Audio Tracks��6   $
��Bold"�6(   $��10,5,0,0q�6:   $��0��6L   6\   � 5   
   $��1��6   $��	10,5,10,0q�6    � ColumnDefinitions 5      �� 5      $
��Auto �6#   �� 5      $
��Auto �6#   .+�5      �� $��	Add Track��6     Caliburn.Micro.Message   Attach$" [Event Click] = [Action Add]��6)   $��0��6[   $��75��6k   $��0,0,10,0q�6v   �� 5      $��Remove��6   $% [Event Click] = [Action Remove]��6&   $��1��6\   $��75��6l   5      �� 5   
   $��2��6    �� SelectionMode$ Extended=�5      
�������$��10,10,10,10q�6B   ��5      �� *AudioTracks+��5      �� 5       �� 
TargetType" ��6   .�5       ��  �� Property � �� �� Value$ Stretch=�6C   6U   5!      ��5$      �� 5%      .R�5'      � $��Stretch=�6    5(      �� 5)      $
��Auto �6/   �� 5*      $��* �6/   �� 5+      $
��Auto �6/   .+�5/      �� $��Images/Movies.png��6    $��16��6;   $��16��6F   $��0��6R   $��	10,0,10,0q�6b   � 52      $��1��6   $��Stretch=�6/    53      �� 54   "   $
��Auto �63   �� 55   "   $
��Auto �63   �� 56   "   $
��Auto �63   �� 57   "   $
��Auto �63   �� 58   "   $
��Auto �63   �� 59   "   $
��Auto �63   �� 5:   "   $
��Auto �63   �� 5;   "   $
��Auto �63   �� 5<   "   $
��Auto �63   �� 5=   "   $
��Auto �63   �� 5>   "   $
��Auto �63     5A      �� 5B   "   $��28 �60   �� 5C   "   $��28 �60   .+�5H      �� $��Source��6(   $
��Bold"�66   $��0��6H   $��Center=�6X   �� 5I      $	��100��6'   $��1��63   $��5,0,5,0q�6C   $��22��6T   �� 5J      $��Codec��6(   $
��Bold"�65   $��2��6G   $��Center=�6W   �� 5K      $	��100��6'   $��3��63   $��5,0,5,0q�6C   $��22��6T   �� 5L      $��Bitrate��6(   $
��Bold"�67   $��4��6I   $��Center=�6Y   �� 5M      $��70��6'   $��5��62   $��5,0,5,0q�6B   $��22��6S   �� 5N      $��
Samplerate��6(   $
��Bold"�6:   $��6��6L   $��Center=�6\   �� 5O      $��70��6'   $��7��62   $��5,0,5,0q�6B   $��22��6S   �� 5R      $��Mixdown��6(   $
��Bold"�67   $��0��6I   $��1��6Y   $��Center=�6f   �� 5S      $	��100��6'   $��1��63   $��5,0,5,0q�6C   $��1��6T   $��22��6a   �� 5T      $	��DRC��6(   $
��Bold"�63   $��2��6E   $��1��6U   $��Center=�6b   F   @EagleBoost.Wpf.Presentation.Controls.NumericUpDown.NumericUpDown  5U      $��45��6:   $��1��6E   $��3��6R   $
��Left=�6b   $��5,0,5,0q�6~   �� 5V      $
��Gain��6(   $
��Bold"�64   $��4��6F   $��1��6V   $��Center=�6c     5W      $��45��6:   $��1��6E   $��5��6R   $
��Left=�6b   $��5,0,5,0q�6~   5Y      �� 5\      $��Images/delete.png��6    $��16��6;   $��16��6F   $��2��6R   $��	10,0,10,0q�6b   .  (System.Windows.Interactivity.Interaction   Triggers 5]      /  )System.Windows.Interactivity.EventTrigger  5^   "      	EventName$ 	MouseDown��61   .  (System.Windows.Interactivity.TriggerBase	   Actions.	 5_   &   "  Caliburn.Micro.ActionMessage  
   
MethodName$
 	RemoveJob��68      
Parameters. 5`   *     Caliburn.Micro.Parameter     Value 68   �� 5a   '   5b   #   5d      5f      5i      5k      5m      5n      