@Herr Kobelrausch:
Bitte einfach code deployen, dann mit PuTTY verbinden:
putty -fn "Monospace 12" -serial /dev/ttyACM0 -sercfg 8,1,115200,n,X
Beim starten der "app" sollte ein Hilfetext kommen, oder einfach "help" eintippen
oder eben die reset Taste drücken.

Hier der "help" Text mit Erklärungen wie der UART Task umgesetzt wurde
----------------------------------------------------------------------
To generate a random value enter following command:
#r,<lower bound>:<upper bound>\n
lower_bound and upper_bound have to be unsigned integers (>= 0).
lower_bound has to be smaller than upper bound!
Instead of \n you can also use \r. With PuTTY
* to send \r (carriage return) hit enter or ctrl+m
* to send \n (line feed) use ctrl+j
Special keys (like arrow keys, escape, f-keys, etc.) are NOT supported!
To display this message again enter 'help' or '?'
