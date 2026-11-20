/**
 * Raven - render/anim_decode.c
 * Animated images: GIF (with Raven's own LZW decoder) and APNG (whose frames
 * are decoded by stb_image, each wrapped in a small PNG of its own).
 *
 * Opening a file only reads its structure: where each frame is, its
 * rectangle, its delay, how it is blended and disposed of. Nothing is
 * decoded ahead of time and no frame is kept: a cursor composes the frames
 * one after the other into a single canvas of the size of the image (plus,
 * for the frames that restore what was under them, a copy of their
 * rectangle). Playing costs the same memory whatever the number of frames;
 * going back to an earlier frame composes again from the first.
 *
 * Every offset read from the file is checked against its size: a damaged
 * or truncated file gives fewer frames, or frames with missing pixels,
 * never a read outside the buffer. Thread-safe: an animation is not changed
 * once it is open, and each cursor belongs to one user.
 */

#include "renderer.h"

/* The largest canvas of an animation (pixels): bigger files are shown as still images. */
#define RVN_ANIM_MAX_PIXELS (4096u * 4096u)
#define RVN_ANIM_MAX_SIDE 16384
/* Frames read at most; the ones after are ignored. */
#define RVN_ANIM_MAX_FRAMES 65536
/* What a frame of no or almost no delay lasts, as in browsers (ms). */
#define RVN_ANIM_DEFAULT_DELAY 100u

static rvn_atomic_t g_anim_serial;

static uint32_t rvn_be32(const uint8_t* p) {
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint16_t rvn_be16(const uint8_t* p) {
	return (uint16_t)(((uint32_t)p[0] << 8) | (uint32_t)p[1]);
}

static uint16_t rvn_le16(const uint8_t* p) {
	return (uint16_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8));
}

/* Browsers make the frames of 10 ms or less last 100 ms: animations made for old, slow decoders. */
static uint32_t rvn_anim_delay(uint32_t ms) {
	return ms <= 10u ? RVN_ANIM_DEFAULT_DELAY : ms;
}

static rvn_anim_frame_t* rvn_anim_add_frame(rvn_anim_t* anim, uint32_t* cap) {
	rvn_anim_frame_t* frame;
	if (anim->frame_count >= RVN_ANIM_MAX_FRAMES) return NULL;
	RVN_ARRAY_RESERVE(anim->frames, *cap, (uint32_t)anim->frame_count + 1);
	frame = &anim->frames[anim->frame_count++];
	memset(frame, 0, sizeof(*frame));
	frame->transparent = -1;
	return frame;
}

/* ---------------------------------------------------------------------------
 *  GIF
 * ------------------------------------------------------------------------- */

static int rvn_gif_detect(const uint8_t* data, size_t size) {
	return size >= 13 && memcmp(data, "GIF8", 4) == 0 && (data[4] == '7' || data[4] == '9') && data[5] == 'a';
}

/* Skips a chain of sub-blocks starting at `pos`: the position after its terminator, or 0 when it is cut. */
static size_t rvn_gif_skip_blocks(const uint8_t* data, size_t size, size_t pos) {
	while (pos < size) {
		size_t length = data[pos++];
		if (length == 0) return pos;
		pos += length;
	}
	return 0;
}

