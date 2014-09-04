#4000
триггер на скорняке ~
0 j 100
~
wait 2
switch %object.name% 
case труп мыши
  say  Благодарствую, ты меня выручаешь, получи плату. 
  load obj 4003
  дать 1 монет %actor.name%
  mpurge труп 
break
default
  say  Зачем мне это? 
  eval getobject %object.name%
  if  %getobject.car% == труп
     mpurge труп
  else
     броси %getobject.car%.%getobject.cdr%
  end
  break
done 
~
$
$
$
