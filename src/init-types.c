/*
 * ClutterSmith - a visual authoring environment for clutter.
 * Copyright (c) 2009, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * Alternatively, you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 3, with the following additional permissions:
 *
 * 1. Intel grants you an additional permission under Section 7 of the
 * GNU General Public License, version 3, exempting you from the
 * requirement in Section 6 of the GNU General Public License, version 3,
 * to accompany Corresponding Source with Installation Information for
 * the Program or any work based on the Program.  You are still required
 * to comply with all other Section 6 requirements to provide
 * Corresponding Source.
 *
 * 2. Intel grants you an additional permission under Section 7 of the
 * GNU General Public License, version 3, allowing you to convey the
 * Program or a work based on the Program in combination with or linked
 * to any works licensed under the GNU General Public License version 2,
 * with the terms and conditions of the GNU General Public License
 * version 2 applying to the combined or linked work as a whole.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Written by: Øyvind Kolås <oyvind.kolas@intel.com>
 */


#include <clutter/clutter.h>
#include <mx/mx.h>
#include "cs-texture.h"

gpointer foo = NULL;

static void o(GType type)
{
  if (!G_TYPE_IS_INTERFACE (type))
    {
      foo = g_type_class_ref (type);
    }
}

void init_types(void){
o(CLUTTER_TYPE_ACTOR);
o(CLUTTER_TYPE_ALLOCATION_FLAGS);
o(CLUTTER_TYPE_ALPHA);
o(CLUTTER_TYPE_ANIMATABLE);
o(CLUTTER_TYPE_ANIMATION);
o(CLUTTER_TYPE_ANIMATION_MODE);
o(CLUTTER_TYPE_BACKEND);
o(CLUTTER_TYPE_BEHAVIOUR);
o(CLUTTER_TYPE_BEHAVIOUR_DEPTH);
o(CLUTTER_TYPE_BEHAVIOUR_ELLIPSE);
o(CLUTTER_TYPE_BEHAVIOUR_OPACITY);
o(CLUTTER_TYPE_BEHAVIOUR_PATH);
o(CLUTTER_TYPE_BEHAVIOUR_ROTATE);
o(CLUTTER_TYPE_BEHAVIOUR_SCALE);
o(CLUTTER_TYPE_BINDING_POOL);
o(CLUTTER_TYPE_CAIRO_TEXTURE);
o(CLUTTER_TYPE_CHILD_META);
o(CLUTTER_TYPE_CLONE);
o(CLUTTER_TYPE_CONTAINER);
o(CLUTTER_TYPE_EVENT_FLAGS);
o(CLUTTER_TYPE_EVENT_TYPE);
o(CLUTTER_TYPE_FEATURE_FLAGS);
o(CLUTTER_TYPE_FONT_FLAGS);
o(CLUTTER_TYPE_GRAVITY);
o(CLUTTER_TYPE_GROUP);
o(CLUTTER_TYPE_INIT_ERROR);
o(CLUTTER_TYPE_INPUT_DEVICE_TYPE);
o(CLUTTER_TYPE_INTERVAL);
o(CLUTTER_TYPE_LIST_MODEL);
o(CLUTTER_TYPE_MEDIA);
o(CLUTTER_TYPE_MODEL);
o(CLUTTER_TYPE_MODEL_ITER);
o(CLUTTER_TYPE_MODIFIER_TYPE);
o(CLUTTER_TYPE_PARAM_COLOR);
o(CLUTTER_TYPE_PARAM_FIXED);
o(CLUTTER_TYPE_PARAM_UNITS);
o(CLUTTER_TYPE_RECTANGLE);
o(CLUTTER_TYPE_STAGE);
o(CLUTTER_TYPE_TEXT);
o(CLUTTER_TYPE_TEXTURE);
o(MX_TYPE_BIN);
o(MX_TYPE_BOX_LAYOUT);
o(MX_TYPE_BUTTON);
o(MX_TYPE_CLIPBOARD);
o(MX_TYPE_COMBO_BOX);
o(MX_TYPE_DRAG_AXIS);
o(MX_TYPE_ENTRY);
o(MX_TYPE_EXPANDER);
o(MX_TYPE_GRID);
o(MX_TYPE_ICON);
o(MX_TYPE_ITEM_FACTORY);
o(MX_TYPE_ITEM_VIEW);
o(MX_TYPE_LABEL);
o(MX_TYPE_LIST_VIEW);
o(MX_TYPE_PROGRESS_BAR);
o(MX_TYPE_SCROLLABLE);
o(MX_TYPE_SCROLL_BAR);
o(MX_TYPE_SCROLL_VIEW);
o(MX_TYPE_STYLABLE);
o(MX_TYPE_STYLE);
o(MX_TYPE_STYLE_ERROR);
o(MX_TYPE_TABLE);
o(MX_TYPE_TABLE_CHILD);
o(MX_TYPE_TEXTURE_CACHE);
o(MX_TYPE_TEXTURE_FRAME);
o(MX_TYPE_TOOLTIP);
o(MX_TYPE_VIEWPORT);
o(MX_TYPE_WIDGET);
o(MX_TYPE_SLIDER);
o(CB_TYPE_TEXTURE);
o(CB_TYPE_TEXTURE);
}