static int rvn_gif_parse(rvn_anim_t* anim) {
	const uint8_t* data = anim->data;
	size_t size = anim->size, pos = 13;
	uint32_t cap = 0, global_palette = 0, global_count = 0;
	int delay = 0, dispose = 0, transparent = -1;
	int max_x = 0, max_y = 0;

	anim->format = RVN_ANIM_GIF;
	anim->width = rvn_le16(data + 6);
	anim->height = rvn_le16(data + 8);
	anim->loop_count = 1;
	if (data[10] & 0x80) {
		global_count = 2u << (data[10] & 7);
		global_palette = (uint32_t)pos;
		pos += 3u * global_count;
		if (pos > size) return R_FALSE;
	}
	while (pos < size) {
		uint8_t block = data[pos++];
		if (block == 0x3B) break;
		if (block == 0x21) {
			uint8_t label;
			size_t next;
			if (pos >= size) break;
			label = data[pos++];
			if (label == 0xF9 && pos + 5 <= size && data[pos] >= 4) {
				/* graphic control: delay, disposal and transparent color of the next image */
				uint8_t packed = data[pos + 1];
				dispose = (packed >> 2) & 7;
				delay = rvn_le16(data + pos + 2) * 10;
				transparent = (packed & 1) ? data[pos + 4] : -1;
			} else if (label == 0xFF && pos + 12 <= size && data[pos] == 11 &&
				(memcmp(data + pos + 1, "NETSCAPE2.0", 11) == 0 || memcmp(data + pos + 1, "ANIMEXTS1.0", 11) == 0)) {
				/* loop count: repeated that many times after the first (0: forever) */
				size_t sub = pos + 12;
				if (sub + 4 <= size && data[sub] >= 3 && (data[sub + 1] & 7) == 1) {
					int repeats = rvn_le16(data + sub + 2);
					anim->loop_count = repeats == 0 ? 0 : repeats + 1;
				}
			}
			next = rvn_gif_skip_blocks(data, size, pos);
			if (!next) break;
			pos = next;
		} else if (block == 0x2C) {
			rvn_anim_frame_t* frame;
			uint8_t packed;
			uint32_t palette = global_palette, count = global_count;
			size_t next;
			if (pos + 9 > size) break;
			packed = data[pos + 8];
			if (packed & 0x80) {
				count = 2u << (packed & 7);
				palette = (uint32_t)(pos + 9);
				if (pos + 9 + 3u * count > size) break;
			}
			frame = rvn_anim_add_frame(anim, &cap);
			if (!frame) break;
			frame->x = rvn_le16(data + pos);
			frame->y = rvn_le16(data + pos + 2);
			frame->width = rvn_le16(data + pos + 4);
			frame->height = rvn_le16(data + pos + 6);
			frame->interlaced = (packed & 0x40) ? 1 : 0;
			frame->palette = palette;
			frame->palette_count = count;
			frame->transparent = transparent;
			frame->delay_ms = rvn_anim_delay((uint32_t)delay);
			frame->dispose = dispose == 2 ? RVN_ANIM_DISPOSE_BACKGROUND : dispose == 3 ? RVN_ANIM_DISPOSE_PREVIOUS : RVN_ANIM_DISPOSE_NONE;
			frame->blend = RVN_ANIM_BLEND_OVER;
			if (frame->x + frame->width > max_x) max_x = frame->x + frame->width;
			if (frame->y + frame->height > max_y) max_y = frame->y + frame->height;
			pos += 9 + ((packed & 0x80) ? 3u * count : 0u);
			/* (the pixels: the LZW code size, then sub-blocks) */
			frame->offset = (uint32_t)pos;
			delay = 0;
			dispose = 0;
			transparent = -1;
			if (pos >= size) break;
			next = rvn_gif_skip_blocks(data, size, pos + 1);
			/* a cut image keeps the pixels that are there; nothing can follow it */
			if (!next) break;
			pos = next;
		} else {
			/* not a GIF block: the rest of the file is not read */
			break;
		}
	}
	/* (a screen of no size: the one that holds the frames) */
	if (anim->width == 0 || anim->height == 0) {
		anim->width = max_x;
		anim->height = max_y;
	}
	return R_TRUE;
}

/* Row of the image that the n-th row stored in an interlaced GIF goes to. */
static int rvn_gif_interlaced_row(int n, int height) {
	int pass = (height + 7) / 8;
	if (n < pass) return n * 8;
	n -= pass;
	pass = (height + 3) / 8;
	if (n < pass) return 4 + n * 8;
	n -= pass;
	pass = (height + 1) / 4;
	if (n < pass) return 2 + n * 4;
	n -= pass;
	return 1 + n * 2;
}

typedef struct rvn_gif_lzw_t {
	uint16_t prefix[4096];
	uint8_t suffix[4096];
	uint8_t stack[4097];
} rvn_gif_lzw_t;

