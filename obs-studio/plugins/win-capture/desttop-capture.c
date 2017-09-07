#include <util/dstr.h>
#include "dc-capture.h"
//桌面鼠标跟随
#define TEXT_DESKTOP_NAME obs_module_text("DesktopCapture")
#define TEXT_CAPTURE_CURSOR  obs_module_text("CaptureCursor")
#define TEXT_MOVEMODE  obs_module_text("moveMode__")
#define TEXT_COMPATIBILITY   obs_module_text("Compatibility")
#define TEXT_DESKTOP         obs_module_text("Desktop")
#define TEXT_PRIMARY_MONITOR obs_module_text("PrimaryMonitor")

struct desktop_capture {
	obs_source_t      *source;

	int               desttop;
	bool              capture_cursor;
    bool              compatibility;//多设配器的兼�?
    bool              moveMode;//模式

    struct dc_capture data;//dc捕获结构�?
};

struct desktop_info {
	int               cur_id;
	int               desired_id;
	int               id;
	RECT              rect;
};
/* ------------------------------------------------------------------------- */

static inline void do_log(int level, const char *msg, ...)
{
	va_list args;
	struct dstr str = {0};

	va_start(args, msg);

	dstr_copy(&str, "[GDI desktop capture]: ");
	dstr_vcatf(&str, msg, args);
	blog(level, "%s", str.array);
	dstr_free(&str);

	va_end(args);
}

static BOOL CALLBACK enum_monitor(HMONITOR handle, HDC hdc, LPRECT rect,
		LPARAM param)
{
	struct desktop_info *monitor = (struct desktop_info *)param;

	if (monitor->cur_id == 0 || monitor->desired_id == monitor->cur_id) {
		monitor->rect = *rect;
		monitor->id   = monitor->cur_id;
	}

	UNUSED_PARAMETER(hdc);
	UNUSED_PARAMETER(handle);
	return (monitor->desired_id > monitor->cur_id++);
}

static void update_monitor(struct desktop_capture *capture,	obs_data_t *settings)
{
    struct desktop_info monitor = {0};
    uint32_t width, height;

    monitor.desired_id = (int)obs_data_get_int(settings, "desktop");
    EnumDisplayMonitors(NULL, NULL, enum_monitor, (LPARAM)&monitor);

    capture->desttop = monitor.id;
    int xPos = (int)obs_data_get_int(obs_source_get_settings(capture->source), "xPos");
    int yPos = (int)obs_data_get_int(obs_source_get_settings(capture->source), "yPos");
    width = (int)obs_data_get_int(settings, "Width");
    height = (int)obs_data_get_int(settings, "Height");


    dc_capture_init(&capture->data,xPos, yPos,
            width, height, capture->capture_cursor,
            capture->compatibility);
}

static inline void update_settings(struct desktop_capture *capture,	obs_data_t *settings)
{
	capture->desttop        = (int)obs_data_get_int(settings, "desktop");
	capture->capture_cursor = obs_data_get_bool(settings, "capture_cursor");
	capture->compatibility  = obs_data_get_bool(settings, "compatibility");
    capture->moveMode  = obs_data_get_bool(settings, "moveMode");//
	dc_capture_free(&capture->data);
	update_monitor(capture, settings);
}

/* ------------------------------------------------------------------------- */

static const char *desktop_capture_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return TEXT_DESKTOP_NAME;
}

static void desktop_capture_destroy(void *data)
{
	struct desktop_capture *capture = data;

	obs_enter_graphics();
	dc_capture_free(&capture->data);
	obs_leave_graphics();

	bfree(capture);
}

static void desktop_capture_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "desktop", 0);
	obs_data_set_default_int(settings, "Width", 800);
	obs_data_set_default_int(settings, "Height", 600);
	obs_data_set_default_int(settings, "xPos", GetSystemMetrics(SM_CXSCREEN) / 2 - 300);
	obs_data_set_default_int(settings, "yPos", GetSystemMetrics(SM_CYSCREEN) / 2 - 300);
	obs_data_set_default_bool(settings, "capture_cursor", true);
	obs_data_set_default_bool(settings, "compatibility", false);
    obs_data_set_default_bool(settings, "moveMode",true);//设置默认值
}

