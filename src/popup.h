/*
 * Hornsey - Moblin Media Player.
 * Copyright © 2009 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef HRN_POPUP_H
#define HRN_POPUP_H

void          popup_actor          (ClutterActor *stage,
                                    gint          x,
                                    gint          y,
                                    ClutterActor *actor);

void          popup_actor_fixed    (ClutterActor *stage,
                                    gint          x,
                                    gint          y,
                                    ClutterActor *actor);
void          popup_close          (void);
ClutterActor *popup_actions        (gpointer     *actions,
                                    gpointer      userdata);
ClutterActor *popup_actions_bolded (gpointer     *actions,
                                    gpointer      userdata,
                                    gint          boldno);

#endif