/* Decodes the pixels of a frame onto the canvas (its transparent color leaves what is there). */
static void rvn_gif_draw(const rvn_anim_t* anim, const rvn_anim_frame_t* frame, uint8_t* canvas) {
	const uint8_t* data = anim->data;
	const uint8_t* palette = data + frame->palette;
	size_t size = anim->size, pos = frame->offset, block_left = 0;
	rvn_gif_lzw_t* lzw;
	uint32_t bits = 0;
	int nbits = 0, min_size, code_size, clear, eoi, next, old = -1, first = 0;
	int column = 0, row = 0, y, done = R_FALSE;
	int width = frame->width, height = frame->height;
	uint64_t left = (uint64_t)width * (uint64_t)height;

	if (pos >= size || left == 0) return;
	min_size = data[pos++];
	/* (8 at most for a GIF; up to 11 is still a valid LZW stream) */
	if (min_size < 1 || min_size > 11) return;
	lzw = (rvn_gif_lzw_t*)rvn_alloc(sizeof(rvn_gif_lzw_t));
	clear = 1 << min_size;
	eoi = clear + 1;
	next = clear + 2;
	code_size = min_size + 1;
	y = frame->interlaced ? rvn_gif_interlaced_row(0, height) : 0;

	while (!done && left > 0) {
		int code, in_code, sp = 0;
		while (nbits < code_size) {
			if (block_left == 0) {
				if (pos >= size) { done = R_TRUE; break; }
				block_left = data[pos++];
				if (block_left == 0) { done = R_TRUE; break; }
			}
			if (pos >= size) { done = R_TRUE; break; }
			bits |= (uint32_t)data[pos++] << nbits;
			nbits += 8;
			block_left--;
		}
		if (done) break;
		code = (int)(bits & ((1u << code_size) - 1u));
		bits >>= code_size;
		nbits -= code_size;
		if (code == clear) {
			code_size = min_size + 1;
			next = clear + 2;
			old = -1;
			continue;
		}
		if (code == eoi) break;
		if (old < 0) {
			/* the first code after a clear is a color */
			if (code >= clear) break;
			lzw->stack[sp++] = (uint8_t)code;
			first = code;
			old = code;
		} else {
			if (code > next) break;
			in_code = code;
			if (code == next) {
				/* a string that starts with the end of the previous one */
				lzw->stack[sp++] = (uint8_t)first;
				code = old;
			}
			while (code >= clear && sp < 4096) {
				lzw->stack[sp++] = lzw->suffix[code];
				code = lzw->prefix[code];
			}
			if (code >= clear) break;
			first = code;
			lzw->stack[sp++] = (uint8_t)code;
			if (next < 4096) {
				lzw->prefix[next] = (uint16_t)old;
				lzw->suffix[next] = (uint8_t)first;
				next++;
				if (next == (1 << code_size) && code_size < 12) code_size++;
			}
			old = in_code;
		}
		/* the string comes out in reverse */
		while (sp > 0 && left > 0) {
			int index = lzw->stack[--sp];
			int cx = frame->x + column, cy = frame->y + y;
			if (index != frame->transparent && (uint32_t)index < frame->palette_count && cx < anim->width && cy < anim->height) {
				uint8_t* pixel = canvas + ((size_t)cy * (size_t)anim->width + (size_t)cx) * 4;
				pixel[0] = palette[index * 3];
				pixel[1] = palette[index * 3 + 1];
				pixel[2] = palette[index * 3 + 2];
				pixel[3] = 255;
			}
			left--;
			if (++column == width) {
				column = 0;
				row++;
				y = frame->interlaced ? rvn_gif_interlaced_row(row, height) : row;
			}
		}
	}
	rvn_free(lzw);
}

/* ---------------------------------------------------------------------------
 *  APNG
 * ------------------------------------------------------------------------- */

static const uint8_t rvn_png_signature[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };

static int rvn_png_detect(const uint8_t* data, size_t size) {
	return size >= 8 && memcmp(data, rvn_png_signature, 8) == 0;
}

static int rvn_apng_add_segment(rvn_anim_t* anim, uint32_t* cap, size_t offset, size_t length) {
	if (length == 0) return R_TRUE;
	RVN_ARRAY_RESERVE(anim->segments, *cap, anim->segment_count + 1);
	anim->segments[anim->segment_count].offset = (uint32_t)offset;
	anim->segments[anim->segment_count].length = (uint32_t)length;
	anim->segment_count++;
	return R_TRUE;
}

