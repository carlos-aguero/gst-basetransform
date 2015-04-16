/* GStreamer
 * Copyright (C) 2015 FIXME <fixme@example.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstmyelement
 *
 * The myelement element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v fakesrc ! myelement ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include "gstmyelement.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <err.h>


char response[] = "HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html><html><head><title>Bye-bye baby bye-bye</title>"
"<style>body { background-color: #111 }"
"h1 { font-size:4cm; text-align: center; color: black;"
" text-shadow: 0 0 2mm red}</style></head>"
"<body><h1>Goodbye, world!</h1></body></html>\r\n";

GST_DEBUG_CATEGORY_STATIC (gst_my_element_debug_category);
#define GST_CAT_DEFAULT gst_my_element_debug_category

/* prototypes */


static void gst_my_element_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_my_element_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_my_element_dispose (GObject * object);
static void gst_my_element_finalize (GObject * object);

static GstCaps *gst_my_element_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter);
static GstCaps *gst_my_element_fixate_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * othercaps);
static gboolean gst_my_element_accept_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps);
static gboolean gst_my_element_set_caps (GstBaseTransform * trans,
    GstCaps * incaps, GstCaps * outcaps);
static gboolean gst_my_element_query (GstBaseTransform * trans,
    GstPadDirection direction, GstQuery * query);
static gboolean gst_my_element_decide_allocation (GstBaseTransform * trans,
    GstQuery * query);
static gboolean gst_my_element_filter_meta (GstBaseTransform * trans,
    GstQuery * query, GType api, const GstStructure * params);
static gboolean gst_my_element_propose_allocation (GstBaseTransform * trans,
    GstQuery * decide_query, GstQuery * query);
static gboolean gst_my_element_transform_size (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, gsize size, GstCaps * othercaps,
    gsize * othersize);
static gboolean gst_my_element_get_unit_size (GstBaseTransform * trans,
    GstCaps * caps, gsize * size);
static gboolean gst_my_element_start (GstBaseTransform * trans);
static gboolean gst_my_element_stop (GstBaseTransform * trans);
static gboolean gst_my_element_sink_event (GstBaseTransform * trans,
    GstEvent * event);
static gboolean gst_my_element_src_event (GstBaseTransform * trans,
    GstEvent * event);
static GstFlowReturn gst_my_element_prepare_output_buffer (GstBaseTransform *
    trans, GstBuffer * input, GstBuffer ** outbuf);
static gboolean gst_my_element_copy_metadata (GstBaseTransform * trans,
    GstBuffer * input, GstBuffer * outbuf);
static gboolean gst_my_element_transform_meta (GstBaseTransform * trans,
    GstBuffer * outbuf, GstMeta * meta, GstBuffer * inbuf);
static void gst_my_element_before_transform (GstBaseTransform * trans,
    GstBuffer * buffer);
static GstFlowReturn gst_my_element_transform (GstBaseTransform * trans,
    GstBuffer * inbuf, GstBuffer * outbuf);
static GstFlowReturn gst_my_element_transform_ip (GstBaseTransform * trans,
    GstBuffer * buf);
void gst_hls_demux_updates_loop (GstMyElement * myelement);


enum
{
  PROP_0
};

/* pad templates */

static GstStaticPadTemplate gst_my_element_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate gst_my_element_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstMyElement, gst_my_element, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT (gst_my_element_debug_category, "myelement", 0,
        "debug category for myelement element"));

static void
gst_my_element_class_init (GstMyElementClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *base_transform_class =
      GST_BASE_TRANSFORM_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_static_pad_template_get (&gst_my_element_src_template));
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_static_pad_template_get (&gst_my_element_sink_template));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "FIXME Long name", "Generic", "FIXME Description",
      "FIXME <fixme@example.com>");

  gobject_class->set_property = gst_my_element_set_property;
  gobject_class->get_property = gst_my_element_get_property;
  gobject_class->dispose = gst_my_element_dispose;
  gobject_class->finalize = gst_my_element_finalize;
  base_transform_class->transform_caps =
      GST_DEBUG_FUNCPTR (gst_my_element_transform_caps);
