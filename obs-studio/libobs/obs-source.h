/******************************************************************************
    Copyright (C) 2013-2014 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include "obs.h"

/**
 * @file
 * @brief header for modules implementing sources.
 *
 * Sources are modules that either feed data to libobs or modify it.
 */

#ifdef __cplusplus
extern "C" {
#endif

//obssource源类型
enum obs_source_type {
    OBS_SOURCE_TYPE_INPUT,//输入源
    OBS_SOURCE_TYPE_FILTER,//滤镜源
    OBS_SOURCE_TYPE_TRANSITION,//过渡源
    OBS_SOURCE_TYPE_SCENE,//场景源
};


/**
 * @name Source output flags
 *
 * These flags determine what type of data the source outputs and expects.
 * @{
 */

/**
 * Source has video.
 *
 * Unless SOURCE_ASYNC_VIDEO is specified, the source must include the
 * video_render callback in the source definition structure.
 */
#define OBS_SOURCE_VIDEO        (1<<0)

/**
 * Source has audio.
 *
 * Use the obs_source_output_audio function to pass raw audio data, which will
 * be automatically converted and uploaded.  If used with SOURCE_ASYNC_VIDEO,
 * audio will automatically be synced up to the video output.
 */
#define OBS_SOURCE_AUDIO        (1<<1)

/** Async video flag (use OBS_SOURCE_ASYNC_VIDEO) */
#define OBS_SOURCE_ASYNC        (1<<2)

/**
 * Source passes raw video data via RAM.
 *
 * Use the obs_source_output_video function to pass raw video data, which will
 * be automatically uploaded at the specified timestamp.
 *
 * If this flag is specified, it is not necessary to include the video_render
 * callback.  However, if you wish to use that function as well, you must call
 * obs_source_getframe to get the current frame data, and
 * obs_source_releaseframe to release the data when complete.
 */
#define OBS_SOURCE_ASYNC_VIDEO  (OBS_SOURCE_ASYNC | OBS_SOURCE_VIDEO)

/**
 * Source uses custom drawing, rather than a default effect.
 *
 * If this flag is specified, the video_render callback will pass a NULL
 * effect, and effect-based filters will not use direct rendering.
 */
#define OBS_SOURCE_CUSTOM_DRAW  (1<<3)

/**
 * Source supports interaction.
 *
 * When this is used, the source will receive interaction events
 * if they provide the necessary callbacks in the source definition structure.
 */
#define OBS_SOURCE_INTERACTION (1<<5)

/**
 * Source composites sub-sources
 *
 * When used specifies that the source composites one or more sub-sources.
 * Sources that render sub-sources must implement the audio_render callback
 * in order to perform custom mixing of sub-sources.
 *
 * This capability flag is always set for transitions.
 */
#define OBS_SOURCE_COMPOSITE (1<<6)

/**
 * Source should not be fully duplicated
 *
 * When this is used, specifies that the source should not be fully duplicated,
 * and should prefer to duplicate via holding references rather than full
 * duplication.
 */
#define OBS_SOURCE_DO_NOT_DUPLICATE (1<<7)

/**
 * Source is deprecated and should not be used
 */
#define OBS_SOURCE_DEPRECATED (1<<8)

/**
 * Source cannot have its audio monitored
 *
 * Specifies that this source may cause a feedback loop if audio is monitored
 * with a device selected as desktop audio.
 *
 * This is used primarily with desktop audio capture sources.
 */
#define OBS_SOURCE_DO_NOT_SELF_MONITOR (1<<9)

/** @} */

typedef void (*obs_source_enum_proc_t)(obs_source_t *parent,
		obs_source_t *child, void *param);

struct obs_source_audio_mix {
	struct audio_output_data output[MAX_AUDIO_MIXES];
};

/**
 * Source definition structure
 */
struct obs_source_info {
	/* ----------------------------------------------------------------- */
	/* Required implementation*/

	/** Unique string identifier for the source */
    const char *id;//id标识，添加一个源的时候需要这个id来区分

	/**
	 * Type of source.
	 *
	 * OBS_SOURCE_TYPE_INPUT for input sources,
	 * OBS_SOURCE_TYPE_FILTER for filter sources, and
	 * OBS_SOURCE_TYPE_TRANSITION for transition sources.
	 */
    enum obs_source_type type;//源类型，输入滤镜过渡场景

	/** Source output flags */
    uint32_t output_flags;//输出标志(位标志):egOBS_SOURCE_AUDIO源是否有音频，OBS_SOURCE_VIDEO源是否有视频

	/**
	 * Get the translated name of the source type
	 *
	 * @param  type_data  The type_data variable of this structure
	 * @return               The translated name of the source type
	 */
    const char *(*get_name)(void *type_data);//get_name获取源的名字，比如，添加一个输入源时右键菜单现实的输入源名字

	/**
	 * Creates the source data for the source
	 *
     * @param  settings  Settings to initialize the source with 设置
     * @param  source    Source that this data is associated with 与此数据相关的源
     * @return           The data associated with this source 与此源相关的数据
	 */
    void *(*create)(obs_data_t *settings, obs_source_t *source);//更具obs_source创建源data，返回与此源相关的源data

	/**
	 * Destroys the private data for the source
	 *
	 * Async sources must not call obs_source_output_video after returning
	 * from destroy
	 */
	void (*destroy)(void *data);

	/** Returns the width of the source.  Required if this is an input
	 * source and has non-async video */
	uint32_t (*get_width)(void *data);

	/** Returns the height of the source.  Required if this is an input
	 * source and has non-async video */
	uint32_t (*get_height)(void *data);

	/* ----------------------------------------------------------------- */
	/* Optional implementation */

	/**
	 * Gets the default settings for this source
	 *
	 * @param[out]  settings  Data to assign default settings to
	 */
	void (*get_defaults)(obs_data_t *settings);

	/**
	 * Gets the property information of this source
     * 获取源属性
     * @return         The properties data
	 */
    obs_properties_t *(*get_properties)(void *data);

	/**
	 * Updates the settings for this source
     * 更新源的设置settings
	 * @param data      Source data
	 * @param settings  New settings for this source
	 */
	void (*update)(void *data, obs_data_t *settings);

	/** Called when the source has been activated in the main view */
	void (*activate)(void *data);

	/**
	 * Called when the source has been deactivated from the main view
	 * (no longer being played/displayed)
	 */
	void (*deactivate)(void *data);

	/** Called when the source is visible */
	void (*show)(void *data);

	/** Called when the source is no longer visible */
	void (*hide)(void *data);

	/**
	 * Called each video frame with the time elapsed
     * 每一个视频帧随时间流逝
     * @param  data     Source data //源data
     * @param  seconds  Seconds elapsed since the last frame 最后一帧几秒钟消失
	 */
	void (*video_tick)(void *data, float seconds);

	/**
	 * Called when rendering the source with the graphics subsystem.
	 *
	 * If this is an input/transition source, this is called to draw the
	 * source texture with the graphics subsystem using the specified
	 * effect.
	 *
	 * If this is a filter source, it wraps source draw calls (for
	 * example applying a custom effect with custom parameters to a
	 * source).  In this case, it's highly recommended to use the
	 * obs_source_process_filter function to automatically handle
	 * effect-based filter processing.  However, you can implement custom
	 * draw handling as desired as well.
	 *
	 * If the source output flags do not include SOURCE_CUSTOM_DRAW, all
	 * a source needs to do is set the "image" parameter of the effect to
	 * the desired texture, and then draw.  If the output flags include
	 * SOURCE_COLOR_MATRIX, you may optionally set the "color_matrix"
	 * parameter of the effect to a custom 4x4 conversion matrix (by
	 * default it will be set to an YUV->RGB conversion matrix)
	 *
	 * @param data    Source data
	 * @param effect  Effect to be used with this source.  If the source
	 *                output flags include SOURCE_CUSTOM_DRAW, this will
	 *                be NULL, and the source is expected to process with
	 *                an effect manually.
	 */
//渲染源
    void (*video_render)(void *data, gs_effect_t *effect);

	/**
	 * Called to filter raw async video data.
	 *
	 * @note          This function is only used with filter sources.
	 *
	 * @param  data   Filter data
	 * @param  frame  Video frame to filter
	 * @return        New video frame data.  This can defer video data to
	 *                be drawn later if time is needed for processing
	 */
	struct obs_source_frame *(*filter_video)(void *data,
			struct obs_source_frame *frame);

	/**
	 * Called to filter raw audio data.
	 *
	 * @note          This function is only used with filter sources.
	 *
	 * @param  data   Filter data
	 * @param  audio  Audio data to filter.
	 * @return        Modified or new audio data.  You can directly modify
	 *                the data passed and return it, or you can defer audio
	 *                data for later if time is needed for processing.  If
	 *                you are returning new data, that data must exist
	 *                until the next call to the filter_audio callback or
	 *                until the filter is removed/destroyed.
	 */
	struct obs_audio_data *(*filter_audio)(void *data,
			struct obs_audio_data *audio);

	/**
	 * Called to enumerate all active sources being used within this
	 * source.  If the source has children that render audio/video it must
	 * implement this callback.
	 *
	 * @param  data           Filter data
	 * @param  enum_callback  Enumeration callback
	 * @param  param          User data to pass to callback
	 */
	void (*enum_active_sources)(void *data,
			obs_source_enum_proc_t enum_callback,
			void *param);

	/**
	 * Called when saving a source.  This is a separate function because
	 * sometimes a source needs to know when it is being saved so it
	 * doesn't always have to update the current settings until a certain
	 * point.
	 *
	 * @param  data      Source data
	 * @param  settings  Settings
	 */
	void (*save)(void *data, obs_data_t *settings);

	/**
	 * Called when loading a source from saved data.  This should be called
	 * after all the loading sources have actually been created because
	 * sometimes there are sources that depend on each other.
	 *
	 * @param  data      Source data
	 * @param  settings  Settings
	 */
	void (*load)(void *data, obs_data_t *settings);

	/**
	 * Called when interacting with a source and a mouse-down or mouse-up
	 * occurs.
	 *
	 * @param data         Source data
	 * @param event        Mouse event properties
	 * @param type         Mouse button pushed
	 * @param mouse_up     Mouse event type (true if mouse-up)
	 * @param click_count  Mouse click count (1 for single click, etc.)
	 */
	void (*mouse_click)(void *data,
			const struct obs_mouse_event *event,
			int32_t type, bool mouse_up, uint32_t click_count);
	/**
	 * Called when interacting with a source and a mouse-move occurs.
	 *
	 * @param data         Source data
	 * @param event        Mouse event properties
	 * @param mouse_leave  Mouse leave state (true if mouse left source)
	 */
	void (*mouse_move)(void *data,
			const struct obs_mouse_event *event, bool mouse_leave);

	/**
	 * Called when interacting with a source and a mouse-wheel occurs.
	 *
	 * @param data         Source data
	 * @param event        Mouse event properties
	 * @param x_delta      Movement delta in the horizontal direction
	 * @param y_delta      Movement delta in the vertical direction
	 */
	void (*mouse_wheel)(void *data,
			const struct obs_mouse_event *event, int x_delta,
			int y_delta);
	/**
	 * Called when interacting with a source and gain focus/lost focus event
	 * occurs.
	 *
	 * @param data         Source data
	 * @param focus        Focus state (true if focus gained)
	 */
	void (*focus)(void *data, bool focus);

	/**
	 * Called when interacting with a source and a key-up or key-down
	 * occurs.
	 *
	 * @param data         Source data
	 * @param event        Key event properties
	 * @param focus        Key event type (true if mouse-up)
	 */
	void (*key_click)(void *data, const struct obs_key_event *event,
			bool key_up);

	/**
	 * Called when the filter is removed from a source
	 *
	 * @param  data    Filter data
	 * @param  source  Source that the filter being removed from
	 */
	void (*filter_remove)(void *data, obs_source_t *source);

	/**
	 * Private data associated with this entry
	 */
	void *type_data;

	/**
	 * If defined, called to free private data on shutdown
	 */
	void (*free_type_data)(void *type_data);

	bool (*audio_render)(void *data, uint64_t *ts_out,
			struct obs_source_audio_mix *audio_output,
			uint32_t mixers, size_t channels, size_t sample_rate);

	/**
	 * Called to enumerate all active and inactive sources being used
	 * within this source.  If this callback isn't implemented,
	 * enum_active_sources will be called instead.
	 *
	 * This is typically used if a source can have inactive child sources.
	 *
	 * @param  data           Filter data
	 * @param  enum_callback  Enumeration callback
	 * @param  param          User data to pass to callback
	 */
	void (*enum_all_sources)(void *data,
			obs_source_enum_proc_t enum_callback,
			void *param);
};

EXPORT void obs_register_source_s(const struct obs_source_info *info,
		size_t size);

/**
 * Registers a source definition to the current obs context.  This should be
 * used in obs_module_load.
 *
 * @param  info  Pointer to the source definition structure
 */
#define obs_register_source(info) \
	obs_register_source_s(info, sizeof(struct obs_source_info))

#ifdef __cplusplus
}
#endif
