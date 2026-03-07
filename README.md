# IgiTimer

A simple and functional timer for Pebble smartwatches, featuring a high-contrast interface and quick controls.

## Button Functions

### UP Button
* **Single Press**: Adds **1 minute** to the timer.
* **Long Press**: Adds **5 minutes** to the timer.
* *Note: Incrementing resets seconds to zero.*

### SELECT Button (Center)
* **Single Press**: **PAUSES** or **RESUMES** the countdown.
* **Long Press**: **RESTARTS** the timer using the last set total duration.

### DOWN Button
* **Single Press**: Subtracts **1 minute** from the timer.
* **Long Press**: Subtracts **5 minutes** from the timer.
* *Note: If the time reaches zero, the timer stops.*

## User Interface
* **Display**: Minutes in the top half, Seconds in the bottom half.
* **Progress Bar**: A vertical bar on the left (green on black background) empties as time passes.
* **Status**:
  - When paused, a pause icon appears in the center.
  - When time expires, the watch vibrates.