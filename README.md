# Beerboy 2.0  - *Automation for homebrewing*

Beerboy is an Arduino brewing controller for every homebrewer.

### Functionalities

It's a single Input / single Output system.  It's perfect for 
BIAB systems. "Beerboy 2.0" can be used also for classic ALL GRAIN just by 
swapping the plug of the heating element of the mash kettle 
with the one of the boil kettle (you'll also need to move the
temp probe in the boil kettle). If your AG system has a sparge kettle/pot 
this controller can't manage it.

It can be used in two `modes`:
* __AUTO__
* __MANUAL__
### AUTO
Everything is set before the start of the brewday. The mash temperature is controled by 
a PID algorithm sequence
You can set up : 
1. Number of mash steps 
2. Temperature and time in minutes of each step of the mash 
3. Boil duration in minutes 
4. Number of hop jetty 
5. Time of each hop jetty 
6. Hopstand duration in minutes.

### MANUAL
It's a classic temperature PID controller. You can olny set the 
temperature desired and change it whenever you want.
### *WORK IN PROGERSS*
....
### About
Beerboy 2.0 is a brew controller developed by Alberto M. Ramagini