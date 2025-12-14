# ESP32-BT-Speaker
ESP32 Bluetooth speaker with prompt audio support (this code is for ESP32-A1S/ESP32-AudioKit boards.)

# How to use
1. Use Arduino IDE (new version preferred but you can use older version without dark mode)
2. Select "ESP32-WROOM-DA" module from the board list
3. Select "Huge APP (3MB No OTA/1MB SPIFFS)" from the partition scheme
4. Connect the board and select the port
5. Install https://github.com/pschatzmann/ESP32-A2DP
6. Install https://github.com/pschatzmann/arduino-audio-driver
7. Install https://github.com/pschatzmann/arduino-audio-tools
8. Install OneButton library
9. Upload

# How to customize prompt audio
1. Acquire desired sound files
2. Adjust and edit
3. Save as 16bit Stereo WAV file with 44100Hz sampling rate
4. Use HxD or other software to convert it to C array
5. Change data and length value pair in `riffsound.h` for each sound you want to change (important to change the length value otherwise the audio may end too early or cause overflow !!)
6. Upload

# How do I add more sounds?
Add events you want to capture in the code and use `queuePromptSound(sound, length);` where needed.

Sounds must be less than 1.5 seconds per file !!

# Known Issues
- The volume of Bluetooth source is set automatically to 0 upon connecting for some reason.
- Clicks can be heard in the prompt audio, this may be because of the header data and metadata included (consider removing it on your side)
- Stock sounds are too loud
- The playback of the prompt audio speeds up when Bluetooth source connects

# Special Thanks
Phil Schatzmann (he basically wrote the whole thing that runs this code)
