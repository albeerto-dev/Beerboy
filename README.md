# Beerboy - *Homebrewing Automation* <img src= "https://github.com/albeerto-dev/Beerboy/blob/master/LG_BB_small.png" align="right" />

Beerboy is an `Arduino` based open source brewing controller for homebrewers.

## Functionalities

It's for a single Input / single Output system.  It's perfect for
BIAB systems. "Beerboy" can be used also for classic ALL GRAIN just by
swapping the plug of the heating element of the mash kettle
with the one of the boil kettle (you'll also need to move the
temperature probe in the boil kettle). If your AG system has a sparge kettle/pot
this controller can't manage it.

It can be used in two `modes`:
* __AUTO__
* __MANUAL__
#### AUTO
Everything is set before the start of the brewday. The mash temperature is controlled by
a `PID algorithm`.
In sequence you can set up :
1. Number of mash steps
2. Temperature and time in minutes of each step of the mash
3. Boil duration in minutes
4. Number of hop jetty
5. Time of each hop jetty
6. Hopstand duration in minutes.

With auto mode it's also possible to choose your chilling method. If an `IMMERSION CHILLER` is used, a piece of code needs to be changed. Doing this the wort temperature is going to be printed during the chilling phase.
 ```javascript
 bool immersionChiller = 1;//Set to "1" if you use IMMERSION CHILLER. Set to "0" if others
 ```
#### MANUAL
It's like a classic temperature `PID controller`. You can olny set the
temperature desired and change it whenever you want.
## Hardware components
* Arduino Uno R3 (or equivalent) x 1
* Buzzer x 1
* Button x 3
* Display LCD 16x2 x 1
* Temperature probe DS18B20 x 1
* Wires
* Resistor 10 kOhm x 3 [buttons] - PIN 8 9 10
* Resistor 2 kOhm x 1 [LCD frontlight] - PIN v0
* Resistor 220 Ohm x 1 [LCD backlight] - PIN A
* Resistor 100 Ohm x 1 [buzzer]
* Resistor 4k7 Ohm x 1 [DS18B20] - PIN 13
* SSR 25A / 40A x 1
## Tuning PID parameters
<img src= "https://github.com/albeerto-dev/Beerboy/blob/master/PID_tuning.gif" align="right" />

Before the first batch with this controller it's necessary to tune the 3 parameters of the `PID controller : KI , KD, KP`.
Fill up the mash tun with the same liters of water usually used for the brew. Select MANUAL MODE on the controller and while the arduino board is connected to your PC and the Serial monitor is opend set new tuning parameters following these steps:
 1. Set all gains to zero except KP.
 2. Increase the P gain until the response to a disturbance is steady oscillation.
 3. Increase the D gain until the the oscillations go away (i.e. it's critically damped).
 4. Repeat steps 2 and 3 until increasing the D gain does not stop the oscillations.
 5. Set P and D to the last stable values.
 6. Increase the I gain until it brings you to the setpoint with the number of oscillations desired (normally zero but a quicker response can be had if you don't mind a couple oscillations of overshoot)

 [ *The image has the only scope to show how parameters work and those values are just an example* ]

 The piece of code to set:
 ```javascript
 //Specify the links and initial tuning parameters
double Kp=1, Ki=0, Kd=0;
 ```
## Wiring instructions
That's the precise scheme. AC Input should be your main plug. AC Output should be your heating element.
![Image of wiring](https://github.com/albeerto-dev/Beerboy/blob/master/Beerboy_Scheme.png)
## Video demonstration
work in progress / coming soon
## Version
V 1.0.1
## About
`Beerboy` is a brewing controller developed by `Alberto M. Ramagini`.

* E-mail : alberto.ramagini@gmail.com
* PayPal : www.paypal.me/albertoramagini
