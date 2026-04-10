/*
 * postfx.c -- Post-processing effects implementation
 *
 * Combined single-pass shader: bloom glow, CRT scanlines,
 * chromatic aberration, and vignette.
 */

#include "postfx.h"
#include "raylib.h"

/* ── GLSL 330 fragment shader (embedded) ───────────────────────────────────── */

static const char *postfx_fs =
    "#version 330\n"
    "\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "uniform float time;\n"
    "uniform vec2 resolution;\n"
    "uniform float bloomIntensity;\n"
    "\n"
    "out vec4 finalColor;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vec2 uv = fragTexCoord;\n"
    "\n"
    "    // ── Chromatic aberration ──────────────────────────────\n"
    "    float aberration = 0.002;\n"
    "    float r = texture(texture0, uv + vec2( aberration, 0.0)).r;\n"
    "    float g = texture(texture0, uv).g;\n"
    "    float b = texture(texture0, uv + vec2(-aberration, 0.0)).b;\n"
    "    vec3 color = vec3(r, g, b);\n"
    "\n"
    "    // ── Bloom (box blur 5x5 + additive blend) ────────────\n"
    "    vec3 bloom = vec3(0.0);\n"
    "    vec2 texel = 1.0 / resolution;\n"
    "    for (int x = -2; x <= 2; x++) {\n"
    "        for (int y = -2; y <= 2; y++) {\n"
    "            vec2 offset = vec2(float(x), float(y)) * texel * 2.0;\n"
    "            vec3 s = texture(texture0, uv + offset).rgb;\n"
    "            // Extract bright pixels (threshold 0.5)\n"
    "            float brightness = dot(s, vec3(0.2126, 0.7152, 0.0722));\n"
    "            if (brightness > 0.5) {\n"
    "                bloom += s * (brightness - 0.5) * 2.0;\n"
    "            }\n"
    "        }\n"
    "    }\n"
    "    bloom /= 25.0;\n"
    "    color += bloom * bloomIntensity;\n"
    "\n"
    "    // ── CRT scanlines ────────────────────────────────────\n"
    "    float scanline = sin(fragTexCoord.y * resolution.y * 3.14159) * 0.04;\n"
    "    color -= scanline;\n"
    "\n"
    "    // ── Vignette ─────────────────────────────────────────\n"
    "    vec2 vig_uv = fragTexCoord * 2.0 - 1.0;\n"
    "    float vignette = 1.0 - dot(vig_uv, vig_uv) * 0.35;\n"
    "    color *= vignette;\n"
    "\n"
    "    // ── Slight color grading (boost cyan/teal highlights) ─\n"
    "    color.g *= 1.02;\n"
    "    color.b *= 1.04;\n"
    "\n"
    "    finalColor = vec4(color, 1.0);\n"
    "}\n";

/* ── Public implementation ─────────────────────────────────────────────────── */

void postfx_init(PostFX *pfx, int width, int height) {
    pfx->target = LoadRenderTexture(width, height);
    pfx->shader = LoadShaderFromMemory(NULL, postfx_fs);
    pfx->time_loc = GetShaderLocation(pfx->shader, "time");
    pfx->resolution_loc = GetShaderLocation(pfx->shader, "resolution");
    pfx->bloom_intensity_loc = GetShaderLocation(pfx->shader, "bloomIntensity");
    pfx->enabled = true;

    /* Set static uniforms */
    float res[2] = {(float)width, (float)height};
    SetShaderValue(pfx->shader, pfx->resolution_loc, res, SHADER_UNIFORM_VEC2);

    float bloom = 1.5f;
    SetShaderValue(pfx->shader, pfx->bloom_intensity_loc, &bloom, SHADER_UNIFORM_FLOAT);
}

void postfx_cleanup(PostFX *pfx) {
    UnloadShader(pfx->shader);
    UnloadRenderTexture(pfx->target);
}

void postfx_begin(PostFX *pfx) {
    if (pfx->enabled) {
        BeginTextureMode(pfx->target);
    } else {
        BeginDrawing();
    }
    ClearBackground(BLACK);
}

void postfx_end(PostFX *pfx, float time) {
    if (pfx->enabled) {
        EndTextureMode();

        /* Update time uniform */
        SetShaderValue(pfx->shader, pfx->time_loc, &time, SHADER_UNIFORM_FLOAT);

        BeginDrawing();
        ClearBackground(BLACK);
        BeginShaderMode(pfx->shader);
        /* RenderTexture is Y-flipped, so draw with negative height */
        DrawTextureRec(
            pfx->target.texture,
            (Rectangle){0, 0, (float)pfx->target.texture.width, -(float)pfx->target.texture.height},
            (Vector2){0, 0}, WHITE);
        EndShaderMode();
        EndDrawing();
    } else {
        EndDrawing();
    }
}

void postfx_toggle(PostFX *pfx) {
    pfx->enabled = !pfx->enabled;
}
