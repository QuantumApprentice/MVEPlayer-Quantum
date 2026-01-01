#pragma once
#include <stdio.h>
#include <stdint.h>

#include "parse_audio.h"

void init_audio_alsa(audio_handle* audio);
void fill_audio_alsa(audio_handle* audio);
void parse_audio_frame_alsa(audio_handle* audio, uint16_t length);
void play_alsa(bool pause);
void shutdown_audio_alsa();