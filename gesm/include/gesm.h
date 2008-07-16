/*
 * Copyright (C) 2008 by OpenMoko, Inc.
 * Written by Paul-Valentin Borza <gestures@borza.ro>
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef GESM_H_
#define GESM_H_

#include "ges.h"

typedef enum device {
	dev_none = 0,
	dev_wii1,
	dev_neo2,
	dev_neo3
} device;

typedef void (* class_process)(struct sample_3d_t sample[], unsigned int sample_len);
typedef void (* model_process)(struct accel_3d_t accel[], unsigned int accel_len);

#endif /*GESM_H_*/