/*  base_transform_class->fixate_caps =
      GST_DEBUG_FUNCPTR (gst_my_element_fixate_caps);
  base_transform_class->accept_caps =
      GST_DEBUG_FUNCPTR (gst_my_element_accept_caps);
  base_transform_class->set_caps = GST_DEBUG_FUNCPTR (gst_my_element_set_caps);
  base_transform_class->query = GST_DEBUG_FUNCPTR (gst_my_element_query); */
/*  base_transform_class->decide_allocation =
      GST_DEBUG_FUNCPTR (gst_my_element_decide_allocation);
  base_transform_class->filter_meta =
      GST_DEBUG_FUNCPTR (gst_my_element_filter_meta);
  base_transform_class->propose_allocation =
      GST_DEBUG_FUNCPTR (gst_my_element_propose_allocation);
  base_transform_class->transform_size =
      GST_DEBUG_FUNCPTR (gst_my_element_transform_size);
  base_transform_class->get_unit_size =
      GST_DEBUG_FUNCPTR (gst_my_element_get_unit_size);
  base_transform_class->start = GST_DEBUG_FUNCPTR (gst_my_element_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_my_element_stop);
  base_transform_class->sink_event =
      GST_DEBUG_FUNCPTR (gst_my_element_sink_event);
  base_transform_class->src_event =
      GST_DEBUG_FUNCPTR (gst_my_element_src_event);
  base_transform_class->prepare_output_buffer =
      GST_DEBUG_FUNCPTR (gst_my_element_prepare_output_buffer);
  base_transform_class->copy_metadata =
      GST_DEBUG_FUNCPTR (gst_my_element_copy_metadata);
  base_transform_class->transform_meta =
      GST_DEBUG_FUNCPTR (gst_my_element_transform_meta);*/
/*  base_transform_class->before_transform =
      GST_DEBUG_FUNCPTR (gst_my_element_before_transform);*/
/*  base_transform_class->transform =
      GST_DEBUG_FUNCPTR (gst_my_element_transform);*/
  base_transform_class->transform_ip =
      GST_DEBUG_FUNCPTR (gst_my_element_transform_ip);

}

void gst_hls_demux_updates_loop(GstMyElement * myelement)
{
  GST_INFO_OBJECT (myelement, "INIT Web Server");

  while (1) {

    int one = 1, client_fd;
    struct sockaddr_in svr_addr, cli_addr;
    socklen_t sin_len = sizeof(cli_addr);

    GST_INFO_OBJECT (myelement, "Element Init"); 
 
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
      GST_ERROR_OBJECT (myelement, "Can't open socket");
    }
 
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
 
    int port = 8080;
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    svr_addr.sin_port = htons(port);
 
    if (bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1) {
      close(sock);
      GST_ERROR_OBJECT (myelement, "can't bind");
    }
 
    listen(sock, 5);
    while (1) {
      client_fd = accept(sock, (struct sockaddr *) &cli_addr, &sin_len);
      GST_INFO_OBJECT (myelement, "Got Connection");
   
      if (client_fd == -1) {
        GST_ERROR_OBJECT (myelement, "Can't Accept");
        continue;
      }
      write(client_fd, response, sizeof(response) - 1); /*-1:'\0'*/
      close(client_fd);
    }
  }
}

static void
gst_my_element_init (GstMyElement * myelement)
{

  g_rec_mutex_init (&myelement->updates_lock);
  myelement->updates_task =
      gst_task_new ((GstTaskFunction) gst_hls_demux_updates_loop, myelement, NULL);
  gst_task_set_lock (myelement->updates_task, &myelement->updates_lock);
  g_mutex_init (&myelement->updates_timed_lock);

  gst_task_start (myelement->updates_task);

  myelement->parent_info = FALSE;

}

