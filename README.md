avr-oscilloscope
================

A 150 ksps oscilloscope with edge triggering for the ATMega328.

```
make
make flash
```

Open a serial terminal. To read in 1024 samples, enter 'r'. To set the trigger level, enter 't' followed by three digits to form a number 0-255, with 0 being -10 volts and 255 being 10V.

