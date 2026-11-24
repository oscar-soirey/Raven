/**
 * Raven - media/media_avfoundation.m
 * Media players on macOS and iOS (media.h): AVPlayer plays the file or the
 * address, the sound goes to the output of the system on its own, and an
 * AVPlayerItemVideoOutput hands the picture of now as a CVPixelBuffer in
 * BGRA, which is copied into tight RGBA rows for the texture of the player.
 * Compiled with ARC.
 *
 * AVFoundation is not linked: it is opened with dlopen when the first player
 * opens something, and its classes are found by name (as WebKit is, see
 * platform/apple/webkit_web_view.m). It brings CoreMedia, VideoToolbox,
 * AudioToolbox and MediaToolbox with it: a program that plays nothing does
 * not load them. Only the declarations of the headers are used; nothing here
 * makes the linker look for a symbol of AVFoundation, CoreMedia or CoreVideo
 * (the functions of CoreVideo are found with dlsym, the constants are written
 * as their values).
 *
 * Copy or zero-copy: the pixel buffers of AVFoundation are backed by an
 * IOSurface, which Metal could sample without a copy. The texture of a
 * player is an ordinary texture of Raven instead, the same on every backend
 * (Vulkan through MoltenVK, Metal, the null backend of the tests), filled
 * through rvn_texture_replace_pixels: one copy of the picture per new frame,
 * about 8 MB for 1080p, a millisecond or two, only while it is shown.
 *
 * Polled from the UI thread (media.c): the status of the item, its time, its
 * size and its tracks are read when asked. AVFoundation brings some of them
 * up to date from the main queue: where nothing else runs the main run loop
 * (the null platform, headless), a poll lets it run once, without waiting.
 */

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>
#include <dlfcn.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>

#include "media.h"

#if !__has_feature(objc_arc)
#error "media_avfoundation.m is compiled with -fobjc-arc"
#endif

/* The functions of CoreVideo, found in the process once AVFoundation is loaded. */
typedef CVReturn (*rvn_cv_lock_fn)(CVPixelBufferRef, CVPixelBufferLockFlags);
typedef void* (*rvn_cv_base_fn)(CVPixelBufferRef);
typedef size_t (*rvn_cv_size_fn)(CVPixelBufferRef);
typedef OSType (*rvn_cv_format_fn)(CVPixelBufferRef);

static struct {
	int tried;
	int available;
	Class player_class, item_class, asset_class, output_class;
	rvn_cv_lock_fn lock, unlock;
	rvn_cv_base_fn base;
	rvn_cv_size_fn width, height, row_bytes;
	rvn_cv_format_fn format;
} g_rvn_av;

static int rvn_av_available(void) {
	if (!g_rvn_av.tried) {
		void* library;
		g_rvn_av.tried = 1;
		library = dlopen("/System/Library/Frameworks/AVFoundation.framework/AVFoundation", RTLD_LAZY | RTLD_LOCAL);
		/* (the handle stays open: the classes live as long as the program) */
		if (library) {
			g_rvn_av.player_class = NSClassFromString(@"AVPlayer");
			g_rvn_av.item_class = NSClassFromString(@"AVPlayerItem");
			g_rvn_av.asset_class = NSClassFromString(@"AVURLAsset");
			g_rvn_av.output_class = NSClassFromString(@"AVPlayerItemVideoOutput");
			g_rvn_av.lock = (rvn_cv_lock_fn)dlsym(RTLD_DEFAULT, "CVPixelBufferLockBaseAddress");
			g_rvn_av.unlock = (rvn_cv_lock_fn)dlsym(RTLD_DEFAULT, "CVPixelBufferUnlockBaseAddress");
			g_rvn_av.base = (rvn_cv_base_fn)dlsym(RTLD_DEFAULT, "CVPixelBufferGetBaseAddress");
			g_rvn_av.width = (rvn_cv_size_fn)dlsym(RTLD_DEFAULT, "CVPixelBufferGetWidth");
			g_rvn_av.height = (rvn_cv_size_fn)dlsym(RTLD_DEFAULT, "CVPixelBufferGetHeight");
			g_rvn_av.row_bytes = (rvn_cv_size_fn)dlsym(RTLD_DEFAULT, "CVPixelBufferGetBytesPerRow");
			g_rvn_av.format = (rvn_cv_format_fn)dlsym(RTLD_DEFAULT, "CVPixelBufferGetPixelFormatType");
		}
		g_rvn_av.available = library && g_rvn_av.player_class && g_rvn_av.item_class && g_rvn_av.asset_class && g_rvn_av.output_class &&
			g_rvn_av.lock && g_rvn_av.unlock && g_rvn_av.base && g_rvn_av.width && g_rvn_av.height && g_rvn_av.row_bytes && g_rvn_av.format;
	}
	return g_rvn_av.available;
}

