#include <OneButton.h>
#include "AudioTools.h"
#include "AudioTools/AudioLibs/AudioBoardStream.h" // install https://github.com/pschatzmann/arduino-audio-driver

// #include "BluetoothA2DPSink.h"
#include "BluetoothA2DPSinkQueued.h"
#include "riffsound.h"

#define KEY_VOLUP 18
#define KEY_NEXT 23
#define KEY_PLAY 19
#define KEY_PREV 13
#define KEY_VOLDN 36

uint8_t curVolume = 63;
bool isMuted = false;

AudioBoardStream kit(AudioKitEs8388V1);
BluetoothA2DPSinkQueued a2dp_sink(kit);
PinsAudioKitEs8388v1Class board;

OneButton volup_btn(KEY_VOLUP, true);
OneButton next_btn(KEY_NEXT, true);
OneButton play_btn(KEY_PLAY, true);
OneButton prev_btn(KEY_PREV, true);
OneButton voldn_btn(KEY_VOLDN, true);

// Not a typo, necessary for the code to compile ! Do not fix
enum AudioSourcee {
  SOURCE_BLUETOOTH,
  SOURCE_PROMPT
};

AudioSourcee currentSource = SOURCE_BLUETOOTH;
bool promptPlaying = false;

struct PromptSound {
  const unsigned char* data;
  unsigned int length;
};

#define MAX_PROMPT_QUEUE 5
PromptSound soundQueue[MAX_PROMPT_QUEUE];
int queueHead = 0;
int queueTail = 0;
int queueCount = 0;

void readDataStream(const uint8_t *data, uint32_t length) {
  if (currentSource == SOURCE_BLUETOOTH) {
    kit.write(data, length);
  }
}

void queuePromptSound(const unsigned char* data, unsigned int length) {
  if (queueCount >= MAX_PROMPT_QUEUE) {
    Serial.println("Prompt queue is full !!");
    return;
  }
  soundQueue[queueTail].data = data;
  soundQueue[queueTail].length = length;
  queueTail = (queueTail + 1) % MAX_PROMPT_QUEUE;
  queueCount++;
  Serial.println("Prompt queued !!");
}

void playNextPrompt() {
  if (queueCount == 0 || promptPlaying) {
    return;
  }
  promptPlaying = true;
  currentSource = SOURCE_PROMPT;

  PromptSound prompt = soundQueue[queueHead];
  queueHead = (queueHead + 1) % MAX_PROMPT_QUEUE;
  queueCount--;

  // May be removed later if it does not work
  int originalVol = a2dp_sink.get_volume();
  a2dp_sink.set_volume(originalVol / 4);
  Serial.println("Playing prompt !!");

  const int CHUNK_SIZE1 = 512;
  size_t position = 0;

  while (position < prompt.length) {
    size_t chunk = min((size_t)CHUNK_SIZE1, prompt.length);
    kit.write(prompt.data + position, chunk);
    position += chunk;
  }

  // May be removed later

  a2dp_sink.set_volume(originalVol);
  promptPlaying = false;
  currentSource = SOURCE_BLUETOOTH;
  Serial.println("Playback finished !!");
}

void goNext() {
  ESP_LOGI("OneButton", "NEXT action initiated");
  a2dp_sink.next();
}

void goPlay() {
  ESP_LOGI("OneButton", "PLAY action initiated");
  if (a2dp_sink.is_output_active() == true) {
    a2dp_sink.pause();
  } else {
    a2dp_sink.play();
  }
}

void goPrev() {
  ESP_LOGI("OneButton", "PREV action initiated");
  a2dp_sink.previous();
}

void connectionStatusChanged(esp_a2d_connection_state_t state, void *ptr) {
  switch (state) {
    case esp_a2d_connection_state_t::ESP_A2D_CONNECTION_STATE_DISCONNECTED:
      ESP_LOGI("Bluedroid", "A2DP Source Disconnected");
      queuePromptSound(disconnected, disconnected_len);
      digitalWrite(19, true);
      break;
    case esp_a2d_connection_state_t::ESP_A2D_CONNECTION_STATE_CONNECTED:
      ESP_LOGI("Bluedroid", "A2DP Source Connected");
      queuePromptSound(connected, connected_len);
      digitalWrite(19, false);
      break;
    default:
      // Nothing
      break;
  }
}

void volumeUpFine() {
  curVolume = a2dp_sink.get_volume();
  curVolume = curVolume + 1;
    if (curVolume > 127) {
    curVolume = 127;
    queuePromptSound(volmax, volmax_len);
  }
  a2dp_sink.set_volume(curVolume);
}

void volumeUpCoarse() {
  curVolume = a2dp_sink.get_volume();
  curVolume = curVolume + 4;
  if (curVolume > 127) {
    curVolume = 127;
    queuePromptSound(volmax, volmax_len);
  }
  a2dp_sink.set_volume(curVolume);
}

void volumeDownFine() {
  curVolume = a2dp_sink.get_volume();
  curVolume = curVolume - 1;
  if (curVolume <= 0) { 
    curVolume = 0;
    queuePromptSound(volmin, volmin_len);
  }
  a2dp_sink.set_volume(curVolume);
}

void volumeDownCoarse() {
  curVolume = a2dp_sink.get_volume();
  curVolume = curVolume - 4;
  if (curVolume <= 0) { 
    curVolume = 0;
    queuePromptSound(volmin, volmin_len);
  }
  a2dp_sink.set_volume(curVolume);
}

void setupKeys() {
  // Setup buttons
  volup_btn.attachClick(volumeUpFine);
  volup_btn.attachDuringLongPress(volumeUpCoarse);
  next_btn.attachClick(goNext);
  play_btn.attachClick(goPlay);
  prev_btn.attachClick(goPrev);
  voldn_btn.attachClick(volumeDownFine);
  voldn_btn.attachDuringLongPress(volumeDownCoarse);

  // Set delays
  volup_btn.setDebounceMs(100);
  next_btn.setDebounceMs(100);
  play_btn.setDebounceMs(100);
  prev_btn.setDebounceMs(100);
  voldn_btn.setDebounceMs(100);
  volup_btn.setClickMs(500);
  next_btn.setClickMs(500);
  play_btn.setClickMs(500);
  prev_btn.setClickMs(500);
  voldn_btn.setClickMs(500);
  volup_btn.setPressMs(1000);
  next_btn.setPressMs(1000);
  play_btn.setPressMs(1000);
  prev_btn.setPressMs(1000);
  voldn_btn.setPressMs(1000);
}

void setup() {
  Serial.begin(115200);
  auto cfg = kit.defaultConfig();
  a2dp_sink.set_on_connection_state_changed(connectionStatusChanged);
  a2dp_sink.start("ESP-BT-SPK");
  cfg.sample_rate = a2dp_sink.sample_rate();
  cfg.channels = 2;
  kit.begin(cfg);
  setupKeys();
  queuePromptSound(poweron, poweron_len);
  pinMode(19, OUTPUT);
  digitalWrite(19, false);
  delay(100);
  digitalWrite(19, true);
  delay(100);
  digitalWrite(19, false);
  delay(100);
  digitalWrite(19, true);
  delay(100);
}

void loop() {
  volup_btn.tick();
  next_btn.tick();
  play_btn.tick();
  prev_btn.tick();
  voldn_btn.tick();
  yield();
  if (queueCount > 0 && !promptPlaying) {
    playNextPrompt();
  }
  delay(50);
}
