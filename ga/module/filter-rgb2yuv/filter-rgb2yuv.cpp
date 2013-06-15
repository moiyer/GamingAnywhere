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
#include <pthread.h>
#include <map>

#include "vsource.h"
#include "server.h"
#include "rtspserver.h"
#include "encoder-common.h"

#include "ga-common.h"
#include "ga-conf.h"
#include "ga-avcodec.h"

#include "pipeline.h"
#include "filter-rgb2yuv.h"

#define	POOLSIZE	8

using namespace std;
static pthread_mutex_t initMutex = PTHREAD_MUTEX_INITIALIZER;
static map<void*,bool> initialized;

int
filter_RGB2YUV_init(void *arg) {
	// arg is image source id
	int iid;
	int iwidth;
	int iheight;
	int istride;
	//char pipename[64];
	const char **filterpipe = (const char **) arg;
	//pipeline *srcpipe = (pipeline*) arg;
	pipeline *srcpipe = pipeline::lookup(filterpipe[0]);
	pipeline *pipe = NULL;
	struct pooldata *data = NULL;
	//
	//
	map<void*,bool>::iterator mi;
	pthread_mutex_lock(&initMutex);
	if((mi = initialized.find(arg)) != initialized.end()) {
		if(mi->second != false) {
			// has been initialized
			pthread_mutex_unlock(&initMutex);
			return 0;
		}
	}
	pthread_mutex_unlock(&initMutex);
	//
	if(srcpipe == NULL) {
		ga_error("RGB2YUV filter: init - NULL pipeline specified (%s).\n", filterpipe[0]);
		goto init_failed;
	}
	iid = ((struct vsource_config*) srcpipe->get_privdata())->id;
	iwidth = video_source_width(iid);
	iheight = video_source_height(iid);
	istride = video_source_stride(iid);
	//
	if((pipe = new pipeline()) == NULL) {
		ga_error("RGB2YUV filter: init pipeline failed.\n");
		goto init_failed;
	}
	// has privdata from the source?
	if(srcpipe->get_privdata_size() > 0) {
		if(pipe->alloc_privdata(srcpipe->get_privdata_size()) == NULL) {
			ga_error("RGB2YUV filter: cannot allocate privdata.\n");
			goto init_failed;
		}
		pipe->set_privdata(srcpipe->get_privdata(), srcpipe->get_privdata_size());
	}
	//
	if((data = pipe->datapool_init(POOLSIZE, sizeof(struct vsource_frame))) == NULL) {
		ga_error("RGB2YUV filter: cannot allocate data pool.\n");
		goto init_failed;
	}
	// per frame init
	for(; data != NULL; data = data->next) {
		if(vsource_frame_init((struct vsource_frame*) data->ptr, iwidth, iheight, istride) == NULL) {
			ga_error("RGB2YUV filter: init frame failed.\n");
			goto init_failed;
		}
	}
	//
	//snprintf(pipename, sizeof(pipename), F_RGB2YUV_PIPEFORMAT, iid);
	//pipeline::do_register(pipename, pipe);
	pipeline::do_register(filterpipe[1], pipe);
	//
	pthread_mutex_lock(&initMutex);
	initialized[arg] = true;
	pthread_mutex_unlock(&initMutex);
	//
	return 0;
init_failed:
	if(pipe) {
		delete pipe;
	}
	return -1;
}