/* Seconds of a CMTime; -1 when it is not a number of seconds (invalid, indefinite). */
static double rvn_av_seconds(CMTime time) {
	if (!(time.flags & kCMTimeFlags_Valid) || (time.flags & (kCMTimeFlags_Indefinite | kCMTimeFlags_PositiveInfinity |
		kCMTimeFlags_NegativeInfinity)) || time.timescale <= 0) return -1.0;
	return (double)time.value / (double)time.timescale;
}

static CMTime rvn_av_time(double seconds) {
	CMTime time;
	time.value = (CMTimeValue)llround(seconds * 600000.0);
	time.timescale = 600000;
	time.flags = kCMTimeFlags_Valid;
	time.epoch = 0;
	return time;
}

static CMTime rvn_av_zero(void) {
	return rvn_av_time(0.0);
}

@interface RvnAVStream : NSObject
@property(nonatomic, strong) AVPlayer* player;
@property(nonatomic, strong) AVPlayerItem* item;
@property(nonatomic, strong) AVPlayerItemVideoOutput* output;
@property(nonatomic, strong) id endObserver;
@end

@implementation RvnAVStream {
@public
	atomic_int ended;
	float rate;
	int playing;
	int tracks_known, has_video, has_audio;
}
@end

static RvnAVStream* rvn_av_stream(void* stream) {
	return (__bridge RvnAVStream*)stream;
}

/* Lets the main queue run where no one else does (see the top of the file). */
static void rvn_av_run_main_queue(void) {
	if (!pthread_main_np()) return;
	if (rvn_media_platform_pumps()) return;
	CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0, true);
}

static void* rvn_av_open(const char* location, int is_url, char* error) {
	@autoreleasepool {
		NSString* text = [NSString stringWithUTF8String:location];
		NSURL* url = nil;
		AVURLAsset* asset;
		AVPlayerItem* item;
		AVPlayerItemVideoOutput* output;
		RvnAVStream* stream;
		__weak RvnAVStream* weak;
		if (text) url = is_url ? [NSURL URLWithString:text] : [NSURL fileURLWithPath:text];
		if (!url) {
			snprintf(error, 256, "not an address AVFoundation can open: %s", location);
			return NULL;
		}
		asset = [[g_rvn_av.asset_class alloc] initWithURL:url options:nil];
		item = asset ? [[g_rvn_av.item_class alloc] initWithAsset:asset] : nil;
		/* (BGRA: what the texture takes after one swap of red and blue) */
		output = [[g_rvn_av.output_class alloc] initWithPixelBufferAttributes:@{ @"PixelFormatType": @(kCVPixelFormatType_32BGRA) }];
		if (!item || !output) {
			snprintf(error, 256, "AVFoundation cannot open %s", location);
			return NULL;
		}
		[item addOutput:output];
		stream = [[RvnAVStream alloc] init];
		stream.item = item;
		stream.output = output;
		stream.player = [[g_rvn_av.player_class alloc] initWithPlayerItem:item];
		stream.player.actionAtItemEnd = AVPlayerActionAtItemEndPause;
		stream->rate = 1.0f;
		weak = stream;
		/* (posted on the thread that ends the item: only a flag is set) */
		stream.endObserver = [[NSNotificationCenter defaultCenter] addObserverForName:@"AVPlayerItemDidPlayToEndTimeNotification"
			object:item queue:nil usingBlock:^(NSNotification* note) {
				RvnAVStream* strong = weak;
				(void)note;
				if (strong) atomic_store(&strong->ended, 1);
			}];
		return (__bridge_retained void*)stream;
	}
}

static void rvn_av_close(void* data) {
	@autoreleasepool {
		RvnAVStream* stream = (__bridge_transfer RvnAVStream*)data;
		if (stream.endObserver) [[NSNotificationCenter defaultCenter] removeObserver:stream.endObserver];
		[stream.player pause];
		[stream.item removeOutput:stream.output];
		[stream.player replaceCurrentItemWithPlayerItem:nil];
		stream.endObserver = nil;
		stream.output = nil;
		stream.item = nil;
		stream.player = nil;
	}
}

static void rvn_av_play(void* data) {
	RvnAVStream* stream = rvn_av_stream(data);
	stream->playing = 1;
	atomic_store(&stream->ended, 0);
	stream.player.rate = stream->rate;
}

static void rvn_av_pause(void* data) {
	RvnAVStream* stream = rvn_av_stream(data);
	stream->playing = 0;
	[stream.player pause];
}

static void rvn_av_seek(void* data, double seconds) {
	RvnAVStream* stream = rvn_av_stream(data);
	atomic_store(&stream->ended, 0);
	[stream.player seekToTime:rvn_av_time(seconds) toleranceBefore:rvn_av_zero() toleranceAfter:rvn_av_zero() completionHandler:^(BOOL finished) {
		(void)finished;
	}];
}

