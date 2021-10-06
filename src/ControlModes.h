#ifndef CONTROLMODES
#define CONTROLMODES

// Control the PAN/TILT/JAW servos directly using DMX
// For full pan/tilt/jaw skeletons:
//   input 1 -> Pan
//   input 2 -> Tilt
//   input 3 -> Jaw
// For 2 axis skeletons:
//   input 1 -> stage left servo
//   input 2 -> stage right servo
//   input 3 -> Jaw
#define CONTROL_MODE_DMX 0

// An "idle" mode which will keep the head "homed" but randomly
//  look around in random directions
#define CONTROL_MODE_IDLE 1


#endif
