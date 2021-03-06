/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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
#include "obs-internal.h"
#include "graphics/matrix4.h"

/* how obs scene! */

struct item_action {
    bool visible;
    uint64_t timestamp;//时间戳
};
//场景item，场景item有两个指针一个指向所属场景，一个指向obs_source输入源
//场景item,描述的是一个场景下的一个输入源。
struct obs_scene_item {
    volatile long         ref;
    volatile bool         removed;

    int64_t               id;

    struct obs_scene      *parent;//场景item所属的场景
    struct obs_source     *source;//场景item的输入源
    volatile long         active_refs;
    volatile long         defer_update;
    bool                  user_visible;
    bool                  visible;//是否可见
    bool                  selected;//是否选中了
    bool                  locked;//新增，场景item是否锁定，锁定就不能移动缩放等

    gs_texrender_t        *item_render;
    struct obs_sceneitem_crop crop;//裁切

    struct vec2           pos;//item位置
    struct vec2           scale;//缩放
    float                 rot;
    uint32_t              align;

    /* last width/height of the source, this is used to check whether
     * the transform needs updating */
    uint32_t              last_width;
    uint32_t              last_height;

    struct vec2           output_scale;
    enum obs_scale_type   scale_filter;//缩放类型（在滤镜中的缩放比例）

    struct matrix4        box_transform;//4X4矩阵
    struct matrix4        draw_transform;

    enum obs_bounds_type  bounds_type;//边框类型
    uint32_t              bounds_align;
    struct vec2           bounds;

    obs_hotkey_pair_id    toggle_visibility;

    pthread_mutex_t       actions_mutex;
    DARRAY(struct item_action) audio_actions;//源的音频

    /* would do **prev_next, but not really great for reordering */
    struct obs_scene_item *prev;//上一个场景item
    struct obs_scene_item *next;//下一个场景item
};
//场景
struct obs_scene {
    struct obs_source     *source;//源

    int64_t               id_counter;

    pthread_mutex_t       video_mutex;
    pthread_mutex_t       audio_mutex;
    struct obs_scene_item *first_item;//场景的第一item
};
