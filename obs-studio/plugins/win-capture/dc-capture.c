#include "dc-capture.h"

#define WIN32_MEAN_AND_LEAN
#include <windows.h>

static inline void init_textures(struct dc_capture *capture)
{
    for (int i = 0; i < capture->num_textures; i++) {
        if (capture->compatibility)
            capture->textures[i] = gs_texture_create(
                        capture->width, capture->height,
                        GS_BGRA, 1, NULL, GS_DYNAMIC);
        else
            capture->textures[i] = gs_texture_create_gdi(
                        capture->width, capture->height);

        if (!capture->textures[i]) {
            blog(LOG_WARNING, "[dc_capture_init] Failed to "
                              "create textures");
            return;
        }
    }

    capture->valid = true;
}

void dc_capture_init(struct dc_capture *capture, int x, int y,
                     uint32_t width, uint32_t height, bool cursor,
                     bool compatibility)
{
    memset(capture, 0, sizeof(struct dc_capture));

    capture->x              = x;
    capture->y              = y;
    capture->width          = width;
    capture->height         = height;
    capture->capture_cursor = cursor;

    obs_enter_graphics();

    if (!gs_gdi_texture_available())
        compatibility = true;

    capture->compatibility = compatibility;
    capture->num_textures  = compatibility ? 1 : 2;

    init_textures(capture);

    obs_leave_graphics();

    if (!capture->valid)
        return;

    if (compatibility) {
        BITMAPINFO bi = {0};
        BITMAPINFOHEADER *bih = &bi.bmiHeader;
        bih->biSize     = sizeof(BITMAPINFOHEADER);
        bih->biBitCount = 32;
        bih->biWidth    = width;
        bih->biHeight   = height;
        bih->biPlanes   = 1;

        capture->hdc = CreateCompatibleDC(NULL);
        capture->bmp = CreateDIBSection(capture->hdc, &bi,
                                        DIB_RGB_COLORS, (void**)&capture->bits,
                                        NULL, 0);
        capture->old_bmp = SelectObject(capture->hdc, capture->bmp);
    }
}

void dc_capture_free(struct dc_capture *capture)
{
    if (capture->hdc) {
        SelectObject(capture->hdc, capture->old_bmp);
        DeleteDC(capture->hdc);
        DeleteObject(capture->bmp);
    }

    obs_enter_graphics();

    for (int i = 0; i < capture->num_textures; i++)
        gs_texture_destroy(capture->textures[i]);

    obs_leave_graphics();

    memset(capture, 0, sizeof(struct dc_capture));
}
//窗口鼠标绘制
static void draw_cursor(struct dc_capture *capture, HDC hdc, HWND window)
{
    HICON      icon;
    ICONINFO   ii;
    CURSORINFO *ci = &capture->ci;
    POINT      win_pos = {capture->x, capture->y};

    if (!(capture->ci.flags & CURSOR_SHOWING))
        return;

    icon = CopyIcon(capture->ci.hCursor);
    if (!icon)
        return;

    if (GetIconInfo(icon, &ii)) {
        POINT pos;

        if (window)
            ClientToScreen(window, &win_pos);//将鼠标指针位置转换为窗口坐标

        pos.x = ci->ptScreenPos.x - (int)ii.xHotspot - win_pos.x;
        pos.y = ci->ptScreenPos.y - (int)ii.yHotspot - win_pos.y;

        //if(GetAsyncKeyState(VK_LBUTTON)&&0x8000){//鼠标左键按下状态
        //    HPEN hPen=CreatePen(PS_SOLID,1,RGB(255,0,0));
        //    //将笔选入DC
        //    (HPEN)SelectObject(hdc,hPen);
        //    HBRUSH hBrush=CreateSolidBrush(RGB(122,122,122));
        //    //画矩形
        //    RECT rect;
        //            rect.bottom=pos.y+50;
        //            rect.left=pos.x+50;
        //            rect.right=pos.x;
        //            rect.top=pos.y;
        //            FillRect(hdc,&rect,hBrush);
        //    //画圆
        //    //Ellipse(hdc,pos.x-25,pos.y-25,(pos.x+25),(pos.y+25));
        //    DeleteObject(hBrush);
        //    DeleteObject(hPen);
        //}


        //HWND my = GetForegroundWindow();
        //RECT rect1;
        //GetWindowRect(my, &rect1);

        //HPEN hPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
        ////将笔选入DC
        //(HPEN)SelectObject(hdc, hPen);
        //HBRUSH hBrush = CreateSolidBrush(RGB(122, 122, 122));
        ////画矩形
        //RECT rect;
        //rect.bottom = rect1.bottom-250;
        //rect.left = rect1.left+180;
        //rect.right = rect1.right-180;
        //rect.top = rect1.top+70;
        //FillRect(hdc, &rect, hBrush);
        ////画圆
        ////Ellipse(hdc,pos.x-25,pos.y-25,(pos.x+25),(pos.y+25));
        //DeleteObject(hBrush);
        //DeleteObject(hPen);

        DrawIcon(hdc, pos.x, pos.y, icon);//绘制icon

        DeleteObject(ii.hbmColor);
        DeleteObject(ii.hbmMask);
    }

    DestroyIcon(icon);
}

