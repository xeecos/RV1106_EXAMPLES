/*
 *             (C) 2012 Mauro Carvalho Chehab
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335  USA
 */

#include <config.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

#if defined(__OpenBSD__)
#include <sys/videoio.h>
#include <sys/ioctl.h>
#else
#include <linux/videodev2.h>
#endif

#include "libv4l-plugin.h"

#define SYS_IOCTL(fd, cmd, arg) \
	syscall(SYS_ioctl, (int)(fd), (unsigned long)(cmd), (void *)(arg))
#define SYS_READ(fd, buf, len) \
	syscall(SYS_read, (int)(fd), (void *)(buf), (size_t)(len));
#define SYS_WRITE(fd, buf, len) \
	syscall(SYS_write, (int)(fd), (const void *)(buf), (size_t)(len));


#if HAVE_VISIBILITY
#define PLUGIN_PUBLIC __attribute__ ((visibility("default")))
#else
#define PLUGIN_PUBLIC
#endif

#define MPLANE_MAX_FORMATS 32

struct mplane_formats {
	struct v4l2_format formats[MPLANE_MAX_FORMATS];
	int index_map[MPLANE_MAX_FORMATS];
	unsigned int num_formats;
	int def_format;
};

struct mplane_plugin {
	union {
		struct {
			unsigned int		mplane_capture : 1;
			unsigned int		mplane_output  : 1;
		};
		unsigned int mplane;
	};

	struct mplane_formats capture_formats;
	struct mplane_formats output_formats;
};

#define SIMPLE_CONVERT_IOCTL(fd, cmd, arg, __struc) ({		\
	int __ret;						\
	struct __struc *req = arg;				\
	uint32_t type = req->type;				\
	if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||	\
	    type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {	\
		errno = EINVAL;					\
		__ret = -1;					\
	} else {						\
		req->type = convert_type(type);			\
		__ret = SYS_IOCTL(fd, cmd, arg);		\
		req->type = type;				\
	}							\
	__ret;							\
	})

/* Setup supported(single plane) formats */
static void mplane_setup_formats(int fd, struct mplane_formats *formats,
				 enum v4l2_buf_type type)
{
	int ret, n;

	formats->num_formats = 0;
	formats->def_format = -1;

	for (n = 0; formats->num_formats < MPLANE_MAX_FORMATS; n++) {
		struct v4l2_fmtdesc fmtdesc = { 0 };
		struct v4l2_format format = { 0 };

		fmtdesc.type = type;
		fmtdesc.index = n;

		ret = SYS_IOCTL(fd, VIDIOC_ENUM_FMT, &fmtdesc);
		if (ret < 0)
			break;

		//TODO: Is there any better way to detect it?

		format.type = type;
		format.fmt.pix.pixelformat = fmtdesc.pixelformat;

		/* Allow error since not all the drivers support try_fmt */
		SYS_IOCTL(fd, VIDIOC_TRY_FMT, &format);

		switch (format.fmt.pix_mp.num_planes) {
		case 1:
			if (formats->def_format < 0)
				formats->def_format = formats->num_formats;

			/* fall-through */
		case 0:
			/**
			 * Allow 0 planes since not all the drivers would set
			 * num_planes in try_fmt.
			 */
			formats->formats[formats->num_formats] = format;
			formats->index_map[formats->num_formats] = n;
			formats->num_formats++;
			break;
		default:
			break;
		}
	}
}