void
gst_my_element_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstMyElement *myelement = GST_MY_ELEMENT (object);

  GST_DEBUG_OBJECT (myelement, "set_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_my_element_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstMyElement *myelement = GST_MY_ELEMENT (object);

  GST_DEBUG_OBJECT (myelement, "get_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_my_element_dispose (GObject * object)
{
  GstMyElement *myelement = GST_MY_ELEMENT (object);

  GST_DEBUG_OBJECT (myelement, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_my_element_parent_class)->dispose (object);
}

void
gst_my_element_finalize (GObject * object)
{
  GstMyElement *myelement = GST_MY_ELEMENT (object);

  GST_DEBUG_OBJECT (myelement, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_my_element_parent_class)->finalize (object);
}

static GstCaps *
gst_my_element_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);
  GstCaps *othercaps;

  GST_DEBUG_OBJECT (myelement, "transform_caps");

  othercaps = gst_caps_copy (caps);

  /* Copy other caps and modify as appropriate */
  /* This works for the simplest cases, where the transform modifies one
   * or more fields in the caps structure.  It does not work correctly
   * if passthrough caps are preferred. */
  if (direction == GST_PAD_SRC) {
    /* transform caps going upstream */
  } else {
    /* transform caps going downstream */
  }

  if (filter) {
    GstCaps *intersect;

    intersect = gst_caps_intersect (othercaps, filter);
    gst_caps_unref (othercaps);

    return intersect;
  } else {
    return othercaps;
  }
}

static GstCaps *
gst_my_element_fixate_caps (GstBaseTransform * trans, GstPadDirection direction,
    GstCaps * caps, GstCaps * othercaps)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);

  GST_DEBUG_OBJECT (myelement, "fixate_caps");

  return NULL;
}

static gboolean
gst_my_element_accept_caps (GstBaseTransform * trans, GstPadDirection direction,
    GstCaps * caps)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);

  GST_DEBUG_OBJECT (myelement, "accept_caps");

  return TRUE;
}

static gboolean
gst_my_element_set_caps (GstBaseTransform * trans, GstCaps * incaps,
    GstCaps * outcaps)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);

  GST_DEBUG_OBJECT (myelement, "set_caps");

  return TRUE;
}

static gboolean
gst_my_element_query (GstBaseTransform * trans, GstPadDirection direction,
    GstQuery * query)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);

  GST_DEBUG_OBJECT (myelement, "query");

  return TRUE;
}

/* decide allocation query for output buffers */
static gboolean
gst_my_element_decide_allocation (GstBaseTransform * trans, GstQuery * query)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);

  GST_DEBUG_OBJECT (myelement, "decide_allocation");

  return TRUE;
}

static gboolean
gst_my_element_filter_meta (GstBaseTransform * trans, GstQuery * query,
    GType api, const GstStructure * params)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);

  GST_DEBUG_OBJECT (myelement, "filter_meta");

  return TRUE;
}

/* propose allocation query parameters for input buffers */
static gboolean
gst_my_element_propose_allocation (GstBaseTransform * trans,
    GstQuery * decide_query, GstQuery * query)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);

  GST_DEBUG_OBJECT (myelement, "propose_allocation");

  return TRUE;
}

/* transform size */
static gboolean
gst_my_element_transform_size (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, gsize size, GstCaps * othercaps,
    gsize * othersize)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);

  GST_DEBUG_OBJECT (myelement, "transform_size");

  return TRUE;
}

static gboolean
gst_my_element_get_unit_size (GstBaseTransform * trans, GstCaps * caps,
    gsize * size)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);

  GST_DEBUG_OBJECT (myelement, "get_unit_size");

  return TRUE;
}

/* states */
static gboolean
gst_my_element_start (GstBaseTransform * trans)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);

  GST_DEBUG_OBJECT (myelement, "start");

  return TRUE;
}

static gboolean
gst_my_element_stop (GstBaseTransform * trans)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);

  GST_DEBUG_OBJECT (myelement, "stop");

  return TRUE;
}

/* sink and src pad event handlers */
static gboolean
gst_my_element_sink_event (GstBaseTransform * trans, GstEvent * event)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);

  GST_DEBUG_OBJECT (myelement, "sink_event");

  return GST_BASE_TRANSFORM_CLASS (gst_my_element_parent_class)->
      sink_event (trans, event);
}

