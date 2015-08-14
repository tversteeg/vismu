#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <getopt.h>

#include <fftw3.h>
#include <pthread.h>
#include <alsa/asoundlib.h>

#define BUFFER_SIZE 2048
#define LISTEN_BUFFER_SIZE 128

#define DEBUG

typedef struct {
	int format, out[BUFFER_SIZE];
	unsigned int rate;
	char *source;
} audiodata_t;

pthread_mutex_t mutex;
pthread_cond_t cond;

double rootMeanSquare(short *buffer)
{
	long int sum = 0.0;
	int i;
	for(i = 0; i < LISTEN_BUFFER_SIZE; i++){
		sum += buffer[i] * buffer[i];
	}

	return sqrt(sum / LISTEN_BUFFER_SIZE);
}

int alsaGetCaptureHandle(char *name, snd_pcm_t **handle, snd_pcm_hw_params_t **params, int *dir, snd_pcm_uframes_t frames)
{
	int status = snd_pcm_open(handle, name, SND_PCM_STREAM_CAPTURE, 0);
	if(status < 0){
#ifdef DEBUG
		fprintf(stderr, "Error opening stream: %s\n", snd_strerror(status));
#endif
		return -1;
	}

	snd_pcm_hw_params_alloca(params);
	snd_pcm_hw_params_any(*handle, *params);
	if(snd_pcm_hw_params_set_access(*handle, *params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0){
#ifdef DEBUG
		fprintf(stderr, "Error setting access\n");
#endif
		return -1;
	}
	if(snd_pcm_hw_params_set_format(*handle, *params, SND_PCM_FORMAT_S16_LE) < 0){
#ifdef DEBUG
		fprintf(stderr, "Error setting format\n");
#endif
		return -1;
	}
	if(snd_pcm_hw_params_set_channels(*handle, *params, 2) < 0){
#ifdef DEBUG
		fprintf(stderr, "Error setting channels\n");
#endif
		return -1;
	}
	unsigned int rate = 44100;
	if(snd_pcm_hw_params_set_rate_near(*handle, *params, &rate, dir) < 0){
#ifdef DEBUG
		fprintf(stderr, "Error setting rate\n");
#endif
		return -1;
	}
	snd_pcm_uframes_t framesset = frames;
	snd_pcm_hw_params_set_period_size_near(*handle, *params, &framesset, dir);

	status = snd_pcm_hw_params(*handle, *params);
	if(status < 0){
		snd_pcm_close(*handle);
#ifdef DEBUG
		fprintf(stderr, "Unable to set hardware parameters: %s\n", snd_strerror(status));
#endif
		return -1;
	}

	return 0;
}

int alsaListenDevice(int card, int dev)
{
	char name[32];
	sprintf(name, "hw:%d,%d", card, dev);

	int dir;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	if(alsaGetCaptureHandle(name, &handle, &params, &dir, 50000) < 0){
		return -1;
	}
	
	short buffer[LISTEN_BUFFER_SIZE];
	double peak = 0.0;
	int i = 0;
	while(i++ < 10){
		int frames;
		while((frames = snd_pcm_readi(handle, buffer, LISTEN_BUFFER_SIZE / 4))< 0){ // Buffer size / channels / 2
			snd_pcm_prepare(handle);
		}

		if(frames == -EPIPE){
#ifdef DEBUG
			fprintf(stderr, "Buffer overrun\n");
#endif
			snd_pcm_prepare(handle);
			continue;
		}else if(frames < 0){
#ifdef DEBUG
			fprintf(stderr, "Read error: %s\n", snd_strerror(frames));
#endif
		}

		double val = rootMeanSquare(buffer);
		if(peak < val){
			peak = val;
		}
	}

	snd_pcm_drain(handle);
	snd_pcm_close(handle);

	return 20 * log10(peak);
}

void alsaSetDefaultInput(audiodata_t *audio)
{
	snd_ctl_card_info_t *info;
	snd_ctl_card_info_alloca(&info);
	snd_pcm_info_t *pcminfo;
	snd_pcm_info_alloca(&pcminfo);

	int card = -1;
	if(snd_card_next(&card) < 0 || card < 0){
		fprintf(stderr, "Error: no soundcard found\n");
		exit(1);
	}
#ifdef DEBUG
	printf("List of %s hardware devices:\n", snd_pcm_stream_name(SND_PCM_STREAM_PLAYBACK));
#endif
	while(card >= 0){
		char name[32];
		sprintf(name, "hw:%d", card);
		
		snd_ctl_t *handle;
		snd_ctl_open(&handle, name, 0);
		snd_ctl_card_info(handle, info);

		int dev = -1;
		while(1){
			snd_ctl_pcm_next_device(handle, &dev);
			if(dev < 0){
				break;
			}

			snd_pcm_info_set_device(pcminfo, dev);
			snd_pcm_info_set_subdevice(pcminfo, 0);
			snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_PLAYBACK);
			if(snd_ctl_pcm_info(handle, pcminfo) < 0){
				continue;
			}
#ifdef DEBUG
			printf("%i: %s [%s], device %i: %s [%s]\n", card, snd_ctl_card_info_get_id(info), snd_ctl_card_info_get_name(info), dev, snd_pcm_info_get_id(pcminfo), snd_pcm_info_get_name(pcminfo));
#endif
			if(alsaListenDevice(card, dev) <= 0){
				continue;
			}

			sprintf(audio->source, "hw:%d,%d", card, dev);
		}

		snd_ctl_close(handle);

		if(snd_card_next(&card) < 0){
			exit(1);
		}
	}
}

