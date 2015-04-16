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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GST_MY_ELEMENT_H_
#define _GST_MY_ELEMENT_H_

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

#define GST_TYPE_MY_ELEMENT   (gst_my_element_get_type())
#define GST_MY_ELEMENT(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MY_ELEMENT,GstMyElement))
#define GST_MY_ELEMENT_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MY_ELEMENT,GstMyElementClass))
#define GST_IS_MY_ELEMENT(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MY_ELEMENT))
#define GST_IS_MY_ELEMENT_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MY_ELEMENT))

typedef struct _GstMyElement GstMyElement;
typedef struct _GstMyElementClass GstMyElementClass;

struct _GstMyElement
{
  GstBaseTransform base_myelement;

   /* Updates task */
  GstTask *updates_task;
  GRecMutex updates_lock;
  GMutex updates_timed_lock;
  GTimeVal next_update;         /* Time of the next update */
  gboolean cancelled;

  gboolean parent_info;
  GstIterator *it, *it_pads;
  gpointer point;
};

struct _GstMyElementClass
{
  GstBaseTransformClass base_myelement_class;
};

GType gst_my_element_get_type (void);

G_END_DECLS

#endif