static gboolean
gst_my_element_src_event (GstBaseTransform * trans, GstEvent * event)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);

  GST_DEBUG_OBJECT (myelement, "src_event");

  return GST_BASE_TRANSFORM_CLASS (gst_my_element_parent_class)->
      src_event (trans, event);
}

static GstFlowReturn
gst_my_element_prepare_output_buffer (GstBaseTransform * trans,
    GstBuffer * input, GstBuffer ** outbuf)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);

  GST_DEBUG_OBJECT (myelement, "prepare_output_buffer");

  return GST_FLOW_OK;
}

/* metadata */
static gboolean
gst_my_element_copy_metadata (GstBaseTransform * trans, GstBuffer * input,
    GstBuffer * outbuf)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);

  GST_DEBUG_OBJECT (myelement, "copy_metadata");

  return TRUE;
}

static gboolean
gst_my_element_transform_meta (GstBaseTransform * trans, GstBuffer * outbuf,
    GstMeta * meta, GstBuffer * inbuf)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);

  GST_DEBUG_OBJECT (myelement, "transform_meta");

  return TRUE;
}

static void
gst_my_element_before_transform (GstBaseTransform * trans, GstBuffer * buffer)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);

  GST_DEBUG_OBJECT (myelement, "before_transform");

}

/* transform */
static GstFlowReturn
gst_my_element_transform (GstBaseTransform * trans, GstBuffer * inbuf,
    GstBuffer * outbuf)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);

  GST_DEBUG_OBJECT (myelement, "transform");

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_my_element_transform_ip (GstBaseTransform * trans, GstBuffer * buf)
{
  GstMyElement *myelement = GST_MY_ELEMENT (trans);


  if (!myelement->parent_info)
  {
    GST_DEBUG_OBJECT (myelement, "transform_ip");
//  GST_ERROR_OBJECT (myelement, ">>>>>>>>>>>>>>>>>>>>>>> NAME: %s", GST_ELEMENT_NAME(myelement));
    GstElement *parent = gst_element_get_parent(myelement);
    GST_INFO_OBJECT (myelement, "Element parent pointer: 0x%p", parent);
    myelement->parent_info = TRUE;

//  GST_ERROR_OBJECT (myelement, ">>>>PARENT: 0x%p", gst_object_get_parent(myelement));
//  GST_ERROR_OBJECT (myelement, ">>>>PARENT: 0x%p", GST_ELEMENT_PARENT(myelement));
//    GstIterator * gst_bin_iterate_elements (GstBin *bin);
    myelement->it = gst_bin_iterate_elements (parent);

    while (gst_iterator_next (myelement->it, &myelement->point) == GST_ITERATOR_OK) {
//      GstElement* element = GST_ELEMENT (myelement->point);
//AT THIS POINT ITERATING THE AMOUNT OF ELEMENTS AT THE GST-LAUNCH PIPELINE
      GST_INFO_OBJECT(myelement, "Iterator");

//      myelement->it_pads = gst_element_iterate_pads (element);
//      printf ("element -> %s\n", gst_element_get_name (element));

/*      while (gst_iterator_next (myelement->it_pads, &myelement->point_pad) == GST_ITERATOR_OK) {
        GstPad * pad = GST_PAD (point_pad);
        printf ("pad-> %s\n", gst_element_get_name (pad));
      }*/
    }
  }

  return GST_FLOW_OK;
}

static gboolean
plugin_init (GstPlugin * plugin)
{

  /* FIXME Remember to set the rank if it's an element that is meant
     to be autoplugged by decodebin. */
  return gst_element_register (plugin, "myelement", GST_RANK_NONE,
      GST_TYPE_MY_ELEMENT);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.0.FIXME"
#endif
#ifndef PACKAGE
#define PACKAGE "FIXME_package"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "FIXME_package_name"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://FIXME.org/"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    myelement,
    "FIXME plugin description",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)
