#ifndef AMRL_H_
#define AMRL_H_
#include <stddef.h>

#include <raylib.h>

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

    AmAssetType type;
    AmAssetAs as;
}AmAsset;

typedef struct {
    AmAsset* assets;
    size_t count;
    size_t capactiy;
}AmAssetManager;

void am_unload_all(AmAssetManager* self);

Image am_get_image(AmAssetManager* self, const char* path);
Texture am_get_texture(AmAssetManager* self, const char* path);
Font am_get_font(AmAssetManager* self, const char* path);
Shader am_get_shader(AmAssetManager* self, const char* vsPath, const char* fsPath);

#endif // AMRL_H_

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

AmAsset* am_load_asset(AmAssetManager* self, AmAssetType type, ...) {
    va_list args;

    va_start(args, type);
    const char* path = va_arg(args, const char*);

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
            const char* fs_path = va_arg(args, const char*);
            asset.path2 = fs_path;
            asset.as.shader = LoadShader(path, fs_path);
            valid = IsShaderValid(asset.as.shader);
            break;
        }break;
    }

    va_end(args);
    
    if (valid) {
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

    if (type == ASSET_SHADER) {
        const char* path2 = va_arg(args, const char*);
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