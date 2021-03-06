#include <obs-module.h>
//色源
struct color_source {
    uint32_t color;//一些属性，颜色

	uint32_t width;
	uint32_t height;

    obs_source_t *src;//obs_source
};
//源的名字，比如在来源右键添加源的时候显示出来源的名字
static const char *color_source_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
    return obs_module_text("ColorSource");
}
//更新,settings的数据更新到data
static void color_source_update(void *data, obs_data_t *settings)
{
	struct color_source *context = data;
	uint32_t color = (uint32_t)obs_data_get_int(settings, "color");
	uint32_t width = (uint32_t)obs_data_get_int(settings, "width");
	uint32_t height = (uint32_t)obs_data_get_int(settings, "height");

	context->color = color;
	context->width = width;
	context->height = height;
}

//创建一个color_source结构体，里面context指向obs_source
static void *color_source_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(source);

	struct color_source *context = bzalloc(sizeof(struct color_source));
	context->src = source;

    color_source_update(context, settings);//color_source结构体的成员变量更新，通过settings里面的值。

	return context;
}

static void color_source_destroy(void *data)
{
	bfree(data);
}

//
static obs_properties_t *color_source_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

    obs_properties_t *props = obs_properties_create();//创建obs_properties结构体

    obs_properties_add_color(props, "color",
        obs_module_text("ColorSource.Color"));//添加color属性到obs_properties结构体//obs_module_text("ColorSource.Color")就只是一个字符串，跟翻译相关吧

	obs_properties_add_int(props, "width",
		obs_module_text("ColorSource.Width"), 0, 4096, 1);

	obs_properties_add_int(props, "height",
		obs_module_text("ColorSource.Height"), 0, 4096, 1);

	return props;
}
//渲染绘制
static void color_source_render(void *data, gs_effect_t *effect)
{
    UNUSED_PARAMETER(effect);

    struct color_source *context = data;

    gs_effect_t    *solid = obs_get_base_effect(OBS_EFFECT_SOLID);//获取gs_effect，图像效果
    gs_eparam_t    *color = gs_effect_get_param_by_name(solid, "color");
    gs_technique_t *tech  = gs_effect_get_technique(solid, "Solid");

    struct vec4 colorVal;
    vec4_from_rgba(&colorVal, context->color);
    gs_effect_set_vec4(color, &colorVal);//设置颜色

    gs_technique_begin(tech);
    gs_technique_begin_pass(tech, 0);

    gs_draw_sprite(0, 0, context->width, context->height);//绘制

    gs_technique_end_pass(tech);
    gs_technique_end(tech);
}

static uint32_t color_source_getwidth(void *data)
{
	struct color_source *context = data;
	return context->width;
}

static uint32_t color_source_getheight(void *data)
{
	struct color_source *context = data;
	return context->height;
}
//设置默认
static void color_source_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "color", 0xFFFFFFFF);
	obs_data_set_default_int(settings, "width", 400);
	obs_data_set_default_int(settings, "height", 400);
}
//通过
struct obs_source_info color_source_info = {
    .id             = "color_source",//创建源的时候是通过id来创建是哪个源
    .type           = OBS_SOURCE_TYPE_INPUT,//源类型:场景,来源,过渡,过滤
    .output_flags   = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW,//输出标志:OBS_SOURCE_VIDEO源有视频,没有OBS_SOURCE_AUDIO说明没有音频
    .create         = color_source_create,//创建
    .destroy        = color_source_destroy,//销毁
    .update         = color_source_update,//更新
	.get_name       = color_source_get_name,
	.get_defaults   = color_source_defaults,
	.get_width      = color_source_getwidth,
	.get_height     = color_source_getheight,
	.video_render   = color_source_render,
    .get_properties = color_source_properties//属性
};
