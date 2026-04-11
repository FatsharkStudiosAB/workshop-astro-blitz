/*
 * lightmap.c -- 2D multiplicative lighting implementation
 */

#include "lightmap.h"
#include "raylib.h"
#include "raymath.h"
#include <string.h>

/* ── Radial gradient generation ────────────────────────────────────────────── */

#define GRADIENT_SIZE 64

static Texture2D generate_gradient(void) {
    Image img = GenImageColor(GRADIENT_SIZE, GRADIENT_SIZE, BLANK);

    float center = GRADIENT_SIZE / 2.0f;
    float max_dist = center;

    for (int y = 0; y < GRADIENT_SIZE; y++) {
        for (int x = 0; x < GRADIENT_SIZE; x++) {
            float dx = (float)x - center;
            float dy = (float)y - center;
            float dist = sqrtf(dx * dx + dy * dy);
            float t = dist / max_dist;
            if (t > 1.0f) {
                t = 1.0f;
            }
            /* Quadratic falloff for soft edges */
            float brightness = (1.0f - t) * (1.0f - t);
            unsigned char a = (unsigned char)(brightness * 255.0f);
            ImageDrawPixel(&img, x, y, (Color){255, 255, 255, a});
        }
    }

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

/* ── Public implementation ─────────────────────────────────────────────────── */

void lightmap_init(LightMap *lm, int width, int height) {
    lm->target = LoadRenderTexture(width, height);
    lm->gradient = generate_gradient();
    lm->light_count = 0;
    lm->ambient = (Color){18, 18, 30, 255}; /* dark blue-black */
}

void lightmap_cleanup(LightMap *lm) {
    UnloadRenderTexture(lm->target);
    UnloadTexture(lm->gradient);
}

void lightmap_clear(LightMap *lm) {
    lm->light_count = 0;
}

void lightmap_add(LightMap *lm, Vector2 world_pos, Color color, float radius) {
    if (lm->light_count >= MAX_LIGHTS) {
        return;
    }
    Light *l = &lm->lights[lm->light_count++];
    l->position = world_pos;
    l->color = color;
    l->radius = radius;
    l->active = true;
}

void lightmap_render(LightMap *lm, Camera2D camera) {
    /* Draw light map to off-screen texture */
    BeginTextureMode(lm->target);
    ClearBackground(lm->ambient);

    BeginBlendMode(BLEND_ADDITIVE);

    for (int i = 0; i < lm->light_count; i++) {
        Light *l = &lm->lights[i];
        if (!l->active) {
            continue;
        }

        /* Convert world position to screen position */
        Vector2 screen_pos = GetWorldToScreen2D(l->position, camera);

        /* Scale radius by camera zoom */
        float screen_radius = l->radius * camera.zoom;

        /* Draw the gradient texture scaled to the light radius */
        float size = screen_radius * 2.0f;
        Rectangle src = {0, 0, (float)GRADIENT_SIZE, (float)GRADIENT_SIZE};
        Rectangle dst = {screen_pos.x - screen_radius, screen_pos.y - screen_radius, size, size};
        DrawTexturePro(lm->gradient, src, dst, (Vector2){0, 0}, 0.0f, l->color);
    }

    EndBlendMode();
    EndTextureMode();

    /* Composite light map over scene with multiplicative blending */
    BeginBlendMode(BLEND_MULTIPLIED);
    /* Light map render texture is Y-flipped */
    DrawTextureRec(
        lm->target.texture,
        (Rectangle){0, 0, (float)lm->target.texture.width, -(float)lm->target.texture.height},
        (Vector2){0, 0}, WHITE);
    EndBlendMode();
}

void lightmap_render_scaled(LightMap *lm, Camera2D camera, float intensity) {
    if (intensity <= 0.0f) {
        return; /* lighting disabled -- skip entirely */
    }

    /* At full intensity, use standard render */
    if (intensity >= 1.0f) {
        lightmap_render(lm, camera);
        return;
    }

    /* Partial intensity: lighten the ambient to reduce the darkening effect.
     * Interpolate ambient from the real ambient towards full white (no shadow). */
    Color original_ambient = lm->ambient;
    unsigned char ar = (unsigned char)((float)original_ambient.r +
                                       (255.0f - (float)original_ambient.r) * (1.0f - intensity));
    unsigned char ag = (unsigned char)((float)original_ambient.g +
                                       (255.0f - (float)original_ambient.g) * (1.0f - intensity));
    unsigned char ab = (unsigned char)((float)original_ambient.b +
                                       (255.0f - (float)original_ambient.b) * (1.0f - intensity));
    lm->ambient = (Color){ar, ag, ab, 255};

    lightmap_render(lm, camera);

    /* Restore original ambient */
    lm->ambient = original_ambient;
}
