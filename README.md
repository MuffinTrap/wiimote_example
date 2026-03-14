# wiimote_example
Modified version of libogc's wiimote example

Modified by muffintrap to test expansions.
- Prints out connected expansion and some input state from it.
- Bound minus key to disconnect all wiimotes.

## Why:
Expansions work differently on Dolphin emulator between libogc versions 3.0.4 and 2.11.0-1 
In earlier version the expansions were noticed immediately
In 3.0.4 the expansions are only noticed after wiimote is disconnected and connected again.
