#pragma once
#include <stdio.h>
#include <stdint.h>

#include "parse_video.h"

void init_audio_alsa();
void fill_audio_alsa(int frequency);
void parse_audio_frame_alsa(uint16_t length);
void shutdown_audio_alsa();