static int rvn_apng_parse(rvn_anim_t* anim) {
	const uint8_t* data = anim->data;
	size_t size = anim->size, pos = 8;
	uint32_t frame_cap = 0, segment_cap = 0, declared = 0;
	int has_actl = R_FALSE, seen_idat = R_FALSE, after_idat = R_FALSE;
	/* the frame being read, and whether its pixels are the IDAT chunks (the image of the file) */
	rvn_anim_frame_t* frame = NULL;
	int frame_is_image = R_FALSE;

	anim->format = RVN_ANIM_APNG;
	while (pos + 12 <= size) {
		uint32_t length = rvn_be32(data + pos);
		const uint8_t* type = data + pos + 4;
		const uint8_t* body = data + pos + 8;
		if (length > size - pos - 12) break;
		if (pos == 8) {
			/* the header comes first */
			if (memcmp(type, "IHDR", 4) != 0 || length != 13) return R_FALSE;
			anim->width = (int)rvn_be32(body) > 0 ? (int)rvn_be32(body) : 0;
			anim->height = (int)rvn_be32(body + 4) > 0 ? (int)rvn_be32(body + 4) : 0;
			anim->header = (uint32_t)(pos + 8);
		} else if (memcmp(type, "acTL", 4) == 0 && length == 8 && !seen_idat) {
			has_actl = R_TRUE;
			declared = rvn_be32(body);
			anim->loop_count = rvn_be32(body + 4) > 0x7FFFFFFFu ? 0 : (int)rvn_be32(body + 4);
		} else if (memcmp(type, "PLTE", 4) == 0 && !seen_idat) {
			anim->palette = (uint32_t)pos;
			anim->palette_length = length + 12;
		} else if (memcmp(type, "tRNS", 4) == 0 && !seen_idat) {
			anim->transparency = (uint32_t)pos;
			anim->transparency_length = length + 12;
		} else if (memcmp(type, "fcTL", 4) == 0) {
			uint32_t w, h, x, y, num, den;
			if (length != 26) break;
			w = rvn_be32(body + 4);
			h = rvn_be32(body + 8);
			x = rvn_be32(body + 12);
			y = rvn_be32(body + 16);
			num = rvn_be16(body + 20);
			den = rvn_be16(body + 22);
			/* a frame outside the image, or with unknown operations: the animation stops before it */
			if (w == 0 || h == 0 || x > (uint32_t)anim->width || y > (uint32_t)anim->height ||
				w > (uint32_t)anim->width - x || h > (uint32_t)anim->height - y || body[24] > 2 || body[25] > 1) break;
			/* the image of the file is the first frame only when the control comes before it, at its size */
			if (!seen_idat && (x != 0 || y != 0 || w != (uint32_t)anim->width || h != (uint32_t)anim->height)) break;
			frame = rvn_anim_add_frame(anim, &frame_cap);
			if (!frame) break;
			frame->x = (int)x;
			frame->y = (int)y;
			frame->width = (int)w;
			frame->height = (int)h;
			frame->delay_ms = rvn_anim_delay(den == 0 ? num * 10u : (uint32_t)((uint64_t)num * 1000u / den));
			frame->dispose = body[24] == 1 ? RVN_ANIM_DISPOSE_BACKGROUND : body[24] == 2 ? RVN_ANIM_DISPOSE_PREVIOUS : RVN_ANIM_DISPOSE_NONE;
			frame->blend = body[25] == 1 ? RVN_ANIM_BLEND_OVER : RVN_ANIM_BLEND_SOURCE;
			frame->offset = anim->segment_count;
			frame_is_image = !seen_idat;
			if (frame_is_image) anim->image_is_frame = R_TRUE;
		} else if (memcmp(type, "IDAT", 4) == 0) {
			if (after_idat) break;
			seen_idat = R_TRUE;
			if (frame && frame_is_image) {
				rvn_apng_add_segment(anim, &segment_cap, pos + 8, length);
				frame->segments++;
			}
		} else if (memcmp(type, "fdAT", 4) == 0) {
			if (frame && !frame_is_image && length > 4) {
				rvn_apng_add_segment(anim, &segment_cap, pos + 12, length - 4);
				frame->segments++;
			}
		} else if (memcmp(type, "IEND", 4) == 0) {
			break;
		}
		if (seen_idat && memcmp(type, "IDAT", 4) != 0) after_idat = R_TRUE;
		pos += 12 + (size_t)length;
	}
	if (!has_actl || anim->width <= 0 || anim->height <= 0) return R_FALSE;
	/* (frames that never got their pixels, at the end of a cut file, are not shown) */
	while (anim->frame_count > 0 && anim->frames[anim->frame_count - 1].segments == 0) anim->frame_count--;
	if (declared > 0 && (uint32_t)anim->frame_count > declared) anim->frame_count = (int)declared;
	/* the first frame cannot restore what was before it: there was nothing */
	if (anim->frame_count > 0 && anim->frames[0].dispose == RVN_ANIM_DISPOSE_PREVIOUS) anim->frames[0].dispose = RVN_ANIM_DISPOSE_BACKGROUND;
	return R_TRUE;
}

