#pragma once

#define EMU_WIDTH       1024
#define EMU_HEIGHT      768
#define EMU_SCALE       1
#define SCREEN_WIDTH    (EMU_WIDTH*EMU_SCALE)
#define SCREEN_HEIGHT   (EMU_HEIGHT*EMU_SCALE)
#define Z_NEAR          0.1f
#define Z_FAR           5000.0f
#define PERSPECTIVE_FOV 45.f
#define ORTHO_SIZE      100.f
#define FPS_CAP         60.f