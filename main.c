#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <getopt.h>

#include <ccore/file.h>
#include <ccore/display.h>
#include <ccore/window.h>
#include <ccore/opengl.h>
#include <ccore/event.h>
#include <ccore/string.h>
#include <ccore/error.h>

#include <GL/glew.h>

#include <fftw3.h>
#include <pthread.h>
#include <alsa/asoundlib.h>

#define BUFFER_SIZE 2048
#define LISTEN_BUFFER_SIZE 128

typedef struct {
	int format, out[BUFFER_SIZE];
	unsigned int rate;
	char *source;
} audiodata_t;

pthread_mutex_t mutex;
pthread_cond_t cond;
bool threaddone;

GLuint loadShader(char *file, GLenum type)
{
	FILE *fp;
	char *buf;
	GLuint shader;
	GLint status, logLength;
	ccFileInfo fi;

	fi = ccFileInfoGet(file);
	if(!fi.size){
		return 0;
	}
	buf = malloc(fi.size);

	fp = fopen(file, "rb");
	fread(buf, 1, fi.size, fp);
	fclose(fp);

	buf[fi.size] = '\0';

	shader = glCreateShader(type);
	glShaderSource(shader, 1, (const char**)&buf, NULL);
	glCompileShader(shader);

	free(buf);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if(status == GL_FALSE){
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

		buf = (char*)malloc(logLength + 1);
		glGetShaderInfoLog(shader, logLength, NULL, buf);

		printf("%s shader error:\n\t%s", (type == GL_VERTEX_SHADER) ? "Vertex" : "Fragment", buf);

		free(buf);	
	}

	return shader;
}

GLuint loadProgram()
{
	GLuint program, vertex, fragment;

	fragment = loadShader(ccStringConcatenate(2, ccFileDataDirGet(), "fragment.glsl"), GL_FRAGMENT_SHADER);
	vertex = loadShader(ccStringConcatenate(2, ccFileDataDirGet(), "vertex.glsl"), GL_VERTEX_SHADER);

	program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);

	glBindAttribLocation(program, 0, "position");
	glLinkProgram(program);

	return program;
}

void loadScreenTriangles(GLuint *vao, GLuint *vbo)
{
	GLfloat vertices[] = {
		-1,-1, 0,
		1,-1, 0,
		-1, 1, 0,
		1,-1, 0,
		-1, 1, 0,
		1, 1, 0,
	};

	glGenVertexArrays(1, vao);
	glBindVertexArray(*vao);

	glGenBuffers(1, vbo);
	glBindBuffer(GL_ARRAY_BUFFER, *vbo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

double rootMeanSquare(const short *buf, size_t len)
{
	long int sum = 0;
	size_t i;
	for(i = 0; i < len; i++){
		sum += buf[i] * buf[i];
	}

	return sqrt(sum / len);
}

int alsaSetHwParams(snd_pcm_t *handle, snd_pcm_hw_params_t *params, int *dir, snd_pcm_uframes_t *frames)
{
	snd_pcm_hw_params_any(handle, params);
	if(snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0){
#ifdef VISMU_DEBUG
		fprintf(stderr, "Error setting access\n");
#endif
		return -1;
	}
	if(snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE) < 0){
#ifdef VISMU_DEBUG
		fprintf(stderr, "Error setting format\n");
#endif
		return -1;
	}
	if(snd_pcm_hw_params_set_channels(handle, params, 2) < 0){
#ifdef VISMU_DEBUG
		fprintf(stderr, "Error setting channels\n");
#endif
		return -1;
	}
	unsigned int rate = 44100;
	if(snd_pcm_hw_params_set_rate_near(handle, params, &rate, dir) < 0){
#ifdef VISMU_DEBUG
		fprintf(stderr, "Error setting rate\n");
#endif
		return -1;
	}
	snd_pcm_hw_params_set_period_size_near(handle, params, frames, dir);

	int status = snd_pcm_hw_params(handle, params);
	if(status < 0){
#ifdef VISMU_DEBUG
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

	snd_pcm_t *handle;
	int status = snd_pcm_open(&handle, name, SND_PCM_STREAM_CAPTURE, 0);
	if(status < 0){
#ifdef VISMU_DEBUG
		fprintf(stderr, "Error opening stream: %s\n", snd_strerror(status));
#endif
		return -1;
	}

	snd_pcm_hw_params_t *params;
	snd_pcm_hw_params_malloc(&params);

	int dir = 0;
	snd_pcm_uframes_t frames = 50000;
	if(alsaSetHwParams(handle, params, &dir, &frames) < 0){
		snd_pcm_close(handle);
		return -1;
	}

	snd_pcm_hw_params_get_period_size(params, &frames, &dir);

	snd_pcm_hw_params_free(params);

	size_t size = frames << 1;
	if(size <= 0 || size < frames){
		snd_pcm_close(handle);
#ifdef VISMU_DEBUG
		fprintf(stderr, "Invalid buffer size: %d\n", size);
#endif
		return -1;
	}
#ifdef VISMU_DEBUG
	printf("Buffer size: %d\n", size);
#endif

	short *buf = (short*)calloc(size, sizeof(short));
	double peak = 0.0;
	int i = 0;
	while(i++ < 10){
		int result;
		while((result = snd_pcm_readi(handle, buf, frames))< 0){ // Buffer size / channels / 2
			snd_pcm_prepare(handle);
		}

		if(result == -EPIPE){
#ifdef VISMU_DEBUG
			fprintf(stderr, "Buffer overrun\n");
#endif
			snd_pcm_prepare(handle);
			continue;
		}else if(result < 0){
#ifdef VISMU_DEBUG
			fprintf(stderr, "Read error: %s\n", snd_strerror(result));
#endif
		}

		double val = rootMeanSquare(buf, size);
		if(peak < val){
			peak = val;
		}
	}

	snd_pcm_drain(handle);
	snd_pcm_close(handle);

	free(buf);

	return 20 * log10(peak);
}

void alsaSetDefaultInput(audiodata_t *audio, int best)
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
#ifdef VISMU_DEBUG
	printf("List of %s hardware devices:\n", snd_pcm_stream_name(SND_PCM_STREAM_PLAYBACK));
#endif

	int highnum = 0, highcard = -1, highdev = -1;
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
#ifdef VISMU_DEBUG
			printf("%i: %s [%s], device %i: %s [%s]\n", card, snd_ctl_card_info_get_id(info), snd_ctl_card_info_get_name(info), dev, snd_pcm_info_get_id(pcminfo), snd_pcm_info_get_name(pcminfo));
#endif
			int peak = alsaListenDevice(card, dev);
			if(peak <= 0 || peak < highnum){
				continue;
			}

			highnum = peak;
			highcard = card;
			highdev = dev;

			if(!best){
				goto done;
			}
		}

		snd_ctl_close(handle);

		if(snd_card_next(&card) < 0){
			exit(1);
		}
	}

