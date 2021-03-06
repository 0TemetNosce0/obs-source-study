#include <windows.h>
#include <obs.h>
#include "cursor-capture.h"

static uint8_t *get_bitmap_data(HBITMAP hbmp, BITMAP *bmp)
{
    if (GetObject(hbmp, sizeof(*bmp), bmp) != 0) {
        uint8_t *output;
        unsigned int size =
                (bmp->bmHeight * bmp->bmWidth * bmp->bmBitsPixel) / 8;

        output = bmalloc(size);
        GetBitmapBits(hbmp, size, output);//该函数将指定位图的位拷贝到缓冲区,output指向接收位图位数据的缓冲区指针
        return output;
    }

    return NULL;
}

static inline uint8_t bit_to_alpha(uint8_t *data, long pixel, bool invert)
{
    uint8_t pix_byte = data[pixel / 8];
    bool alpha = (pix_byte >> (7 - pixel % 8) & 1) != 0;

    if (invert) {
        return alpha ? 0xFF : 0;
    } else {
        return alpha ? 0 : 0xFF;
    }
}

static inline bool bitmap_has_alpha(uint8_t *data, long num_pixels)
{
    for (long i = 0; i < num_pixels; i++) {
        if (data[i * 4 + 3] != 0) {
            return true;
        }
    }

    return false;
}

static inline void apply_mask(uint8_t *color, uint8_t *mask, long num_pixels)
{
    for (long i = 0; i < num_pixels; i++)
        color[i * 4 + 3] = bit_to_alpha(mask, i, false);
}

static inline uint8_t *copy_from_color(ICONINFO *ii, uint32_t *width,
                                       uint32_t *height)
{
    BITMAP bmp_color;
    BITMAP bmp_mask;
    uint8_t *color;
    uint8_t *mask;

    color = get_bitmap_data(ii->hbmColor, &bmp_color);
    if (!color) {
        return NULL;
    }

    if (bmp_color.bmBitsPixel < 32) {
        bfree(color);
        return NULL;
    }

    mask = get_bitmap_data(ii->hbmMask, &bmp_mask);
    if (mask) {
        long pixels = bmp_color.bmHeight * bmp_color.bmWidth;

        if (!bitmap_has_alpha(color, pixels))
            apply_mask(color, mask, pixels);

        bfree(mask);
    }

    *width = bmp_color.bmWidth;
    *height = bmp_color.bmHeight;
    return color;
}

static inline uint8_t *copy_from_mask(ICONINFO *ii, uint32_t *width,
                                      uint32_t *height)
{
    uint8_t *output;
    uint8_t *mask;
    long pixels;
    long bottom;
    BITMAP bmp;

    mask = get_bitmap_data(ii->hbmMask, &bmp);
    if (!mask) {
        return NULL;
    }

    bmp.bmHeight /= 2;

    pixels = bmp.bmHeight * bmp.bmWidth;
    output = bzalloc(pixels * 4);

    bottom = bmp.bmWidthBytes * bmp.bmHeight;

    for (long i = 0; i < pixels; i++) {
        uint8_t alpha = bit_to_alpha(mask, i, false);
        uint8_t color = bit_to_alpha(mask + bottom, i, true);

        if (!alpha) {
            output[i * 4 + 3] = color;
        } else {
            *(uint32_t*)&output[i * 4] = !!color ?
                        0xFFFFFFFF : 0xFF000000;
        }
    }

    bfree(mask);

    *width = bmp.bmWidth;
    *height = bmp.bmHeight;
    return output;
}

static inline uint8_t *cursor_capture_icon_bitmap(ICONINFO *ii,
                                                  uint32_t *width, uint32_t *height)
{
    uint8_t *output;

    output = copy_from_color(ii, width, height);
    if (!output)
        output = copy_from_mask(ii, width, height);

    return output;
}