static void *plugin_init(int fd)
{
	struct v4l2_capability cap;
	int ret;
	struct mplane_plugin plugin, *ret_plugin;

	memset(&plugin, 0, sizeof(plugin));

	/* Check if device needs mplane plugin */
	memset(&cap, 0, sizeof(cap));
	ret = SYS_IOCTL(fd, VIDIOC_QUERYCAP, &cap);
	if (ret) {
		perror("Failed to query video capabilities");
		return NULL;
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) &&
	    (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
		plugin.mplane_capture = 1;

		mplane_setup_formats(fd, &plugin.capture_formats,
				     V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT) &&
	    (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT_MPLANE)) {
		plugin.mplane_output = 1;

		mplane_setup_formats(fd, &plugin.output_formats,
				     V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
	}

	/* Device doesn't need it. return NULL to disable the plugin */
	if (!plugin.mplane)
		return NULL;

	/* Allocate and initialize private data */
	ret_plugin = calloc(1, sizeof(*ret_plugin));
	if (!ret_plugin) {
		perror("Couldn't allocate memory for plugin");
		return NULL;
	}
	*ret_plugin = plugin;

	printf("Using mplane plugin for %s%s\n",
	       plugin.mplane_capture ? "capture " : "",
	       plugin.mplane_output ? "output " : "");

	return ret_plugin;
}

static void plugin_close(void *dev_ops_priv) {
	if (dev_ops_priv == NULL)
		return;

	free(dev_ops_priv);
}

static int querycap_ioctl(int fd, unsigned long int cmd,
			  struct v4l2_capability *arg)
{
	struct v4l2_capability *cap = arg;
	int ret;
	
	ret = SYS_IOCTL(fd, cmd, cap);
	if (ret)
		return ret;

	/* Report mplane as normal caps */
	if (cap->capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)
		cap->capabilities |= V4L2_CAP_VIDEO_CAPTURE;

	if (cap->capabilities & V4L2_CAP_VIDEO_OUTPUT_MPLANE)
		cap->capabilities |= V4L2_CAP_VIDEO_OUTPUT;

	if (cap->device_caps & V4L2_CAP_VIDEO_CAPTURE_MPLANE)
		cap->device_caps |= V4L2_CAP_VIDEO_CAPTURE;

	if (cap->device_caps & V4L2_CAP_VIDEO_OUTPUT_MPLANE)
		cap->device_caps |= V4L2_CAP_VIDEO_OUTPUT;

	cap->capabilities |= V4L2_CAP_EXT_PIX_FORMAT;
	cap->device_caps |= V4L2_CAP_EXT_PIX_FORMAT;

	/*
	 * Don't report mplane caps, as this will be handled via
	 * this plugin
	 */
	cap->capabilities &= ~(V4L2_CAP_VIDEO_OUTPUT_MPLANE |
			       V4L2_CAP_VIDEO_CAPTURE_MPLANE);
	cap->device_caps &= ~(V4L2_CAP_VIDEO_OUTPUT_MPLANE |
			      V4L2_CAP_VIDEO_CAPTURE_MPLANE);

	return 0;
}

static int convert_type(int type)
{
	switch (type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		return V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		return V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	default:
		return type;
	}
}

static int enum_fmt_ioctl(struct mplane_plugin *plugin, int fd,
			  unsigned long int cmd,
			  struct v4l2_fmtdesc *arg)
{
	struct mplane_formats *formats;
	int ret, index;

	switch (arg->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		formats = &plugin->capture_formats;
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		formats = &plugin->output_formats;
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		errno = EINVAL;
		return -1;
	default:
		return SYS_IOCTL(fd, cmd, arg);
	}

	if (arg->index >= formats->num_formats)
		return -EINVAL;

	index = arg->index;
	arg->index = formats->index_map[index];
	ret = SIMPLE_CONVERT_IOCTL(fd, cmd, arg, v4l2_fmtdesc);
	arg->index = index;

	return ret;
}

static void sanitize_format(struct v4l2_format *fmt)
{
	unsigned int offset;

	/*
	 * The v4l2_pix_format structure has been extended with fields that were
	 * not previously required to be set to zero by applications. The priv
	 * field, when set to a magic value, indicates the the extended fields
	 * are valid. We support these extended fields since struct
	 * v4l2_pix_format_mplane supports those fields as well.
	 *
	 * So this function will sanitize v4l2_pix_format if priv != PRIV_MAGIC
	 * by setting priv to that value and zeroing the remaining fields.
	 */

	if (fmt->fmt.pix.priv == V4L2_PIX_FMT_PRIV_MAGIC)
		return;

	fmt->fmt.pix.priv = V4L2_PIX_FMT_PRIV_MAGIC;

	offset = offsetof(struct v4l2_pix_format, priv)
	       + sizeof(fmt->fmt.pix.priv);
	memset(((char *)&fmt->fmt.pix) + offset, 0,
	       sizeof(fmt->fmt.pix) - offset);
}

static int try_set_fmt_ioctl(struct mplane_plugin *plugin, int fd,
			     unsigned long int cmd,
			     struct v4l2_format *arg)
{
	struct mplane_formats *formats;
	struct v4l2_format fmt = { 0 };
	struct v4l2_format *org = arg;
	int ret, i;

	switch (arg->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		formats = &plugin->capture_formats;
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		formats = &plugin->output_formats;
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		errno = EINVAL;
		return -1;
	default:
		return SYS_IOCTL(fd, cmd, arg);
	}

	/* Filter out unsupported formats */
	for (i = 0; i < formats->num_formats; i++) {
		if (formats->formats[i].fmt.pix_mp.pixelformat ==
		    arg->fmt.pix.pixelformat)
			break;
	}
	if (i == formats->num_formats)
		return -EINVAL;

	sanitize_format(org);

	fmt.fmt.pix_mp.width = org->fmt.pix.width;
	fmt.fmt.pix_mp.height = org->fmt.pix.height;
	fmt.fmt.pix_mp.pixelformat = org->fmt.pix.pixelformat;
	fmt.fmt.pix_mp.field = org->fmt.pix.field;
	fmt.fmt.pix_mp.colorspace = org->fmt.pix.colorspace;
	fmt.fmt.pix_mp.xfer_func = org->fmt.pix.xfer_func;
	fmt.fmt.pix_mp.ycbcr_enc = org->fmt.pix.ycbcr_enc;
	fmt.fmt.pix_mp.quantization = org->fmt.pix.quantization;
	fmt.fmt.pix_mp.num_planes = 1;
	fmt.fmt.pix_mp.flags = org->fmt.pix.flags;
	fmt.fmt.pix_mp.plane_fmt[0].bytesperline = org->fmt.pix.bytesperline;
	fmt.fmt.pix_mp.plane_fmt[0].sizeimage = org->fmt.pix.sizeimage;

	ret = SYS_IOCTL(fd, cmd, &fmt);
	if (ret)
		return ret;

	org->fmt.pix.width = fmt.fmt.pix_mp.width;
	org->fmt.pix.height = fmt.fmt.pix_mp.height;
	org->fmt.pix.pixelformat = fmt.fmt.pix_mp.pixelformat;
	org->fmt.pix.field = fmt.fmt.pix_mp.field;
	org->fmt.pix.colorspace = fmt.fmt.pix_mp.colorspace;
	org->fmt.pix.xfer_func = fmt.fmt.pix_mp.xfer_func;
	org->fmt.pix.ycbcr_enc = fmt.fmt.pix_mp.ycbcr_enc;
	org->fmt.pix.quantization = fmt.fmt.pix_mp.quantization;
	org->fmt.pix.bytesperline = fmt.fmt.pix_mp.plane_fmt[0].bytesperline;
	org->fmt.pix.sizeimage = fmt.fmt.pix_mp.plane_fmt[0].sizeimage;
	org->fmt.pix.flags = fmt.fmt.pix_mp.flags;

	/* Now we can use the driver's default format */
	if (cmd == VIDIOC_S_FMT)
		formats->def_format = -1;

	return 0;
}

static int create_bufs_ioctl(int fd, unsigned long int cmd,
			     struct v4l2_create_buffers *arg)
{
	struct v4l2_create_buffers cbufs = { 0 };
	struct v4l2_format *fmt = &cbufs.format;
	struct v4l2_format *org = &arg->format;
	int ret;

	switch (arg->format.type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		fmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		fmt->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		errno = EINVAL;
		return -1;
	default:
		return SYS_IOCTL(fd, cmd, arg);
	}

	cbufs.index = arg->index;
	cbufs.count = arg->count;
	cbufs.memory = arg->memory;
	sanitize_format(org);
	fmt->fmt.pix_mp.width = org->fmt.pix.width;
	fmt->fmt.pix_mp.height = org->fmt.pix.height;
	fmt->fmt.pix_mp.pixelformat = org->fmt.pix.pixelformat;
	fmt->fmt.pix_mp.field = org->fmt.pix.field;
	fmt->fmt.pix_mp.colorspace = org->fmt.pix.colorspace;
	fmt->fmt.pix_mp.xfer_func = org->fmt.pix.xfer_func;
	fmt->fmt.pix_mp.ycbcr_enc = org->fmt.pix.ycbcr_enc;
	fmt->fmt.pix_mp.quantization = org->fmt.pix.quantization;
	fmt->fmt.pix_mp.num_planes = 1;
	fmt->fmt.pix_mp.flags = org->fmt.pix.flags;
	fmt->fmt.pix_mp.plane_fmt[0].bytesperline = org->fmt.pix.bytesperline;
	fmt->fmt.pix_mp.plane_fmt[0].sizeimage = org->fmt.pix.sizeimage;

	ret = SYS_IOCTL(fd, cmd, &cbufs);

	arg->index = cbufs.index;
	arg->count = cbufs.count;
	org->fmt.pix.width = fmt->fmt.pix_mp.width;
	org->fmt.pix.height = fmt->fmt.pix_mp.height;
	org->fmt.pix.pixelformat = fmt->fmt.pix_mp.pixelformat;
	org->fmt.pix.field = fmt->fmt.pix_mp.field;
	org->fmt.pix.colorspace = fmt->fmt.pix_mp.colorspace;
	org->fmt.pix.xfer_func = fmt->fmt.pix_mp.xfer_func;
	org->fmt.pix.ycbcr_enc = fmt->fmt.pix_mp.ycbcr_enc;
	org->fmt.pix.quantization = fmt->fmt.pix_mp.quantization;
	org->fmt.pix.bytesperline = fmt->fmt.pix_mp.plane_fmt[0].bytesperline;
	org->fmt.pix.sizeimage = fmt->fmt.pix_mp.plane_fmt[0].sizeimage;
	org->fmt.pix.flags = fmt->fmt.pix_mp.flags;

	return ret;
}

static int get_fmt_ioctl(struct mplane_plugin *plugin, int fd,
			 unsigned long int cmd, struct v4l2_format *arg)
{
	struct mplane_formats *formats;
	struct v4l2_format fmt = { 0 };
	struct v4l2_format *org = arg;
	int ret;

	switch (arg->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		formats = &plugin->capture_formats;
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		formats = &plugin->output_formats;
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		errno = EINVAL;
		return -1;
	default:
		return SYS_IOCTL(fd, cmd, arg);
	}

	/* Use default format */
	if (formats->def_format >= 0 &&
	    formats->def_format < formats->num_formats) {
		fmt = formats->formats[formats->def_format];
		goto out;
	}

	ret = SYS_IOCTL(fd, cmd, &fmt);
	if (ret)
		return ret;

out:
	memset(&org->fmt.pix, 0, sizeof(org->fmt.pix));
	org->fmt.pix.width = fmt.fmt.pix_mp.width;
	org->fmt.pix.height = fmt.fmt.pix_mp.height;
	org->fmt.pix.pixelformat = fmt.fmt.pix_mp.pixelformat;
	org->fmt.pix.field = fmt.fmt.pix_mp.field;
	org->fmt.pix.colorspace = fmt.fmt.pix_mp.colorspace;
	org->fmt.pix.xfer_func = fmt.fmt.pix_mp.xfer_func;
	org->fmt.pix.ycbcr_enc = fmt.fmt.pix_mp.ycbcr_enc;
	org->fmt.pix.quantization = fmt.fmt.pix_mp.quantization;
	org->fmt.pix.bytesperline = fmt.fmt.pix_mp.plane_fmt[0].bytesperline;
	org->fmt.pix.sizeimage = fmt.fmt.pix_mp.plane_fmt[0].sizeimage;
	org->fmt.pix.priv = V4L2_PIX_FMT_PRIV_MAGIC;
	org->fmt.pix.flags = fmt.fmt.pix_mp.flags;

	/*
	 * If the device doesn't support just one plane, there's
	 * nothing we can do, except return an error condition.
	 */
	if (fmt.fmt.pix_mp.num_planes > 1) {
		errno = EINVAL;
		return -1;
	}

	return ret;
}

static int buf_ioctl(int fd, unsigned long int cmd, struct v4l2_buffer *arg)
{
	struct v4l2_buffer buf = *arg;
	struct v4l2_plane plane = { 0 };
	int ret;

	if (arg->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
	    arg->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		errno = EINVAL;
		return -1;
	}

	buf.type = convert_type(arg->type);

	if (buf.type == arg->type)
		return SYS_IOCTL(fd, cmd, &buf);

	memcpy(&plane.m, &arg->m, sizeof(plane.m));
	plane.length = arg->length;
	plane.bytesused = arg->bytesused;
	
	buf.m.planes = &plane;
	buf.length = 1;

	ret = SYS_IOCTL(fd, cmd, &buf);

	arg->index = buf.index;
	arg->memory = buf.memory;
	arg->flags = buf.flags;
	arg->field = buf.field;
	arg->timestamp = buf.timestamp;
	arg->timecode = buf.timecode;
	arg->sequence = buf.sequence;

	arg->length = plane.length;
	arg->bytesused = plane.bytesused;
	memcpy(&arg->m, &plane.m, sizeof(arg->m));

	return ret;
}

static int exbuf_ioctl(int fd, unsigned long int cmd, struct v4l2_exportbuffer *arg)
{
	if (arg->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
		arg->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		errno = EINVAL;
		return -1;
	}
	arg->type = convert_type(arg->type);
	return SYS_IOCTL(fd, cmd, arg);
}

static int plugin_ioctl(void *dev_ops_priv, int fd,
			unsigned long int cmd, void *arg)
{
	struct mplane_plugin *plugin = dev_ops_priv;

	switch (cmd) {
	case VIDIOC_QUERYCAP:
		return querycap_ioctl(fd, cmd, arg);
	case VIDIOC_TRY_FMT:
	case VIDIOC_S_FMT:
		return try_set_fmt_ioctl(plugin, fd, cmd, arg);
	case VIDIOC_G_FMT:
		return get_fmt_ioctl(plugin, fd, cmd, arg);
	case VIDIOC_ENUM_FMT:
		return enum_fmt_ioctl(plugin, fd, cmd, arg);
	case VIDIOC_S_PARM:
	case VIDIOC_G_PARM:
		return SIMPLE_CONVERT_IOCTL(fd, cmd, arg, v4l2_streamparm);
	case VIDIOC_QBUF:
	case VIDIOC_DQBUF:
	case VIDIOC_QUERYBUF:
	case VIDIOC_PREPARE_BUF:
		return buf_ioctl(fd, cmd, arg);
	case VIDIOC_CREATE_BUFS:
		return create_bufs_ioctl(fd, cmd, arg);
	case VIDIOC_REQBUFS:
		return SIMPLE_CONVERT_IOCTL(fd, cmd, arg, v4l2_requestbuffers);
	case VIDIOC_STREAMON:
	case VIDIOC_STREAMOFF:
	{
		int type = *(int *)arg;

		if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
		    type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
			errno = EINVAL;
			return -1;
		}
		type = convert_type(type);

		return SYS_IOCTL(fd, cmd, &type);
	}
	case VIDIOC_EXPBUF:
		return exbuf_ioctl(fd, cmd, arg);
	default:
		return SYS_IOCTL(fd, cmd, arg);
	}
}

static ssize_t plugin_read(void *dev_ops_priv, int fd, void *buf, size_t len)
{
	return SYS_READ(fd, buf, len);
}

static ssize_t plugin_write(void *dev_ops_priv, int fd, const void *buf,
                         size_t len)
{
	return SYS_WRITE(fd, buf, len);
}

#ifdef HAVE_V4L_BUILTIN_PLUGINS
const struct libv4l_dev_ops libv4l2_plugin_mplane = {
#else
PLUGIN_PUBLIC const struct libv4l_dev_ops libv4l2_plugin = {
#endif
	.init = &plugin_init,
	.close = &plugin_close,
	.ioctl = &plugin_ioctl,
	.read = &plugin_read,
	.write = &plugin_write,
};
