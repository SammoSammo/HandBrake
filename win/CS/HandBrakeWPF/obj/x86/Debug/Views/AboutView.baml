   M S B A M L   `   `   `  ����U  QCaliburn.Micro, Version=1.2.0.0, Culture=neutral, PublicKeyToken=8e5891231f2ed21fG4clr-namespace:Caliburn.Micro;assembly=Caliburn.MicroCaliburn.Micro  5   @    HandBrakeWPF"   HandBrakeWPF.Views.AboutView   5   	   \ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35R NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35D 9http://schemas.microsoft.com/winfx/2006/xaml/presentation    5   	   R NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml     5   	   FMicro4clr-namespace:Caliburn.Micro;assembly=Caliburn.Micro     6@   $	��320��5   	   $	��600��6     � Title  5   �   �� *Title+.��5      �  � RowDefinitions 5   
   �� 5      $��* �6   �� 5	      $
��Auto �6   .+�5   
   � $��0��6    � ColumnDefinitions 5      �� 5      $
��Auto �6#   �� 5      $��* �6#   .+�5      �� $��Images/logo64.png��6   $��64��6/   $��64��6:   $��0��6F    J� SnapsToDevicePixels . $��10,10,10,10q�6q   $
��Left=�6�   $	��Top=�6�   � 5      $��1��6    5      �� 5      $
��Auto �6$   �� 5      $
��Auto �6$   �� 5      $
��Auto �6$   �� 5      $��* �6$   .+�5      ��  �� Orientation$ 
Horizontal=�6   $��5,10,0,0q�66   $��0��6H   .+�5      �� $��	HandBrake��6    $
��Bold"�61   $��14(�6C   $��0,0,5,0q�6Q   �� 5      $��0,0,0,1q�69   $��Bottom=�6J   ��6    �� *	Version+6J   5      �� 5!      $(��"Copyright 2003-2011 HandBrake Team��6   $��5,0,0,0 q�6F   $��1��6X   �� 5#      $��License:��6   $��5,10,0,5q�6,   $��2��6>   �� 5$      $��Stretch=�6   $��Stretch=�66    �� 
IsReadOnly .  �� TextWrapping$
 Wrap=�6f   " �� VerticalScrollBarVisibility$
 Auto=�5%      $��
10,0,10,10q�6=   $��3��6Q   .��6^   ��This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA5+      5-      5.      �� 50   
   $��Stretch=�6   
�������$��1��6J   .+�51      �� $��Close��6      Caliburn.Micro.Message   Attach$$ [Event Click] = [Action Close]��6%   $��Right=�52      $
��12,2q�61   $��0,5,10,5q�6@   $��Center=�6R   53      54      55      