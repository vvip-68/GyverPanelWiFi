
## �������� �������

**������� �������** - ��������� ������� �������� 8x8, 16x16, 32x8 ��� ��������������� ��������� ������� ������������� �������.  
**������� �������** - ������ �� ***���������*** ����������� �������, ������ � ���������� ���� ������� �������� �������.  

### ��� �������

������� ����������� � ������� ��� �������� ������� ����������� ���������������, ���� �� ������.  
������/������� ������� ����� ����������� ��������������� (��� "������") ��� ����������� (��� "������������")  

![��� �������](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Matrix/scheme1.jpg)

�������� �������� �� ��������� ������ � ������� � ���������������� � ������������ ���� ������.  
�������� ���������������� ����� � ����� ������ ������������� � �������� ������ - ���������������� ������� � ������������ ��������.

![��� �������](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Matrix/scheme3.jpg)

### ��� ����������� �������

����� �� ������������� ������ ����� ����������, ������������� �� � ������������ �������� ���� ����������� ������� ���������� � ������� �
������������ ���������� ������� ������ �� ���� �����������.

���� ����������� ������������ ���, � ����� ���� ��������� ������ � ������� ���������, ����� ������� ��������� ����� ���� � �� �������� �� ���.  
���� ����������� �������� ���������� **CONNECTION_ANGLE**, ������� ����� ��������� ��������� ��������:  

- **CONNECTION_ANGLE = 0** - ����� ������
- **CONNECTION_ANGLE = 1** - ����� �������
- **CONNECTION_ANGLE = 2** - ������ �������
- **CONNECTION_ANGLE = 3** - ������ ������

������ �������� **STRIP_DIRECTION** ���������� ����������� ���������� ������� ����������� �� ���� �����������:  
- **STRIP_DIRECTION = 0** - ������  
- **STRIP_DIRECTION = 1** - �����  
- **STRIP_DIRECTION = 2** - �����  
- **STRIP_DIRECTION = 3** - ����  

![��� �����������](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Matrix/scheme2.jpg)

### ��������� �������

� ��������� ����� � ��������� �������� ��� ������������� ������ ��������� ������ �� ������ ����������� ������ �������� 8�8, 16�16 � 32x8 "��������".

![��������� �������](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Matrix/led_matrix.jpg)

��� ��������� ������� ����� ��� ���������� ����� - ***������*** *(����������������)*, � �������� **MATRIX_TRYPE = 0**

**������� 8x8**  
![������� 8x8](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Matrix/Matrix8x8.jpg)

**������� 16x16**  
![������� 16x16](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Matrix/Matrix16x16.jpg)

**������� 32x8**  
![������� 32x8](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Matrix/Matrix32x8.jpg)


# ������� �������

�� ��������� ������� ��������� ������ 8�8, 16�16, 32�8 (��� �������������� ��������� ������) ����� ��������� ������� �������� �������.
� ��������� ������� ������������ �� �� �������� - *���� ����������� � ����������� �� ����*, ��� � ��� ��������� ����������� � ��������� �������.
��� �������� ��������� ������� **������ ����� ���������� ������ � ���� �������������� �����������** - ����� ���� � ��� �� ���� ����������� ������� 
���������� � �������� � ����������� ���������� ������� ����������� �� ����.  

������ ���������� ��������� ������� � �������������� ��������� 8x8 �������� �� ��������. 
������ ���� � �������� ������� ������ ������� ������, ����������� ������� ������ � �������� �� ���� - ����� ��������. 
������ ����� ���������� ���������� ��������� �������, ������� ���������� ��������� � ������� ������� - ������� �������.  

## ������� ������� 3x2 �� ����� ��������� 8x8 - ������� 1

![������� ������� �1](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Matrix/scheme4.jpg)

**������� ��������� �������** - ���������� ��������� � ***������*** � � ***������*** �������� ����������� **META_MATRIX_WIDTH** � **META_MATRIX_HEIGHT** ��������������. 
**��� ������������** ��������� � ������� ������� - ������� ��� ����������� �������� ���������� **META_MATRIX_TYPE**:  

- **META_MATRIX_TYPE = 0** - ������ (���������������� ����������) ���������  
- **META_MATRIX_TYPE = 1** - ������������ ���������� ���������  

**���� ������** ������ ��������� ������� (������ �������) - �������� ���������� **META_MATRIX_ANGLE**:  
- **META_MATRIX_ANGLE = 0** - ����� ������
- **META_MATRIX_ANGLE = 1** - ����� �������
- **META_MATRIX_ANGLE = 2** - ������ �������
- **META_MATRIX_ANGLE = 3** - ������ ������

**�����������**, � ������� ��������� ��������� ������� ������� - ���������� **META_MATRIX_DIRECTION**:  
- **META_MATRIX_DIRECTION = 0** - ������  
- **META_MATRIX_DIRECTION = 1** - �����  
- **META_MATRIX_DIRECTION = 2** - �����  
- **META_MATRIX_DIRECTION = 3** - ����  

����� ����������� ������ ��� ������� ������� �������, ��� � ������ ����������� ������ � ������ � ����� **a_def_hard.h**  
������� �������, ����������� �� ������� ���� ����������� ��������� ����������� ����������:  

![������ 1](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Matrix/Example1.jpg)

## ������� ������� 3x2 �� ����� ��������� 8x8 - ������� 2

![������� ������� �1](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Matrix/scheme5.jpg)

������� �������, ����������� �� ������� ���� ����������� ��������� ����������� ����������:  

![������ 2](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Matrix/Example2.jpg)

## ������ �� ������ ��������

���� ���� ������ ������� �� ������ �������� �������, ������� ������ � ������ ������ �������� � ������ ��� �����������.
��������� **META_MATRIX_WIDTH** � **META_MATRIX_HEIGHT** - ������ � ������ ������� ������� ������� **1**, 
��������� ��������� - **META_MATRIX_TYPE**, **META_MATRIX_ANGLE** � **META_MATRIX_DIRECTION** �� ����� ��������.

![������ 3](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Matrix/Example3.jpg)

# ��������� �� ����������

��������� ������� ������� (� ��� ����� � �������) � ������ �� ����������� ����� �� ����������� �� ���������.
��������� ����������-��������� ***WiFi Panel Setup*** � ������������ � ����������� �����������.
��������� � �������� �������� ���������� �������

![Panel Setup](https://github.com/vvip-68/GyverPanelWiFi/blob/master/wiki/Matrix/WiFiPanelSetup.jpg)

� ���������� �� ������� ��� ������� - ***��������� �������� �������*** � ***��������� ������� �������***.
1. ������� ������� ������ �������� ������� - **������** � **������**, �������� ��� ����������� ������� ����������� - **"������"** (*���������������� ����������*)
��� **"������������"**. �������� ������� ������������ **���� �������** (��� ��������� ������ ��������� � �������) �
**�����������** ���������� ������� ����������� �� ����� ����.  
2. ������� ���������� ��������� ������� ������� � **������** � **������**. �������� ��� ������������������ ���������� ��������� - **��������** ��� **�����������**.
�������� ������� ������������ **����** ������� ������� (��� ��������� ������ ������� � ������� ���������) �
**�����������** ���������� ������� ��������� �� ����� ����.  
3. ������� ������ **"���������"** ��� ���������� ����������� ��������. ����� ���������� ���������������� ������� ���������� ������������� �������������� �
����� ��������� ������� � ����.
