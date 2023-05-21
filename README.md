# CalendarApp
An app that will run on a pico, connected to an E-Ink display. It will be connected to the HA instance running in the home, and display its calendar. 
It will probably also display other useful information, or maybe just nice pictures.

The main idea is that it will run on batteries so it should take advantage of the pico-extras library so it can utilize the ARMs deep sleep,
functionality. Currently I'm running a test with a pico W and an LED that blinks evrey minute. it's running on two AAA batteries which outputs,
2.6 volts. There's about 1200 mah in one AAA, and they are connected in series so their voltage is increase but the mah is still 1200. 
this amounts to 1200/2.6 = 461.5 hours => 19 days, 5 hours and 31 minutes. which isn't too bad. I could double the 
amount of batteries and connect them in paralelle to increase the lifespan (maybe I will). would also be nice to see when the batteries are about to die, 
otherwise you wouldn't know whether it's still working since the E-Ink is always "on". 

## Components
 - Pico W
 - E-Ink display => maybe this one? only downside is that it take 30 sec. to refresh... https://shop.pimoroni.com/products/inky-frame-5-7?variant=40057850331219
 - rechargable batteries
 - Since you've learned you need more solar cells/panels in your bedroom for charging your phone everyday, you could use the solar panel you have already to charge the batteries for this project.