static inline HDC dc_capture_get_dc(struct dc_capture *capture)
{
    if (!capture->valid)
        return NULL;

    if (capture->compatibility)
        return capture->hdc;
    else
        return gs_texture_get_dc(capture->textures[capture->cur_tex]);
}

static inline void dc_capture_release_dc(struct dc_capture *capture)
{
    if (capture->compatibility) {
        gs_texture_set_image(capture->textures[capture->cur_tex],
                capture->bits, capture->width*4, false);
    } else {
        gs_texture_release_dc(capture->textures[capture->cur_tex]);
    }
}

//桌面鼠标跟随
void desktop_capture_capture(struct dc_capture *capture)
{
    HDC hdc_target;
    HDC hdc;

    if (capture->capture_cursor) {
        memset(&capture->ci, 0, sizeof(CURSORINFO));
        capture->ci.cbSize = sizeof(CURSORINFO);
        capture->cursor_captured = GetCursorInfo(&capture->ci);
    }

    if (++capture->cur_tex == capture->num_textures)
        capture->cur_tex = 0;

    hdc = dc_capture_get_dc(capture);
    if (!hdc) {
        blog(LOG_WARNING, "[capture_screen] Failed to get "
                          "texture DC");
        return;
    }

    TCHAR strWndName[128] = { 0 };
    BOOL  bQQWnd = FALSE;
    BOOL  bDrawCursor = TRUE;
    if(0){//鼠标中间捕获
        hdc_target = GetDC(NULL);

        int xpos = capture->ci.ptScreenPos.x - capture->width / 2;
        int ypos = capture->ci.ptScreenPos.y - capture->height / 2;

        int nscreenwidth = GetDeviceCaps(hdc_target, HORZRES);
        int nscreenheight = GetDeviceCaps(hdc_target, VERTRES);

        if (xpos <= 0)
            xpos = 0;
        else if (nscreenwidth - capture->ci.ptScreenPos.x < capture->width / 2)
            xpos = nscreenwidth - capture->width;

        if (ypos <= 0)
            ypos = 0;
        else if (nscreenheight - capture->ci.ptScreenPos.y < capture->height / 2)
            ypos = nscreenheight - capture->height;

        if (hdc_target)
        {
            if (!BitBlt(hdc, 0, 0, capture->width, capture->height, hdc_target, xpos, ypos, SRCCOPY))
            {
                blog(LOG_WARNING, "[dc_capture_capture] Failed to get BitBlt");
            }
            if (capture->cursor_captured&&bDrawCursor)
                DrawIcon(hdc, capture->ci.ptScreenPos.x - xpos, capture->ci.ptScreenPos.y - ypos, capture->ci.hCursor);
        }
        else
        {
            blog(LOG_WARNING, "[dc_capture_capture] Failed to get GetDC");
        }
    }else{//鼠标边缘
        hdc_target = GetDC(NULL);

        int xpos = capture->ci.ptScreenPos.x;//当前鼠标屏幕位置
        int ypos = capture->ci.ptScreenPos.y;

        if (xpos <= capture->x){
            capture->x = xpos;
        }else if(xpos >= (capture->x+capture->width) ){
            capture->x = xpos-capture->width;
        }
        if (ypos <= capture->y){
            capture->y = ypos;
        }else if(ypos >= (capture->y+capture->height) ){
            capture->y = ypos-capture->height;
        }


        if (hdc_target)
        {
            if (!BitBlt(hdc, 0, 0, capture->width, capture->height, hdc_target, capture->x, capture->y, SRCCOPY))//7，8-屏幕左上角坐标。
            {
                blog(LOG_WARNING, "[dc_capture_capture] Failed to get BitBlt");
            }
            if (capture->cursor_captured&&bDrawCursor)
                //                DrawIcon(hdc, capture->ci.ptScreenPos.x - capture->x, capture->ci.ptScreenPos.y - capture->y, capture->ci.hCursor);
                draw_cursor(capture, hdc, GetDesktopWindow());
        }
        else
        {
            blog(LOG_WARNING, "[dc_capture_capture] Failed to get GetDC");
        }
    }

    ReleaseDC(NULL, hdc_target);

    dc_capture_release_dc(capture);

    capture->textures_written[capture->cur_tex] = true;
}
void dc_capture_capture(struct dc_capture *capture, HWND window)
{
    HDC hdc_target;
    HDC hdc;

    if (capture->capture_cursor) {
        memset(&capture->ci, 0, sizeof(CURSORINFO));
        capture->ci.cbSize = sizeof(CURSORINFO);
        capture->cursor_captured = GetCursorInfo(&capture->ci);
    }

    if (++capture->cur_tex == capture->num_textures)
        capture->cur_tex = 0;

    hdc = dc_capture_get_dc(capture);
    if (!hdc) {
        blog(LOG_WARNING, "[capture_screen] Failed to get "
                          "texture DC");
        return;
    }

    hdc_target = GetDC(window);

    BitBlt(hdc, 0, 0, capture->width, capture->height,
           hdc_target, capture->x, capture->y, SRCCOPY);

    ReleaseDC(NULL, hdc_target);

    if (capture->cursor_captured && !capture->cursor_hidden)
        draw_cursor(capture, hdc, window);

    dc_capture_release_dc(capture);

    capture->textures_written[capture->cur_tex] = true;
}

static void draw_texture(struct dc_capture *capture, int id,
                         gs_effect_t *effect)
{
    gs_texture_t   *texture = capture->textures[id];
    gs_technique_t *tech    = gs_effect_get_technique(effect, "Draw");
    gs_eparam_t    *image   = gs_effect_get_param_by_name(effect, "image");
    size_t      passes;

    gs_effect_set_texture(image, texture);

    passes = gs_technique_begin(tech);
    for (size_t i = 0; i < passes; i++) {
        if (gs_technique_begin_pass(tech, i)) {
            if (capture->compatibility)
                gs_draw_sprite(texture, GS_FLIP_V, 0, 0);
            else
                gs_draw_sprite(texture, 0, 0, 0);

            gs_technique_end_pass(tech);
        }
    }
    gs_technique_end(tech);
}

void dc_capture_render(struct dc_capture *capture, gs_effect_t *effect)
{
    int last_tex = (capture->cur_tex > 0) ?
                capture->cur_tex-1 : capture->num_textures-1;

    if (!capture->valid)
        return;

    if (capture->textures_written[last_tex])
        draw_texture(capture, last_tex, effect);
}
