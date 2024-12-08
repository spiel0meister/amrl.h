#ifndef AMRL_H_
#define AMRL_H_
#include <stddef.h>

#include <raylib.h>

#ifndef _WIN32
#include <sys/time.h>
#else // _WIN32
#endif // _WIN32

typedef enum {
    ASSET_IMAGE,
    ASSET_TEXTURE,
    ASSET_FONT,
    ASSET_SHADER,
}AmAssetType;

typedef union {
    Image image;
    Texture texture;
    Font font;
    Shader shader;
}AmAssetAs;

typedef struct {
    const char* path1;
    const char* path2;

#ifndef _WIN32
    time_t last_modified1;
    time_t last_modified2;
#else // _WIN32
#endif // _WIN32

    AmAssetType type;
    AmAssetAs as;
}AmAsset;

typedef struct {
    AmAsset* assets;
    size_t count;
    size_t capactiy;
}AmAssetManager;

void am_unload_all(AmAssetManager* self);
void am_destroy(AmAssetManager* self);

Image am_get_image(AmAssetManager* self, const char* path);
Texture am_get_texture(AmAssetManager* self, const char* path);
Font am_get_font(AmAssetManager* self, const char* path);
Shader am_get_shader(AmAssetManager* self, const char* vsPath, const char* fsPath);

#endif // AMRL_H_