void *
filter_RGB2YUV_threadproc(void *arg) {
	// arg is pointer to source pipe
	//char pipename[64];
	const char **filterpipe = (const char **) arg;
	//pipeline *srcpipe = (pipeline*) arg;
	pipeline *srcpipe = pipeline::lookup(filterpipe[0]);
	pipeline *dstpipe = NULL;
	struct pooldata *srcdata = NULL;
	struct pooldata *dstdata = NULL;
	struct vsource_frame *srcframe = NULL;
	struct vsource_frame *dstframe = NULL;
	// image info
	int iid;
	int iwidth;
	int iheight;
	//int istride = video_source_stride();
	//
	unsigned char *src[] = { NULL, NULL, NULL, NULL };
	unsigned char *dst[] = { NULL, NULL, NULL, NULL };
	int srcstride[] = { 0, 0, 0, 0 };
	int dststride[] = { 0, 0, 0, 0 };
	int pic_size;
	//
	struct SwsContext *swsctx = NULL;
	//
	pthread_mutex_t condMutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	//
	if(srcpipe == NULL) {
		ga_error("RGB2YUV filter: NULL pipeline specified.\n");
		goto filter_quit;
	}
	// init variables
	iid = ((struct vsource_config*) srcpipe->get_privdata())->id;
	iwidth = video_source_width(iid);
	iheight = video_source_height(iid);
	pic_size = iwidth * iheight + iwidth * iheight / 4 + iwidth * iheight / 4;
	//
	//snprintf(pipename, sizeof(pipename), F_RGB2YUV_PIPEFORMAT, iid);
	//if((dstpipe = pipeline::lookup(pipename)) == NULL) {
	if((dstpipe = pipeline::lookup(filterpipe[1])) == NULL) {
		ga_error("RGB2YUV filter: cannot find pipeline '%s'\n", filterpipe[1]/*pipename*/);
		goto filter_quit;
	}
	//
	ga_error("RGB2YUV filter: pipe from '%s' to '%s' (%dx%d, picsize=%d)\n",
		srcpipe->name(), dstpipe->name(),
		iwidth, iheight, pic_size);
	//
	do {
		char pixelfmt[64];
		if(ga_conf_readv("filter-source-pixelformat", pixelfmt, sizeof(pixelfmt)) != NULL) {
			if(strcasecmp("rgba", pixelfmt) == 0) {
				swsctx = ga_swscale_init(PIX_FMT_RGBA, iwidth, iheight, iwidth, iheight);
				ga_error("RGB2YUV filter: RGBA source specified.\n");
			} else if(strcasecmp("bgra", pixelfmt) == 0) {
				swsctx = ga_swscale_init(PIX_FMT_BGRA, iwidth, iheight, iwidth, iheight);
				ga_error("RGB2YUV filter: BGRA source specified.\n");
			}
		}
		if(swsctx == NULL) {
#ifdef __APPLE__
			swsctx = ga_swscale_init(PIX_FMT_RGBA, iwidth, iheight, iwidth, iheight);
#else
			swsctx = ga_swscale_init(PIX_FMT_BGRA, iwidth, iheight, iwidth, iheight);
#endif
		}
	} while(0);
	if(swsctx == NULL) {
		ga_error("RGB2YUV filter: cannot initialize swsscale.\n");
		goto filter_quit;
	}
	//
	srcpipe->client_register(ga_gettid(), &cond);
	// start filtering
	ga_error("RGB2YUV filter started: tid=%ld.\n", ga_gettid());
	//
	while(true) {
		// wait for notification
		srcdata = srcpipe->load_data();
		if(srcdata == NULL) {
			srcpipe->wait(&cond, &condMutex);
			srcdata = srcpipe->load_data();
			if(srcdata == NULL) {
				ga_error("RGB2YUV filter: unexpected NULL frame received (from '%s', data=%d, buf=%d).\n",
					srcpipe->name(), srcpipe->data_count(), srcpipe->buf_count());
				exit(-1);
				// should never be here
				goto filter_quit;
			}
		}
		srcframe = (struct vsource_frame*) srcdata->ptr;
		//
		dstdata = dstpipe->allocate_data();
		dstframe = (struct vsource_frame*) dstdata->ptr;
		// basic info
		dstframe->imgpts = srcframe->imgpts;
		dstframe->imgtype = yuv420p;
		// scale image
		if(srcframe->imgtype == rgba) {
			src[0] = srcframe->imgbuf;
			src[1] = NULL;
			srcstride[0] = srcframe->stride;
			srcstride[1] = 0;
			dst[0] = dstframe->imgbuf;
			dst[1] = dstframe->imgbuf + iheight*iwidth;
			dst[2] = dstframe->imgbuf + iheight*iwidth + (iheight*iwidth>>2);
			dst[3] = NULL;
			dstframe->linesize[0] = dststride[0] = iwidth;
			dstframe->linesize[1] = dststride[1] = iwidth>>1;
			dstframe->linesize[2] = dststride[2] = iwidth>>1;
			dstframe->linesize[3] = dststride[3] = 0;
			sws_scale(swsctx, src, srcstride, 0, iheight, dst, dstframe->linesize);
		} else if(srcframe->imgtype == yuv420p) {
			// assume format is equivalent - copy it
			dstframe->linesize[0] = srcframe->linesize[0];
			dstframe->linesize[1] = srcframe->linesize[1];
			dstframe->linesize[2] = srcframe->linesize[2];
			bcopy(srcframe->imgbuf, dstframe->imgbuf, pic_size);
		}
		srcpipe->release_data(srcdata);
		dstpipe->store_data(dstdata);
		dstpipe->notify_all();
		//
	}
	//
filter_quit:
	if(srcpipe) {
		srcpipe->client_unregister(ga_gettid());
		srcpipe = NULL;
	}
	if(dstpipe) {
		delete dstpipe;
		dstpipe = NULL;
	}
	//
	if(swsctx)	sws_freeContext(swsctx);
	//
	ga_error("RGB2YUV filter: thread terminated.\n");
	//
	return NULL;
}

