#4000
������� �� �������� ~
0 j 100
~
wait 2
switch %object.name% 
case ���� ����
  say  �������������, �� ���� ���������, ������ �����. 
  load obj 4003
  ���� 1 ����� %actor.name%
  mpurge ���� 
break
default
  say  ����� ��� ���? 
  eval getobject %object.name%
  if  %getobject.car% == ����
     mpurge ����
  else
     ����� %getobject.car%.%getobject.cdr%
  end
  break
done 
~
$
$
$