static uint32_t rvn_crc32(const uint8_t* data, size_t size, uint32_t crc) {
	size_t i;
	int k;
	crc = ~crc;
	for (i = 0; i < size; i++) {
		crc ^= data[i];
		for (k = 0; k < 8; k++) crc = (crc & 1u) ? 0xEDB88320u ^ (crc >> 1) : crc >> 1;
	}
	return ~crc;
}

static void rvn_put_be32(uint8_t* p, uint32_t value) {
	p[0] = (uint8_t)(value >> 24);
	p[1] = (uint8_t)(value >> 16);
	p[2] = (uint8_t)(value >> 8);
	p[3] = (uint8_t)value;
}

static void rvn_png_chunk(uint8_t* out, size_t* at, const char* type, const uint8_t* body, size_t length) {
	uint32_t crc;
	rvn_put_be32(out + *at, (uint32_t)length);
	memcpy(out + *at + 4, type, 4);
	if (length) memcpy(out + *at + 8, body, length);
	crc = rvn_crc32(out + *at + 4, length + 4, 0);
	rvn_put_be32(out + *at + 8 + length, crc);
	*at += 12 + length;
}

/*
 * The pixels of an APNG frame (RGBA, its own size): the frame becomes a PNG
 * of its own (the header at its size, the palette, its data as one IDAT)
 * that stb_image decodes. NULL when it cannot be decoded.
 */
static uint8_t* rvn_apng_decode_frame(const rvn_anim_t* anim, const rvn_anim_frame_t* frame) {
	const uint8_t* data = anim->data;
	size_t total = 0, at = 0;
	uint32_t i;
	uint8_t header[13];
	uint8_t* png;
	uint8_t* pixels;
	int width = 0, height = 0;

	for (i = 0; i < frame->segments; i++) total += anim->segments[frame->offset + i].length;
	png = (uint8_t*)rvn_alloc(8 + 25 + anim->palette_length + anim->transparency_length + 12 + total + 12);
	memcpy(png, rvn_png_signature, 8);
	at = 8;
	memcpy(header, data + anim->header, 13);
	rvn_put_be32(header, (uint32_t)frame->width);
	rvn_put_be32(header + 4, (uint32_t)frame->height);
	rvn_png_chunk(png, &at, "IHDR", header, 13);
	if (anim->palette_length) {
		memcpy(png + at, data + anim->palette, anim->palette_length);
		at += anim->palette_length;
	}
	if (anim->transparency_length) {
		memcpy(png + at, data + anim->transparency, anim->transparency_length);
		at += anim->transparency_length;
	}
	/* the IDAT: its CRC is computed over what is copied */
	{
		size_t start = at;
		rvn_put_be32(png + at, (uint32_t)total);
		memcpy(png + at + 4, "IDAT", 4);
		at += 8;
		for (i = 0; i < frame->segments; i++) {
			const rvn_anim_segment_t* segment = &anim->segments[frame->offset + i];
			memcpy(png + at, data + segment->offset, segment->length);
			at += segment->length;
		}
		rvn_put_be32(png + at, rvn_crc32(png + start + 4, total + 4, 0));
		at += 4;
	}
	rvn_png_chunk(png, &at, "IEND", NULL, 0);
	pixels = rvn_image_decode(png, at, &width, &height);
	rvn_free(png);
	if (pixels && (width != frame->width || height != frame->height)) {
		rvn_free(pixels);
		pixels = NULL;
	}
	return pixels;
}

