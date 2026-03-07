# IgiWatch

A dynamic and minimalist watchface for Pebble smartwatches that adapts to your viewing angle using the accelerometer.

## Features

*   **Smart Orientation**: The time display rotates to remain readable whether you are holding your arm upright, tilted left, or tilted right.
*   **Interactive Control**: Uses the accelerometer on-demand to save battery.
*   **Clean Design**: High-contrast typography using Avenir Next.

## How to Use

1.  **Standard View**: By default, the watchface displays the time in a standard vertical orientation.
2.  **Activate Rotation**: **Double tap** the watch (or shake your wrist twice quickly) to enter calibration mode.
3.  **Tilt**: A small indicator dot will appear. Tilt your wrist left or right to adjust the orientation.
4.  **Read**: After 5 seconds, the dot disappears and the time is displayed in the new orientation.

## Technical Details

*   Built with the Pebble C SDK.
*   Uses `pebble-fctx` for advanced font rendering.
*   Efficient accelerometer usage (only active for 5 seconds after interaction).