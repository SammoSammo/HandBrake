   M S B A M L   `   `   `  ����G  CHandBrakeWPF, Version=0.9.5.0, Culture=neutral, PublicKeyToken=nullA%clr-namespace:HandBrakeWPF.ConvertersHandBrakeWPF.Converters  5      "    HandBrakeWPF.Views.QueueView   5   	   \ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35R NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35D 9http://schemas.microsoft.com/winfx/2006/xaml/presentation    5      R NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml     5      Jmc;http://schemas.openxmlformats.org/markup-compatibility/2006     5      @d2http://schemas.microsoft.com/expression/blend/2008     5      U QCaliburn.Micro, Version=1.2.0.0, Culture=neutral, PublicKeyToken=8e5891231f2ed21f(calhttp://www.caliburnproject.org  6K   6
Converters%clr-namespace:HandBrakeWPF.Converters  5      c _System.Windows.Interactivity, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35Bi:http://schemas.microsoft.com/expression/2010/interactivity  5   	   $	��600��5      $	��400��6   ���	#FFF0F0F0  � Title  5   _   �� *Title+ � 	Resources 5      :   4HandBrakeWPF.Converters.BooleanToVisibilityConverter   boolToVisConverter��%   &	          5   
   62   5      .��5      �  � RowDefinitions 5   
   �� 5      $
��Auto �6   �� 5      $
��Auto �6   �� 5      $��* �6   .+�5   
   c� -   5        $��mainToolBar��5      $��Stretch=�6%   $��Stretch=�6C    J� SnapsToDevicePixels .  .@�5      �� -   5        $��Start��5        Caliburn.Micro.Message   Attach$* $[Event Click] = [Action StartEncode]��6"   y�6\   �� *
IsEncoding+ �� 	Converter# [   �� ConverterParameter$
 true��.��5      ��  �� Orientation$ 
Horizontal=�6   .+�5      �� $��Images/Play.png��6   $��32��65   $��32��6A   �� 5      $��Start��6   $��8,0,0,0q�6-   $��Center=�6>   5      5      �� 5      -   5        $��Pause��5      $* $[Event Click] = [Action PauseEncode]��6"   y�6\   �� *
IsEncoding+# [  $ false��.��5       �� $ 
Horizontal=�6   .+�5!      �� $��Images/Pause.png��6   $��32��66   $��32��6B   �� 5"      $��Pause��6   $��8,0,0,0q�6-   $��Center=�6>   5#      5$      5%      �� 5'   
   $��1��6   $��10,20,10,20q�6"   .+�5(      �� ��6   �� *JobsPending+�� 5)      ��6   �� *	JobStatus+5*      �� 5,   
   $��2��6    �� SelectionMode$ Extended=�6A   
�������$��
10,0,10,10q�6q   ��6   �� *	QueueJobs+��5-      �� 5.      	 �� 
TargetType"	 ��6   .�5/      �� 
 �� Property
 � �� �� Value$ Stretch=�6C   6U   50      ��53      �� 54      .R�56      � $��Stretch=�6    � ColumnDefinitions 57      �� 58      $
��Auto �6/   �� 59      $��* �6/   �� 5:      $
��Auto �6/   .+�5>      �� $��Images/Movies.png��6    $��16��6;   $��16��6F   $��0��6R   $��	10,0,10,0q�6b   �� 5A      $��1��6%   $��Stretch=�65   .+�5B      �� $ 
Horizontal=�6)   .+�5C   "   �� $��Source��6,   $
��Bold"�6:   �� 5D   "   ��6,   �� *Source+5E      �� 5G      $ 
Horizontal=�6)   .+�5H   "   �� $��Destination��6,   $
��Bold"�6?   �� 5I   "   ��6,   �� *Source+5J      5K      �� 5N      $��Images/delete.png��6    $��16��6;   $��16��6F   $��2��6R   $��	10,0,10,0q�6b   .  (System.Windows.Interactivity.Interaction   Triggers 5O      /  )System.Windows.Interactivity.EventTrigger  5P   "      	EventName$ 	MouseDown��61   .  (System.Windows.Interactivity.TriggerBase   Actions. 5Q   &   "  Caliburn.Micro.ActionMessage     
MethodName$ 	RemoveJob��68      
Parameters. 5R   *     Caliburn.Micro.Parameter     Value 68   �� 5S   '   5T   #   5V      5X      5[      5]      5`      5a      