/* Puts the pixels of an APNG frame on the canvas, replacing or over what is there. */
static void rvn_apng_draw(const rvn_anim_t* anim, const rvn_anim_frame_t* frame, uint8_t* canvas) {
	uint8_t* pixels = rvn_apng_decode_frame(anim, frame);
	int x, y;
	if (!pixels) return;
	for (y = 0; y < frame->height; y++) {
		const uint8_t* src = pixels + (size_t)y * (size_t)frame->width * 4;
		uint8_t* dst = canvas + ((size_t)(frame->y + y) * (size_t)anim->width + (size_t)frame->x) * 4;
		if (frame->blend == RVN_ANIM_BLEND_SOURCE) {
			memcpy(dst, src, (size_t)frame->width * 4);
			continue;
		}
		for (x = 0; x < frame->width; x++, src += 4, dst += 4) {
			uint32_t sa = src[3], da, out;
			int c;
			if (sa == 255) {
				memcpy(dst, src, 4);
				continue;
			}
			if (sa == 0) continue;
			/* straight alpha: the colors are weighted by what each side covers */
			da = (uint32_t)dst[3] * (255u - sa);
			out = sa * 255u + da;
			for (c = 0; c < 3; c++) dst[c] = (uint8_t)((src[c] * sa * 255u + dst[c] * da + out / 2u) / out);
			dst[3] = (uint8_t)((out + 127u) / 255u);
		}
	}
	rvn_free(pixels);
}

/* ---------------------------------------------------------------------------
 *  Opening and composing
 * ------------------------------------------------------------------------- */

int rvn_anim_detect(const uint8_t* data, size_t size) {
	if (!data) return R_FALSE;
	if (rvn_gif_detect(data, size)) return R_TRUE;
	if (rvn_png_detect(data, size)) {
#if defined(RAVEN_WITH_STB_IMAGE)
		/* an animated PNG says so before its first IDAT */
		size_t pos = 8;
		while (pos + 12 <= size) {
			uint32_t length = rvn_be32(data + pos);
			if (memcmp(data + pos + 4, "acTL", 4) == 0) return R_TRUE;
			if (memcmp(data + pos + 4, "IDAT", 4) == 0 || length > size - pos - 12) return R_FALSE;
			pos += 12 + (size_t)length;
		}
#endif
		/* (without stb_image, nothing decodes the frames of an APNG) */
	}
	return R_FALSE;
}

void rvn_anim_free(rvn_anim_t* anim) {
	if (!anim) return;
	rvn_free(anim->owned);
	rvn_free(anim->frames);
	rvn_free(anim->segments);
	rvn_free(anim);
}

rvn_anim_t* rvn_anim_open(const uint8_t* data, size_t size, int copy) {
	rvn_anim_t* anim;
	int ok;
	if (!rvn_anim_detect(data, size) || size > 0x7FFFFFFFu) return NULL;
	anim = (rvn_anim_t*)rvn_calloc(sizeof(rvn_anim_t));
	anim->data = data;
	anim->size = size;
	ok = rvn_gif_detect(data, size) ? rvn_gif_parse(anim) : rvn_apng_parse(anim);
	/* one frame is a still image, left to the decoder of still images (unless it is not the image of the file) */
	if (!ok || anim->frame_count < 1 || (anim->frame_count < 2 && (anim->format != RVN_ANIM_APNG || anim->image_is_frame)) || anim->width <= 0 || anim->height <= 0 || anim->width > RVN_ANIM_MAX_SIDE ||
		anim->height > RVN_ANIM_MAX_SIDE || (uint64_t)anim->width * (uint64_t)anim->height > RVN_ANIM_MAX_PIXELS) {
		rvn_anim_free(anim);
		return NULL;
	}
	{
		uint64_t total = 0;
		int i;
		for (i = 0; i < anim->frame_count; i++) total += anim->frames[i].delay_ms;
		anim->duration_ms = total > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)total;
	}
	if (copy) {
		anim->owned = (uint8_t*)rvn_alloc(size);
		memcpy(anim->owned, data, size);
		anim->data = anim->owned;
	}
	anim->serial = (uint32_t)rvn_atomic_add(&g_anim_serial, 1);
	return anim;
}

void rvn_anim_cursor_free(rvn_anim_cursor_t* cursor) {
	rvn_free(cursor->canvas);
	rvn_free(cursor->saved);
	memset(cursor, 0, sizeof(*cursor));
	cursor->index = -1;
}