static void rvn_av_set_volume(void* data, float volume, int muted) {
	RvnAVStream* stream = rvn_av_stream(data);
	stream.player.volume = volume;
	stream.player.muted = muted ? YES : NO;
}

static void rvn_av_set_rate(void* data, float rate) {
	RvnAVStream* stream = rvn_av_stream(data);
	stream->rate = rate;
	if (stream->playing) stream.player.rate = rate;
}

static void rvn_av_poll(void* data, double dt, rvn_media_info_t* info) {
	@autoreleasepool {
		RvnAVStream* stream = rvn_av_stream(data);
		AVPlayerItem* item = stream.item;
		AVPlayerItemStatus status;
		double seconds;
		CGSize size;
		(void)dt;
		rvn_av_run_main_queue();
		status = item.status;
		if (status == AVPlayerItemStatusFailed || stream.player.status == AVPlayerStatusFailed) {
			NSError* failure = item.error ? item.error : stream.player.error;
			NSString* text = failure ? failure.localizedDescription : nil;
			NSString* reason = failure ? failure.localizedFailureReason : nil;
			info->status = RVN_MEDIA_FAILED;
			if (reason.length) text = [NSString stringWithFormat:@"%@ (%@)", text ? text : @"", reason];
			snprintf(info->error, sizeof(info->error), "%s", text.length ? text.UTF8String : "AVFoundation cannot play it");
			return;
		}
		if (status != AVPlayerItemStatusReadyToPlay) {
			info->status = RVN_MEDIA_OPENING;
			return;
		}
		info->status = RVN_MEDIA_OPENED;
		seconds = rvn_av_seconds(item.duration);
		info->duration = seconds > 0.0 ? seconds : 0.0;
		seconds = rvn_av_seconds(item.currentTime);
		info->position = seconds > 0.0 ? seconds : 0.0;
		if (!stream->tracks_known) {
			for (AVPlayerItemTrack* track in item.tracks) {
				NSString* type = track.assetTrack.mediaType;
				if ([type isEqualToString:@"vide"]) stream->has_video = 1;
				else if ([type isEqualToString:@"soun"]) stream->has_audio = 1;
			}
			stream->tracks_known = item.tracks.count > 0;
		}
		size = item.presentationSize;
		info->has_video = stream->has_video;
		info->has_audio = stream->has_audio;
		info->width = stream->has_video ? (int)lround(size.width) : 0;
		info->height = stream->has_video ? (int)lround(size.height) : 0;
		info->ended = atomic_exchange(&stream->ended, 0);
		/* (the player stops at the end without telling where: the end is the duration) */
		if (info->ended) {
			stream->playing = 0;
			if (info->duration > 0.0) info->position = info->duration;
		}
	}
}

static uint8_t* rvn_av_take_frame(void* data, int* width, int* height) {
	@autoreleasepool {
		RvnAVStream* stream = rvn_av_stream(data);
		CMTime now = stream.item.currentTime;
		CVPixelBufferRef buffer;
		uint8_t* pixels = NULL;
		if (![stream.output hasNewPixelBufferForItemTime:now]) return NULL;
		buffer = [stream.output copyPixelBufferForItemTime:now itemTimeForDisplay:NULL];
		if (!buffer) return NULL;
		if (g_rvn_av.format(buffer) == kCVPixelFormatType_32BGRA && g_rvn_av.lock(buffer, kCVPixelBufferLock_ReadOnly) == kCVReturnSuccess) {
			int w = (int)g_rvn_av.width(buffer), h = (int)g_rvn_av.height(buffer);
			const uint8_t* base = (const uint8_t*)g_rvn_av.base(buffer);
			if (base && w > 0 && h > 0) {
				pixels = (uint8_t*)rvn_alloc((size_t)w * (size_t)h * 4);
				rvn_media_bgra_to_rgba(pixels, base, w, h, g_rvn_av.row_bytes(buffer));
				*width = w;
				*height = h;
			}
			g_rvn_av.unlock(buffer, kCVPixelBufferLock_ReadOnly);
		}
		CFRelease(buffer);
		return pixels;
	}
}

static const rvn_media_backend_t g_rvn_av_backend = {
	"AVFoundation",
	rvn_av_available,
	rvn_av_open,
	rvn_av_close,
	rvn_av_play,
	rvn_av_pause,
	rvn_av_seek,
	rvn_av_set_volume,
	rvn_av_set_rate,
	rvn_av_poll,
	rvn_av_take_frame
};

const rvn_media_backend_t* rvn_media_system_backend(void) {
	return &g_rvn_av_backend;
}