void* input(void *data)
{
	audiodata_t *audio = (audiodata_t*)data;

	int dir;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	alsaGetCaptureHandle(audio->source, &handle, &params, &dir, 256);
	
	snd_pcm_format_t format;
	snd_pcm_hw_params_get_format(params, &format);

	if(format < 6){
		audio->format = 16;
	}else if(format < 10){
		audio->format = 24;
	}else{
		audio->format = 32;
	}

	snd_pcm_hw_params_get_rate(params, &audio->rate, &dir);
	snd_pcm_uframes_t frames = 256;
	snd_pcm_hw_params_get_period_size(params, &frames, &dir);
	unsigned int time;
	snd_pcm_hw_params_get_period_time(params, &time, &dir);

	int size = frames * (audio->format >> 3) << 1;
	printf("Buffer size: %d\n", size);
	char *buf = (char*)malloc(size);

	int adjustr = audio->format >> 2;
	int adjustl = audio->format >> 3;

	int framecount = 0;
	while(1){
		int status = snd_pcm_readi(handle, buf, frames);
		if(status == -EPIPE){
			snd_pcm_prepare(handle);
		}

		pthread_mutex_lock(&mutex);

		int i;
		for(i = 0; i < size; i = i + (adjustl << 1)){
			int right = buf[i + adjustr - 1] << 2;
			int lo = buf[i + adjustr - 2] >> 6; // lo = ?
			if(lo < 0){
				lo = -lo + 1;
			}
			if(right >= 0){
				right += lo;
			}else{
				right -= lo;
			}

			int left = buf[i + adjustl - 1] << 2;
			lo = buf[i + adjustl - 2] >> 6; // lo = ?
			if(lo < 0){
				lo = -lo + 1;
			}
			if(left >= 0){
				left += lo;
			}else{
				left -= lo;
			}

			audio->out[framecount++] = (right + left) >> 1;

			if(framecount == BUFFER_SIZE - 1){
				framecount = 0;
			}
		}

		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mutex);
	}

	return NULL;
}

int main(int argc, char **argv)
{
	audiodata_t audio = {.rate = 0, .format = -1, .source = NULL};

	int i;
	for(i = 0; i < BUFFER_SIZE; i++){
		audio.out[i] = 0;
	}

	int c;
	while((c = getopt(argc, argv, ":d:")) != -1){
		switch(c){
			case 'd':
				audio.source = optarg;
				break;
		}
	}

	if(audio.source == NULL){
#ifdef DEBUG
		printf("No valid ALSA source is supplied, finding default active input\n");
#endif
		alsaSetDefaultInput(&audio);
	}

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);

	pthread_t thread;
	pthread_create(&thread, NULL, input, (void*)&audio);

	double in[BUFFER_SIZE];
	fftw_complex out[BUFFER_SIZE];
	fftw_plan plan = fftw_plan_dft_r2c_1d(BUFFER_SIZE, in, out, FFTW_MEASURE);

	while(1){
		int i;
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond, &mutex);
		for (i = 0; i < BUFFER_SIZE; i++) {
			in[i] = audio.out[i];
		}
		pthread_mutex_unlock(&mutex);

		fftw_execute(plan);

		double peak = 0;
		for(i = 0; i < BUFFER_SIZE >> 1; i++){
			double real = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
			if(real > peak){
				peak = real;
			}
		}

		if(peak > 0){
			printf("Peak: %f\n", peak);
		}
	}

	fftw_destroy_plan(plan);

	pthread_mutex_destroy(&mutex);
	pthread_exit(NULL);
}
