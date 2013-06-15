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

#ifndef __ISOURCE_H__
#define __ISOURCE_H__

#include <pthread.h>

#include "ga-common.h"
#include "pipeline.h"

#define	MAX_STRIDE			4
#define	IMAGE_SOURCE_CHANNEL_MAX	8
//#define	ISOURCE_PIPEFORMAT		"image-source-%d"

enum vsource_type {
	rgba = 0,
	yuv420p
};

struct vsource_frame {
	long long imgpts;		// presentation timestamp
	enum vsource_type imgtype;	// rgba or yuv420p
	int linesize[MAX_STRIDE];	// strides for YUV
	// internal data - should not change after initialized
	int stride;
	int imgbufsize;
	unsigned char *imgbuf;
	unsigned char *imgbuf_internal;
	int alignment;
};

struct vsource_config {
	int rtp_id;	// RTP channel id
	int width;
	int height;
	int stride;
	// do not touch - filled by video_source_setup functions
	int id;		// image source id
};

EXPORT struct vsource_frame * vsource_frame_init(struct vsource_frame *frame, int width, int height, int stride);
EXPORT void vsource_frame_release(struct vsource_frame *frame);

EXPORT int video_source_channels();
EXPORT int video_source_width(int channel);
EXPORT int video_source_height(int channel);
EXPORT int video_source_stride(int channel);
EXPORT const char *video_source_pipename(int channel);

EXPORT int video_source_setup_ex(const char *pipeformat, struct vsource_config *config, int nConfig);
EXPORT int video_source_setup(const char *pipeformat, int channel_id, int width, int height, int stride);

#endif