static gs_texture_t *get_cached_texture(struct cursor_data *data,
                                        uint32_t cx, uint32_t cy)
{
    struct cached_cursor cc;

    for (size_t i = 0; i < data->cached_textures.num; i++) {
        struct cached_cursor *pcc = &data->cached_textures.array[i];

        if (pcc->cx == cx && pcc->cy == cy)
            return pcc->texture;
    }

    cc.texture = gs_texture_create(cx, cy, GS_BGRA, 1, NULL, GS_DYNAMIC);
    cc.cx = cx;
    cc.cy = cy;
    da_push_back(data->cached_textures, &cc);
    return cc.texture;
}
//捕获鼠标icon
static inline bool cursor_capture_icon(struct cursor_data *data, HICON icon)
{
    uint8_t *bitmap;
    uint32_t height;
    uint32_t width;
    ICONINFO ii;

    if (!icon) {
        return false;
    }
    if (!GetIconInfo(icon, &ii)) {
        return false;
    }

    bitmap = cursor_capture_icon_bitmap(&ii, &width, &height);//获取位图
    if (bitmap) {
        if (data->last_cx != width && data->last_cy != height) {
            data->texture = get_cached_texture(data, width, height);
            data->last_cx = width;
            data->last_cy = height;
        }
        gs_texture_set_image(data->texture, bitmap, width * 4, false);
        gs_texture_set_image(data->texture, bitmap, width * 4, false);
        bfree(bitmap);

        data->x_hotspot = ii.xHotspot;
        data->y_hotspot = ii.yHotspot;
    }

    DeleteObject(ii.hbmColor);//该函数删除一个逻辑笔、画笔、字体、位图、区域或者调色板，释放所有与该对象有关的系统资源，在对象被删除之后，指定的句柄也就失效了。
    DeleteObject(ii.hbmMask);
    return !!data->texture;
}
//鼠标捕获
void cursor_capture(struct cursor_data *data)
{
    CURSORINFO ci = {0};//该结构包含了全局光标信息
    HICON icon;

    ci.cbSize = sizeof(ci);

    if (!GetCursorInfo(&ci)) {//获取鼠标
        data->visible = false;
        return;
    }

    memcpy(&data->cursor_pos, &ci.ptScreenPos, sizeof(data->cursor_pos));

    if (data->current_cursor == ci.hCursor) {
        return;
    }

    icon = CopyIcon(ci.hCursor);//复制指定的图标
    data->visible = cursor_capture_icon(data, icon);
    data->current_cursor = ci.hCursor;
    if ((ci.flags & CURSOR_SHOWING) == 0)
        data->visible = false;
    DestroyIcon(icon);
}

/***************************
 * brief:鼠标绘制
 * input:
 * output:
 * return:
 **************************/
void cursor_draw(struct cursor_data *data, long x_offset, long y_offset,
                 float x_scale, float y_scale, long width, long height)
{
    long x = data->cursor_pos.x + x_offset;
    long y = data->cursor_pos.y + y_offset;
    long x_draw = x - data->x_hotspot;// 鼠标位置
    long y_draw = y - data->y_hotspot;

    if (x < 0 || x > width || y < 0 || y > height)
        return;

    if (data->visible && !!data->texture) {


                gs_blend_state_push();
                gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);
                gs_enable_color(true, true, true, false);

                gs_matrix_push();
                gs_matrix_scale3f(x_scale, y_scale, 1.0f);
                obs_source_draw(data->texture, x_draw, y_draw, 0, 0, false);
                //obs_source_draw(data->texture, 0, 0, 100, 500, false);
                gs_matrix_pop();


        ///////////

        ///////
//        if(GetAsyncKeyState(VK_LBUTTON)&&0x8000){//鼠标左键按下状态
//        //画圆该怎么画，画的位置怎么确定
////            gs_enable_color(true, true, true, true);
////            gs_blend_state_pop();
//
//
//            gs_effect_t    *solid = obs_get_base_effect(OBS_EFFECT_SOLID);//XXX.effect文件，获取gs_effect，图像效果//1
//            gs_eparam_t    *color = gs_effect_get_param_by_name(solid, "color"); //获取gs_effect_t的color
//            gs_technique_t *tech = gs_effect_get_technique(solid, "Solid");//3gs_technique_t
//
//            //4设置颜色
//            struct vec4 colorVal;
//            vec4_from_rgba(&colorVal, 0x222222ff);
//            gs_effect_set_vec4(color, &colorVal);//设置颜色
//
//            gs_technique_begin(tech);//5echnique_begin
//            gs_technique_begin_pass(tech, 0);//6echnique_begin_pass
//            gs_draw_sprite(0, 0, 1000, 1000);//绘制
//
////            gs_load_vertexbuffer(graphics->sprite_buffer);//7 获取顶点
////            gs_draw(GS_TRISTRIP, 0, 0);//8 绘制
//
//            gs_technique_end_pass(tech);//9technique_end_pass
//            gs_technique_end(tech);//10technique_end
//        }
    }
}

void cursor_data_free(struct cursor_data *data)
{
    for (size_t i = 0; i < data->cached_textures.num; i++) {
        struct cached_cursor *pcc = &data->cached_textures.array[i];
        gs_texture_destroy(pcc->texture);
    }
    da_free(data->cached_textures);
    memset(data, 0, sizeof(*data));
}
