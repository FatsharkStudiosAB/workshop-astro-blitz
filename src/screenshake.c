/*
 * screenshake.c -- Screen shake implementation
 */

#include "screenshake.h"

void screenshake_init(ScreenShake *shake) {
    shake->trauma = 0.0f;
}

void screenshake_add_trauma(ScreenShake *shake, float amount) {
    shake->trauma += amount;
    if (shake->trauma > 1.0f) {
        shake->trauma = 1.0f;
    }
}

void screenshake_update(ScreenShake *shake, float dt) {
    shake->trauma -= SHAKE_DECAY_RATE * dt;
    if (shake->trauma < 0.0f) {
        shake->trauma = 0.0f;
    }
}

Camera2D screenshake_apply(const ScreenShake *shake, Camera2D camera) {
    if (shake->trauma <= 0.0f) {
        return camera;
    }

    /* Shake intensity is trauma squared for a more pleasing falloff */
    float intensity = shake->trauma * shake->trauma;

    /* Random offset scaled by intensity and max offset */
    float offset_x = ((float)GetRandomValue(-100, 100) / 100.0f) * SHAKE_MAX_OFFSET * intensity;
    float offset_y = ((float)GetRandomValue(-100, 100) / 100.0f) * SHAKE_MAX_OFFSET * intensity;

    camera.target.x += offset_x;
    camera.target.y += offset_y;

    return camera;
}
