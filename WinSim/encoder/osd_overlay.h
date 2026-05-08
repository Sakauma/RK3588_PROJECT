#ifndef RK3588_OSD_OVERLAY_H
#define RK3588_OSD_OVERLAY_H

#include "encoder_types.h"
#include <stdint.h>
#include <vector>

void DrawOsdNv12(uint8_t* nv12,
	int width,
	int height,
	int hor_stride,
	int ver_stride,
	const std::vector<OsdObject>& objects);

std::vector<OsdObject> MakeTestOsdObjects(int width, int height);

#endif
