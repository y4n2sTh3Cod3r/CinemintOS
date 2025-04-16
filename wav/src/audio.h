/* audio.h - Audio driver header */
#ifndef AUDIO_H
#define AUDIO_H

/* Initialize the audio hardware */
int audio_init();

/* Play a chunk of audio data */
void audio_play_chunk(const unsigned char* data, unsigned int size);

/* Cleanup audio resources */
void audio_cleanup();

#endif /* AUDIO_H */