/* The part of a frame inside the canvas. */
static int rvn_anim_frame_rect(const rvn_anim_t* anim, const rvn_anim_frame_t* frame, int* x0, int* y0, int* x1, int* y1) {
	*x0 = frame->x;
	*y0 = frame->y;
	*x1 = frame->x + frame->width < anim->width ? frame->x + frame->width : anim->width;
	*y1 = frame->y + frame->height < anim->height ? frame->y + frame->height : anim->height;
	return *x1 > *x0 && *y1 > *y0;
}

/* Copies the rectangle of a frame between the canvas and the saved copy. */
static void rvn_anim_copy_rect(const rvn_anim_t* anim, const rvn_anim_frame_t* frame, uint8_t* canvas, uint8_t* saved, int save) {
	int x0, y0, x1, y1, y;
	size_t row;
	if (!rvn_anim_frame_rect(anim, frame, &x0, &y0, &x1, &y1)) return;
	row = (size_t)(x1 - x0) * 4;
	for (y = y0; y < y1; y++) {
		uint8_t* at = canvas + ((size_t)y * (size_t)anim->width + (size_t)x0) * 4;
		uint8_t* copy = saved + (size_t)(y - y0) * row;
		if (save) memcpy(copy, at, row);
		else memcpy(at, copy, row);
	}
}

static void rvn_anim_dispose(const rvn_anim_t* anim, const rvn_anim_frame_t* frame, rvn_anim_cursor_t* cursor) {
	int x0, y0, x1, y1, y;
	if (frame->dispose == RVN_ANIM_DISPOSE_PREVIOUS && cursor->saved) {
		rvn_anim_copy_rect(anim, frame, cursor->canvas, cursor->saved, R_FALSE);
	} else if (frame->dispose == RVN_ANIM_DISPOSE_BACKGROUND && rvn_anim_frame_rect(anim, frame, &x0, &y0, &x1, &y1)) {
		/* (the background of the file is ignored, as browsers do: the frame leaves transparency) */
		for (y = y0; y < y1; y++) memset(cursor->canvas + ((size_t)y * (size_t)anim->width + (size_t)x0) * 4, 0, (size_t)(x1 - x0) * 4);
	}
}

const uint8_t* rvn_anim_compose(const rvn_anim_t* anim, rvn_anim_cursor_t* cursor, int index) {
	size_t canvas_size = (size_t)anim->width * (size_t)anim->height * 4;
	if (index < 0) index = 0;
	if (index >= anim->frame_count) index = anim->frame_count - 1;
	if (cursor->serial != anim->serial) {
		rvn_anim_cursor_free(cursor);
		cursor->serial = anim->serial;
	}
	if (!cursor->canvas) {
		cursor->canvas = (uint8_t*)rvn_calloc(canvas_size);
		cursor->index = -1;
	}
	if (index < cursor->index) {
		/* back: from the first frame again */
		memset(cursor->canvas, 0, canvas_size);
		cursor->index = -1;
	}
	while (cursor->index < index) {
		const rvn_anim_frame_t* frame;
		if (cursor->index >= 0) rvn_anim_dispose(anim, &anim->frames[cursor->index], cursor);
		frame = &anim->frames[++cursor->index];
		if (frame->dispose == RVN_ANIM_DISPOSE_PREVIOUS) {
			if (!cursor->saved) cursor->saved = (uint8_t*)rvn_alloc(canvas_size);
			rvn_anim_copy_rect(anim, frame, cursor->canvas, cursor->saved, R_TRUE);
		}
		if (anim->format == RVN_ANIM_GIF) rvn_gif_draw(anim, frame, cursor->canvas);
		else rvn_apng_draw(anim, frame, cursor->canvas);
	}
	return cursor->canvas;
}

uint8_t* rvn_anim_decode_frame(const rvn_anim_t* anim, int index) {
	rvn_anim_cursor_t cursor;
	uint8_t* pixels;
	memset(&cursor, 0, sizeof(cursor));
	cursor.index = -1;
	rvn_anim_compose(anim, &cursor, index);
	pixels = cursor.canvas;
	cursor.canvas = NULL;
	rvn_anim_cursor_free(&cursor);
	return pixels;
}
