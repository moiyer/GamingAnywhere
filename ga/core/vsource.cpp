/*
 * Copyright (c) 2013 Chun-Ying Huang
 *
 * This file is part of Gaming Anywere (GA).
 *
 * GA is free software; you can redistribute it and/or modify it
 * under the terms of the 3-clause BSD License as published by the
 * Free Software Foundation: http://directory.fsf.org/wiki/License:BSD_3Clause
 *
 * GA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the 3-clause BSD License along with GA;
 * if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <map>

#include "vsource.h"
#include "ga-common.h"

#define	POOLSIZE			8

// golbal image structure
static int gChannels;
static int gWidth[IMAGE_SOURCE_CHANNEL_MAX];
static int gHeight[IMAGE_SOURCE_CHANNEL_MAX];
static int gStride[IMAGE_SOURCE_CHANNEL_MAX];
static pipeline *gPipe[IMAGE_SOURCE_CHANNEL_MAX];

struct vsource_frame *
vsource_frame_init(struct vsource_frame *frame, int width, int height, int stride) {
	int i;
	//
	bzero(frame, sizeof(struct vsource_frame));
	//
	for(i = 0; i < MAX_STRIDE; i++) {
		frame->linesize[i] = stride;
	}
	frame->stride = stride;
	frame->imgbufsize = height * stride;
	if(ga_malloc(frame->imgbufsize, (void**) &frame->imgbuf_internal, &frame->alignment) < 0) {
		return NULL;
	}
	frame->imgbuf = frame->imgbuf_internal + frame->alignment;
	bzero(frame->imgbuf, frame->imgbufsize);
	return frame;
}

void
vsource_frame_release(struct vsource_frame *frame) {
	if(frame == NULL)
		return;
	if(frame->imgbuf != NULL)
		free(frame->imgbuf);
	return;
}

int
video_source_channels() {
	return gChannels;
}

int
video_source_width(int channel) {
	return gWidth[channel];
}

int
video_source_height(int channel) {
	return gHeight[channel];
}

int
video_source_stride(int channel) {
	return gStride[channel];
}

const char *
video_source_get_pipename(int channel) {
	return gPipe[channel]->name();
}

int
video_source_setup_ex(const char *pipeformat, struct vsource_config *config, int nConfig) {
	int idx;
	//
	if(config==NULL || nConfig <=0) {
		ga_error("image source: invalid image source configuration (%d,%p)\n",
			nConfig, config);
		return -1;
	}
	if(nConfig > IMAGE_SOURCE_CHANNEL_MAX) {
		ga_error("image source: too many sources (%d > %d)\n",
			nConfig, IMAGE_SOURCE_CHANNEL_MAX);
		return -1;
	}
	for(idx = 0; idx < nConfig; idx++) {
		struct pooldata *data = NULL;
		int width  = config[idx].width;
		int height = config[idx].height;
		int stride = config[idx].stride;
		char pipename[64];
		//
		gWidth[idx]  = width;
		gHeight[idx] = height;
		gStride[idx] = stride;
		// create pipe
		if((gPipe[idx] = new pipeline()) == NULL) {
			ga_error("image source: init pipeline failed.\n");
			return -1;
		}
		if(gPipe[idx]->alloc_privdata(sizeof(struct vsource_config)) == NULL) {
			ga_error("image source: cannot allocate private data.\n");
			delete gPipe[idx];
			gPipe[idx] = NULL;
			return -1;
		}
		config[idx].id = idx;
		gPipe[idx]->set_privdata(&config[idx], sizeof(struct vsource_config));
		// create data pool for the pipe
		if((data = gPipe[idx]->datapool_init(POOLSIZE, sizeof(struct vsource_frame))) == NULL) {
			ga_error("image source: cannot allocate data pool.\n");
			delete gPipe[idx];
			gPipe[idx] = NULL;
			return -1;
		}
		// per frame init
		for(; data != NULL; data = data->next) {
			if(vsource_frame_init((struct vsource_frame*) data->ptr, width, height, stride) == NULL) {
				ga_error("image source: init frame failed.\n");
				return -1;
			}
		}
		//
		snprintf(pipename, sizeof(pipename), pipeformat, idx);
		if(pipeline::do_register(pipename, gPipe[idx]) < 0) {
			ga_error("image source: register pipeline failed (%s)\n",
					pipename);
			return -1;
		}
	}
	//
	gChannels = idx;
	//
	return 0;
}

int
video_source_setup(const char *pipeformat, int channel_id, int width, int height, int stride) {
	vsource_config config;
	bzero(&config, sizeof(config));
	config.rtp_id = channel_id;
	config.width = width;
	config.height = height;
	config.stride = stride;
	return video_source_setup_ex(pipeformat, &config, 1);
}