/*
   Copyright 2024 Žan Sovič <soviczan7@gmail.com>

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
   LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifdef AMRL_IMPLEMENTATION
#undef AMRL_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

AmAsset* am_append_asset(AmAssetManager* self, AmAsset asset) {
    if (self->count >= self->capactiy) {
        if (self->capactiy == 0) self->capactiy = 8;
        while (self->count >= self->capactiy) self->capactiy *= 2;
        self->assets = realloc(self->assets, sizeof(AmAsset) * self->capactiy);
    }
    AmAsset* asset_ = &self->assets[self->count++];
    *asset_ = asset;
    return asset_;
}

void am_remove_unordered_without_unloading(AmAssetManager* self, size_t i) {
    assert(i < self->count);

    if (self->count == 1) self->count--;
    else self->assets[i] = self->assets[--self->count];
}

void am_destroy(AmAssetManager* self) {
    free(self->assets);
    memset(self, 0, sizeof(*self));
}

AmAsset* am_load_asset(AmAssetManager* self, AmAssetType type, ...) {
    va_list args;

    va_start(args, type);
    const char* path = va_arg(args, const char*);
    const char* path2 = NULL;

    if (type == ASSET_SHADER) {
        path2 = va_arg(args, const char*);
    }

    bool valid = false;
    AmAsset asset = {0};
    asset.type = type;
    asset.path1 = path;

    switch (asset.type) {
        case ASSET_IMAGE:
            asset.as.image = LoadImage(path);
            valid = IsImageValid(asset.as.image);
            break;
        case ASSET_TEXTURE: {
            Image image = LoadImage(path);
            if (!IsImageValid(image)) {
                valid = false;
                break;
            }

            asset.as.texture = LoadTextureFromImage(image);
            UnloadImage(image);
            valid = IsTextureValid(asset.as.texture);
            break;
        }break;
        case ASSET_FONT: {
            asset.as.font = LoadFont(path);
            valid = IsFontValid(asset.as.font);
            break;
        }break;
        case ASSET_SHADER: {
            asset.path2 = path2;
            asset.as.shader = LoadShader(path, path2);
            valid = IsShaderValid(asset.as.shader);
            break;
        }break;
    }

    va_end(args);
    
    if (valid) {
        struct stat st;
        if (stat(path, &st) < 0) {
            TraceLog(LOG_INFO, "ASSET MANAGER: Couldn't stat asset \"%s\"", path);
            return NULL;
        }
        asset.last_modified1 = st.st_mtime;

        if (type == ASSET_SHADER) {
            if (stat(path2, &st) < 0) {
                TraceLog(LOG_INFO, "ASSET MANAGER: Couldn't stat asset \"%s\"", path);
                return NULL;
            }
            asset.last_modified2 = st.st_mtime;
        }

        TraceLog(LOG_INFO, "ASSET MANAGER: Loaded asset \"%s\"", path);
        return am_append_asset(self, asset);
    }

    return NULL;
}

void am_unload_single_asset(AmAsset* asset) {
    switch (asset->type) {
        case ASSET_IMAGE:
            UnloadImage(asset->as.image);
            break;
        case ASSET_TEXTURE:
            UnloadTexture(asset->as.texture);
            break;
        case ASSET_FONT:
            UnloadFont(asset->as.font);
            break;
        case ASSET_SHADER:
            UnloadShader(asset->as.shader);
            break;
    }

    memset(asset, 0, sizeof(*asset));
}

AmAsset* am_get_asset(AmAssetManager* self, AmAssetType type, ...) {
    va_list args;
    va_start(args, type);

    AmAsset* result = NULL;
    const char* path1 = va_arg(args, const char*);
    const char* path2 = NULL;

    if (type == ASSET_SHADER) {
        path2 = va_arg(args, const char*);
    }

    if (type == ASSET_SHADER) {
        for (size_t i = 0; i < self->count; ++i) {
            AmAsset* asset = &self->assets[i];
            bool is_right_asset = true;
            if (asset->path1 != NULL && path1 != NULL) {
                is_right_asset &= strcmp(asset->path1, path1) == 0;
            }
            if (asset->path2 != NULL && path2 != NULL) {
                is_right_asset &= strcmp(asset->path2, path2) == 0;
            }
            if (is_right_asset) {
                result = asset;
                break;
            }
        }

        if (result == NULL) {
            result = am_load_asset(self, type, path1, path2);
        }
    } else {
        for (size_t i = 0; i < self->count; ++i) {
            AmAsset* asset = &self->assets[i];
            if (strcmp(asset->path1, path1) == 0) {
                result = asset;
                break;
            }
        }

        if (result == NULL) {
            result = am_load_asset(self, type, path1);
        }
    }

    struct stat st;
    if (stat(path1, &st) < 0) {
        TraceLog(LOG_INFO, "ASSET MANAGER: Couldn't stat asset \"%s\"", path1);
    }

    if (result != NULL && st.st_mtime > result->last_modified1) {
        am_unload_single_asset(result);
        am_remove_unordered_without_unloading(self, result - self->assets);
        result = am_load_asset(self, type, path1);
    }

    if (type == ASSET_SHADER) {
        if (stat(path2, &st) < 0) {
            TraceLog(LOG_INFO, "ASSET MANAGER: Couldn't stat asset \"%s\"", path1);
        }

        if (result != NULL && st.st_mtime > result->last_modified2) {
            am_unload_single_asset(result);
            am_remove_unordered_without_unloading(self, result - self->assets);
            result = am_load_asset(self, type, path1, path2);
        }
    }

    return result;
}

void am_unload_asset(AmAssetManager* self, AmAssetType type, ...) {
    va_list args;

    va_start(args, type);
    const char* path1 = va_arg(args, const char*);

    AmAsset* asset = NULL;

    if (type == ASSET_SHADER) {
        const char* path2 = va_arg(args, const char*);
        asset = am_get_asset(self, type, path1, path2);
    } else {
        asset = am_get_asset(self, type, path1);
    }

    if (asset != NULL) {
        am_unload_single_asset(asset);
        am_remove_unordered_without_unloading(self, asset - self->assets);
    
        TraceLog(LOG_INFO, "ASSET MANAGER: Unloaded asset \"%s\"", path1);
    }

    va_end(args);
}

void am_unload_all(AmAssetManager* self) {
    for (size_t i = 0; i < self->count; ++i) {
        AmAsset* asset = &self->assets[i];
        am_unload_single_asset(asset);
        am_remove_unordered_without_unloading(self, i);
        i--;
    }
}

Image am_get_image(AmAssetManager* self, const char* path) {
    AmAsset* asset = am_get_asset(self, ASSET_IMAGE, path);
    if (asset == NULL) {
        TraceLog(LOG_ERROR, "ASSET MANAGER: Couldn't load image \"%s\"", path);
        return (Image) {0};
    }

    return asset->as.image;
}

Texture am_get_texture(AmAssetManager* self, const char* path) {
    AmAsset* asset = am_get_asset(self, ASSET_TEXTURE, path);
    if (asset == NULL) {
        TraceLog(LOG_ERROR, "ASSET MANAGER: Couldn't load texture \"%s\"", path);
        return (Texture) {0};
    }

    return asset->as.texture;
}

Font am_get_font(AmAssetManager* self, const char* path) {
    AmAsset* asset = am_get_asset(self, ASSET_FONT, path);
    if (asset == NULL) {
        TraceLog(LOG_ERROR, "ASSET MANAGER: Couldn't load font \"%s\"", path);
        return (Font) {0};
    }

    return asset->as.font;
}

Shader am_get_shader(AmAssetManager* self, const char* vsPath, const char* fsPath) {
    AmAsset* asset = am_get_asset(self, ASSET_SHADER, vsPath, fsPath);
    if (asset == NULL) {
        TraceLog(LOG_ERROR, "ASSET MANAGER: Couldn't load shader \"%s\", \"%s\"", vsPath, fsPath);
        return (Shader) {0};
    }
    
    return asset->as.shader;
}

#endif // AMRL_IMPLEMENTATION