done:
	if(highnum < 0){
		fprintf(stderr, "Could not find default input device!");
		exit(1);
	}

#ifdef VISMU_DEBUG
	printf("Found device \"hw:%d,%d\" with a peak of %d\n", highcard, highdev, highnum);
#endif

	audio->source = (char*)malloc(32);
	sprintf(audio->source, "hw:%d,%d", highcard, highdev);
}

void* input(void *data)
{
	audiodata_t *audio = (audiodata_t*)data;

	snd_pcm_t *handle;
	int status = snd_pcm_open(&handle, audio->source, SND_PCM_STREAM_CAPTURE, 0);
	if(status < 0){
#ifdef VISMU_DEBUG
		fprintf(stderr, "Error opening stream: %s\n", snd_strerror(status));
#endif
		exit(1);
	}

	int dir = 0;
	snd_pcm_uframes_t frames = 256;
	snd_pcm_hw_params_t *params;
	snd_pcm_hw_params_malloc(&params);
	if(alsaSetHwParams(handle, params, &dir, &frames) < 0){
		exit(1);
	}

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
	snd_pcm_hw_params_get_period_size(params, &frames, &dir);
	unsigned int time;
	snd_pcm_hw_params_get_period_time(params, &time, &dir);

	snd_pcm_hw_params_free(params);

	int size = frames * (audio->format >> 3) << 1;
#ifdef VISMU_DEBUG
	printf("Buffer size: %d\n", size);
#endif
	char *buf = (char*)calloc(size, 1);

	int adjustr = audio->format >> 2;
	int adjustl = audio->format >> 3;

	int framecount = 0;
	while(!threaddone){
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

			if(framecount >= BUFFER_SIZE - 1){
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

	int c, best = 0;
	while((c = getopt(argc, argv, "bd:")) != -1){
		switch(c){
			case 'd':
				audio.source = optarg;
				break;
			case 'b':
				best = 1;
				break;
		}
	}

	if(audio.source == NULL){
#ifdef VISMU_DEBUG
		printf("No valid ALSA source is supplied, finding default active input\n");
#endif
		alsaSetDefaultInput(&audio, best);
	}

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
	threaddone = false;

	pthread_t thread;
	pthread_create(&thread, NULL, input, (void*)&audio);

	double in[BUFFER_SIZE];
	fftw_complex out[BUFFER_SIZE];
	fftw_plan plan = fftw_plan_dft_r2c_1d(BUFFER_SIZE, in, out, FFTW_MEASURE);

	ccDisplayInitialize();
	ccWindowCreate((ccRect){.x = 0, .y = 0, .width = 800, .height = 600}, "vismu", 0);
	//	ccGLContextBind();

	glewInit();

	bool loop = true;
	while(loop){
		while(ccWindowEventPoll()){
			ccEvent event = ccWindowEventGet();
			switch(event.type){
				case CC_EVENT_WINDOW_QUIT:
					loop = false;
					break;
				default:
					break;
			}
		}

		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond, &mutex);
		int i;
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

		/*		if(peak > 0){
					printf("Peak: %f\n", peak);
					}*/

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		ccGLBuffersSwap();
	}

	fftw_destroy_plan(plan);

	ccFree();

	threaddone = true;

	pthread_mutex_destroy(&mutex);
	pthread_exit(NULL);
}
