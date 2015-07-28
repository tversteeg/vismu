#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <pthread.h>

#include <alsa/asoundlib.h>

#define BUFFER_SIZE 2048

typedef struct {
	int format, out[BUFFER_SIZE];
	unsigned int rate;
	char *source;
} audiodata_t;

void* input(void *data)
{
	audiodata_t *audio = (audiodata_t*)data;

	snd_pcm_t *handle;
	int status = snd_pcm_open(&handle, audio->source, SND_PCM_STREAM_CAPTURE, 0);
	if(status < 0){
		fprintf(stderr, "Error opening stream: %s\n", snd_strerror(status));
		exit(EXIT_FAILURE);
	}

	return NULL;
}

int main(int argc, char **argv)
{
	audiodata_t audio;

	int i;
	for(i = 0; i < BUFFER_SIZE; i++){
		audio.out[i] = 0;
	}

	int c;
	while((c = getopt(argc, argv, "s")) != -1){
		switch(c){
			case 's':
				audio.source = optarg;
				break;
		}
	}

	pthread_t thread;
	int id = pthread_create(&thread, NULL, input, (void*)&audio);

	return 0;
}
