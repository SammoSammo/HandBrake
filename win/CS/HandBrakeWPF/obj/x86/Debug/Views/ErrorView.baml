   M S B A M L   `   `   `  ����  HandBrakeWPF"    HandBrakeWPF.Views.ErrorView   5   	   \ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35R NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35D 9http://schemas.microsoft.com/winfx/2006/xaml/presentation    5   	   R NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml     5   	   U QCaliburn.Micro, Version=1.2.0.0, Culture=neutral, PublicKeyToken=8e5891231f2ed21f(calhttp://www.caliburnproject.org  6@   $	��380��5   !   $	��680��6.   $��11(�6:   ���	#FFF0F0F0  � Title  6	   �� *Title+.��5      �  � RowDefinitions 5   
   �� 5      $��70 �6   �� 5      $��* �6   �� 5	      $
��Auto �6   .+�5   
   ��  �� Orientation$ 
Horizontal=�6   $��0��6.   $��0,0,0,0q�6;   
�������.+�5      �� $��Images/ErrorX.png��6   $��64��6/   $��64��6:   $��10,0,0,0q�6F   � 5      $��10,0,0,0q�6   $��64��6%    5      �� 5      $
��Auto �6$   �� 5      $
��Auto �6$   �� 5      $��* �6$   .+�5      �� $
��Bold"�6:   $��0��6L   $��0,5,0,0q�6Y   ��6   �� *ErrorMessage+6Y   �� 5      $��1��66   $��0,5,0,0q�6C   ��6   �� *
Solution+6C   �� 5      $��Error Details:��6   $
��Bold"�62   $��0,0,0,2q�6D   $��Bottom=�6U   $��2��6p   5      5      �� 5   
   $��Stretch=�6+   $��Stretch=�6G    �� 
IsReadOnly .  �� TextWrapping$
 Wrap=�6w   " �� VerticalScrollBarVisibility$
 Auto=�5      $��84,10,10,10q�6=   $��1��6R   ��5      �� *	Details+5   R   � 5"   
   $��2��6   
������� � ColumnDefinitions 5#      �� 5$      $��* �6#   �� 5%      $
��Auto �6#   �� 5&      $
��Auto �6#   .+�5)      �� $	��8,2q�6   $��5q�6#   $��1��6.     Caliburn.Micro.Message   Attach$# [Event Click] = [Action Copy]��6>   .��5*      �  5+      �� 5,      $
��Auto �6+   �� 5-      $
��Auto �6+   .+�5/      �� $��Images/copy.png��6   $��0��65   $��16��6E   $��16��6P   $��0,0,5,0q�6\   �� 50      $
��Copy��6    $��1��6,   51      52      �� 54      $��Close��6   $	��8,2q�6%   $��0,5,10,5q�63   $��2��6E   $$ [Event Click] = [Action Close]��6U   6�   55      56      57      