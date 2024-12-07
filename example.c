#include <assert.h>

#include <raylib.h>

#ifndef _WIN32
#include <sys/stat.h>
#endif // _WIN32

#define AMRL_IMPLEMENTATION
#include "amrl.h"

#define SCREEN_FACTOR 100

#define TEXTURE_PATH "raylib_logo.png"
#define FONT_PATH "BigBlueTerm437NerdFont-Regular.ttf"

int main(void) {
    AmAssetManager am = {0};

    InitWindow(SCREEN_FACTOR * 16, SCREEN_FACTOR * 9, "amrl example");

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(WHITE);

        Font font = am_get_font(&am, FONT_PATH);
        Texture tex = am_get_texture(&am, TEXTURE_PATH);

        DrawTexture(tex, 0, 0, WHITE);
        DrawTextEx(font, "raylib", (Vector2){0, tex.height + 5}, 100, 1, RED);

        EndDrawing();
    }

    am_unload_all(&am);
    CloseWindow();
    return 0;
}