static void desktop_capture_update(void *data, obs_data_t *settings)
{
	struct desttop_capture *mc = data;
	update_settings(mc, settings);
}
static void *desktop_capture_create(obs_data_t *settings, obs_source_t *source)
{
	struct desktop_capture *capture;

	capture = bzalloc(sizeof(struct desktop_capture));
	capture->source = source;
	update_settings(capture, settings);

	return capture;
}

extern void desktop_capture_capture(struct dc_capture *capture,bool b);

static void desktop_capture_tick(void *data, float seconds)
{
	struct desktop_capture *capture = data;

	if (!obs_source_showing(capture->source))
		return;

	obs_enter_graphics();
    desktop_capture_capture(&capture->data,capture->moveMode);
	obs_leave_graphics();

	UNUSED_PARAMETER(seconds);
}

static void desktop_capture_render(void *data, gs_effect_t *effect)
{
	struct desktop_capture *capture = data;
	
	dc_capture_render(&capture->data, obs_get_base_effect(OBS_EFFECT_OPAQUE));

	UNUSED_PARAMETER(effect);
}

static uint32_t desktop_capture_width(void *data)
{
	struct desktop_capture *capture = data;
	return capture->data.width;
}

static uint32_t desktop_capture_height(void *data)
{
	struct desktop_capture *capture = data;
	return capture->data.height;
}

static BOOL CALLBACK enum_monitor_props(HMONITOR handle, HDC hdc, LPRECT rect,
	LPARAM param)
{
	UNUSED_PARAMETER(hdc);
	UNUSED_PARAMETER(rect);

	obs_property_t *monitor_list = (obs_property_t*)param;
	MONITORINFO mi;
	size_t monitor_id = 0;
	struct dstr monitor_desc = { 0 };
	struct dstr resolution = { 0 };
	struct dstr format_string = { 0 };
	

	monitor_id = obs_property_list_item_count(monitor_list);

	mi.cbSize = sizeof(mi);
	GetMonitorInfo(handle, &mi);

	dstr_catf(&resolution,
		"%dx%d @ %d,%d",
		mi.rcMonitor.right - mi.rcMonitor.left,
		mi.rcMonitor.bottom - mi.rcMonitor.top,
		mi.rcMonitor.left,
		mi.rcMonitor.top);

	dstr_copy(&format_string, "%s %d: %s");
	if (mi.dwFlags == MONITORINFOF_PRIMARY) {
		dstr_catf(&format_string, " (%s)", TEXT_PRIMARY_MONITOR);
	}

	dstr_catf(&monitor_desc,
		format_string.array,
		TEXT_DESKTOP,
		monitor_id,
		resolution.array);

	obs_property_list_add_int(monitor_list,
		monitor_desc.array, (int)monitor_id);

	dstr_free(&monitor_desc);
	dstr_free(&resolution);
	dstr_free(&format_string);

	return TRUE;
}

static obs_properties_t *desktop_capture_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	obs_property_t *monitors = obs_properties_add_list(props,
		"desktop", TEXT_DESKTOP,
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_properties_add_bool(props, "compatibility", TEXT_COMPATIBILITY);
	obs_properties_add_bool(props, "capture_cursor", TEXT_CAPTURE_CURSOR);
    obs_properties_add_bool(props, "moveMode", TEXT_MOVEMODE);//添加窗口显示出来的属性
	EnumDisplayMonitors(NULL, NULL, enum_monitor_props, (LPARAM)monitors);

	return props;
}

struct obs_source_info desttop_section_info = {
	.id             = "desktop-section",
	.type           = OBS_SOURCE_TYPE_INPUT,
	.output_flags   = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW,
	.get_name       = desktop_capture_getname,
	.create         = desktop_capture_create,
	.destroy        = desktop_capture_destroy,
	.video_render   = desktop_capture_render,
	.video_tick     = desktop_capture_tick,
	.update         = desktop_capture_update,
	.get_width      = desktop_capture_width,
	.get_height     = desktop_capture_height,
	.get_defaults   = desktop_capture_defaults,
	.get_properties = desktop_capture_properties
};

