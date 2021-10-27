# animatronic-skeleton

## Main operation

The basic principle is an Arduino UNO which recieves Renard protocol over the USB 
Serial bus. 
It then takes the Renard input and proxies it all out through a MAX485 chip as DMX.
It also uses some of the values directly to control a 16 channel PWM servo breakout
board which drives the servos on the skeletons.


## Channel Mappings

Main Skeleton Show Sketch

There are some primary models:

* **Skeleton1** - mini skeleton with full pan/tilt/jaw
* **Skeleton2** - full sized skeleton with 2-axis control + jaw
* **Skeleton3** - full sized skeleton with 2-axis control + jaw
* **Skeleton4** - mini skeleton with full pan/tilt/jaw
* **ParLight1 - 4** - RGB Par Lights directed at each skeleton in corresponding number
* **FogMachine** - A DMX controllable fog machine

| Channel | Model | Assignment | Notes |
| ------- | ----- | ---------- | ----- | 
|    1    | Skeleton1 | Control Mode | See CONTROL_MODE_* constants |
|    2    | Skeleton1 | Input 1      | Set Pan                      |
|    3    | Skeleton1 | Input 2      | Set Tilt                     |
|    4    | Skeleton1 | Input 3      | Set Jaw (closed -> open)     |
|    5    | Skeleton2 | Control Mode | See CONTROL_MODE_* constants |
|    6    | Skeleton2 | Input 1      | Set Stage Left Servo         | 
|    7    | Skeleton2 | Input 2      | Set Stage Right Servo        |
|    8    | Skeleton2 | Input 3      | Set Jaw                      |
|    9    | Skeleton3 | Control Mode | See CONTROL_MODE_* constants |
|   10    | Skeleton3 | Input 1      | Set Pan                      |
|   11    | Skeleton3 | Input 2      | Set Tilt                     |
|   12    | Skeleton3 | Input 3      | Set Yaw                      |
|   13    | Skeleton3 | Input 4      | Set Jaw                      |
|   14    | Skeleton4 | Control Mode | See CONTROL_MODE_* constants |
|   15    | Skeleton4 | Input 1      | Set Pan                      |
|   16    | Skeleton4 | Input 2      | Set Tilt                     |
|   17    | Skeleton4 | Input 3      | Set Yaw                      |
|   18    | Skeleton4 | Input 4      | Set Jaw (closed -> open)     |
|   19    | ParLight1 | CH 1         | Master Dimmer                |
|   20    | ParLight1 | CH 2         | Red Dimming                  |
|   21    | ParLight1 | CH 3         | Green Dimming                |
|   22    | ParLight1 | CH 4         | Blue Dimming                 |
|   23    | ParLight1 | CH 5         | Flicker Mode                 |
|   24    | ParLight1 | CH 6         | Control Mode                 |
|   25    | ParLight1 | CH 7         | Color Selection speed        |
|   26    | ParLight2 | CH 1         | Master Dimmer                |
|   27    | ParLight2 | CH 2         | Red Dimming                  |
|   28    | ParLight2 | CH 3         | Green Dimming                |
|   29    | ParLight2 | CH 4         | Blue Dimming                 |
|   30    | ParLight2 | CH 5         | Flicker Mode                 |
|   31    | ParLight2 | CH 6         | Control Mode                 |
|   32    | ParLight2 | CH 7         | Color Selection speed        |
|   33    | ParLight3 | CH 1         | Master Dimmer                |
|   34    | ParLight3 | CH 2         | Red Dimming                  |
|   35    | ParLight3 | CH 3         | Green Dimming                |
|   36    | ParLight3 | CH 4         | Blue Dimming                 |
|   37    | ParLight3 | CH 5         | Flicker Mode                 |
|   38    | ParLight3 | CH 6         | Control Mode                 |
|   39    | ParLight3 | CH 7         | Color Selection speed        |
|   40    | ParLight4 | CH 1         | Master Dimmer                |
|   41    | ParLight4 | CH 2         | Red Dimming                  |
|   42    | ParLight4 | CH 3         | Green Dimming                |
|   43    | ParLight4 | CH 4         | Blue Dimming                 |
|   44    | ParLight4 | CH 5         | Flicker Mode                 |
|   45    | ParLight4 | CH 6         | Control Mode                 |
|   46    | ParLight4 | CH 7         | Color Selection speed        |
|   47    | FogMchine | Ch 1         | Fog Burst (off -> max)       |
|   48    | FogMchine | Ch 2         | Red LED Value                |
|   49    | FogMchine | Ch 3         | Green LED Value              |
|   50    | FogMchine | Ch 4         | Blue LED Value               |
|   51    | FogMchine | Ch 5         | LED Strobe control           |
|   52    | FogMchine | Ch 6         | LED Brightness control (?)   |
|   53    | FogMchine | Ch 7         | LED Sound control (?)        |

## DMX Channel Descriptions

A quick reference for the DMX equiptment used in the show

### Par light

| Channel | Range     | Description                           |
| ------- | --------- | ------------------------------------- |
| CH1     |   0 - 255 | Master Dimmer (off -> on)             |
| CH2     |   0 - 255 | Red Dimming (dim -> bright)           |
| CH3     |   0 - 255 | Green Dimming                         |
| CH4     |   0 - 255 | Blue Dimming                          |
| CH5     |           | Flicker Mode                          |
|         |   0 -   7 |   No Flicker                          |
|         |   8 - 255 |   Flicker speed (slow -> fast)        |
| CH6     |           | Control Mode                          |
|         |   0 -  10 |   Manual Control                      |
|         |  11 -  60 |   Color Selection                     |
|         |  61 - 110 |   Gradual Fade                        |
|         | 111 - 160 |   Variable Pulse                      |
|         | 161 - 210 |   Jump Change                         |
|         | 211 - 255 |   Sound activation mode               |
| CH7     |   0 - 255 | Color selection / discoloration speed |

I'm not 100% sure how to use CH6/CH7 effectively in all modes.
It seems like "Gradual Fade", "variable Pulse" and "Jump Change" modes
all move between any / all colors at a speed dictated by CH7.

### Fog Machine (Rockville R1200L)

| Channel | Range | Description |
| ------- | ----- | ----------- |
| CH1     |   0 - 255 | Fog Burst setting ( 0 = no fog, 255 = pumping max fog) |
| CH2     |   0 - 255 | Red LED value                                          |
| CH3     |   0 - 255 | Green LED value                                        |
| CH4     |   0 - 255 | Blue LED value                                         |
| CH5     |   0 - 255 | LED Strobe control (slow -> fast)                      |
| CH6     |           | LED Brightness control                                 |
|         |   0 -   9 |    LEDs are off                                        |
|         |  10 - 250 |    Set LED Brightness                                  |
|         | 251 - 255 |    Enable sound response mode                          |
| CH7     |   0 - 255 | LED sound control (sound response value)               |

It is very unclear to me what CH6 & CH7 actually do... seems like CH6 only works
when CH2/3/4 are not being used?

Anyway, for primary use, probably only really going to need CH1 - CH5.
