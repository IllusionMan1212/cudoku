#include <stdio.h>
#include <stdbool.h>

#include "3rdparty/fmod/include/fmod.h"

FMOD_SYSTEM *fmod_system;
FMOD_CHANNELGROUP *sfx_channel_group;
FMOD_SOUND *scribble_sound;
FMOD_SOUND *win_sound;

int audio_init() {
  // create fmod_system
  FMOD_RESULT fmod_res = FMOD_System_Create(&fmod_system, FMOD_VERSION);
  if (fmod_res != FMOD_OK) {
    printf("Failed to create FMOD system\n");
    return 1;
  };

  // init FMOD
  FMOD_System_Init(fmod_system, 512, FMOD_INIT_NORMAL, NULL);

  // create channel group
  fmod_res = FMOD_System_CreateChannelGroup(fmod_system, "SFX", &sfx_channel_group);
  if (fmod_res != FMOD_OK) {
    printf("Failed to create \"SFX\" channel group\n");
    return 1;
  };

  fmod_res = FMOD_System_CreateSound(fmod_system, "./assets/audio/scribble.wav", FMOD_DEFAULT, NULL, &scribble_sound);
  if (fmod_res != FMOD_OK) {
    printf("Failed to create \"click\" sound\n");
    return 1;
  };

  fmod_res = FMOD_System_CreateSound(fmod_system, "./assets/audio/win.mp3", FMOD_DEFAULT, NULL, &win_sound);
  if (fmod_res != FMOD_OK) {
    printf("Failed to create \"win\" sound\n");
    return 1;
  };

  return 0;
}

void audio_update() {
  FMOD_System_Update(fmod_system);
}

void audio_play_scribble() {
  FMOD_RESULT fmod_res = FMOD_System_PlaySound(fmod_system, scribble_sound, sfx_channel_group, false, NULL);
  if (fmod_res != FMOD_OK) {
    printf("Failed to play \"scribble\" sound\n");
  };
}

void audio_play_win() {
  FMOD_RESULT fmod_res = FMOD_System_PlaySound(fmod_system, win_sound, sfx_channel_group, false, NULL);
  if (fmod_res != FMOD_OK) {
    printf("Failed to play \"win\" sound\n");
  };
}

void audio_cleanup() {
  FMOD_Sound_Release(win_sound);
  FMOD_Sound_Release(scribble_sound);
  FMOD_ChannelGroup_Release(sfx_channel_group);
  FMOD_System_Release(fmod_system);
}
