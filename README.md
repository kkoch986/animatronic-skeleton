# dmx-skull

This is the repo for the code that will run inside the head of each of the main animatronic skulls. It receives DMX commands and uses them to operate and configure the skull.

TODO: add schematics / wiring diagrams for how this all comes together

## Controlling DMX Starting Channel

DMX starting channel is controlled via the 8x DIP switches.
The switches are numbered 1-8 with the least significant bit being the switch labelled 8 (all the way to the right).

Examples:

| SW1 | SW2 | SW3 | SW4 | SW5 | SW6 | SW7 | SW8 | DMX Channel |
|-----|-----|-----|-----|-----|-----|-----|-----|-------------|
| Off | Off | Off | Off | Off | Off | Off | Off |      0      |
| Off | Off | Off | Off | Off | Off | Off | On  |      1      |
| Off | Off | Off | Off | Off | Off | On  | Off |      2      |
| Off | Off | Off | Off | Off | Off | On  | On  |      3      |
| Off | Off | Off | Off | Off | On  | Off | Off |      4      |
| ... | ... | ... | ... | ... | ... | ... | ... |     ...     |
| On  | On  | On  | On  | On  | On  | Off | On  |     253     |
| On  | On  | On  | On  | On  | On  | On  | Off |     254     |
| On  | On  | On  | On  | On  | On  | On  | On  |     255     |

## Operation Modes

Operating modes are controlled using the first addressed DMX channel, this is true in all operating modes.

Currently the following operating modes are supported, they are described in more detail below as well.

| Code  | Name               |
|-------|--------------------|
| 0x00  | DMX Direct Control |

TODO: more codes? idle mode? some kind of tracking mode? puppeting mode?

### DMX Direct Control (0x00)

In this mode, the motors and eye color are controlled directly via DMX commands.
This mode should be primarily used during a show where the skeletons are being controlled by external software.

#### DMX Commands

All commands are referenced based off the starting channel

| Offset | Name    | Description |
|--------|---------|-------------|
|    0   | SetControlMode | Sets the current operating control mode (see above for list of control modes) |
|    1   | SetJaw  | Sets the openness of the jaw. 0 = fully closed, 255 = fully open |
|    2   | SetPan  | Sets the rotation of the head. 0 = fully stage left, 127 = straight ahead, 255 = fully stage right |
|    3   | SetTilt | Sets the tilt angle of the head. 0 = fully tilted stage left, 127 = upright, 255 = fully tilted stage right |
|    4   | SetNod  | Sets the Nod angle of the head. 0 = fully angled down, 127 = straight on, 255 = fully angled up |
|    5   | SetEyesH | Sets the horizontal angle of the eyes. 0 = fully stage left, 127 = straight on, 255 = fully stage right |
|    6   | SetEyesV | Sets the vertical angle of the eyes. 0 = fully down, 127 = straight on, 255 = fully up |
|    7   | SetEyesRed | |
|    8   | SetEyesGreen | |
|    9   | SetEyesBlue | |

TODO: something about adjusting the motor trim?
TODO: storing / retrieving settings from eeprom?


# PCB TODOs

- Red LED pin is square, gnd should be square
- fix led wiring so they arent pulled high (maybe just replace that vcc pad with a gnd pad?)
- add pins to the other side of the servo board for support
- fix spacing on power pins to servo board
- label main power inputs
- also include the expansion labels on the bottom of the board
- remove servo pins, but replace with the labels of what each index is
- write DMX pin labels on the bottom and top
- draw a skull in between the arduino pins on the top layer
- draw an area on the silkscreen to note the board number and DMX offset
- adjust main board power to be a regular connector (not a screw terminal)
- use arduino nano instead
- isolate logical power from servo power
