/*
 * Copyright (c) 2013-2014 Hugh Bailey <obs.jim@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "../util/darray.h"
#include "../util/threading.h"

#include "decl.h"
#include "signal.h"

//信号 回调
struct signal_callback {
    signal_callback_t callback;//回调函数
	void              *data;
	bool              remove;
};

//信号info
struct signal_info {
	struct decl_info               func;
    DARRAY(struct signal_callback) callbacks;//回调结构体数组
	pthread_mutex_t                mutex;
	bool                           signalling;

    struct signal_info             *next;//下一个信号info
};

//信号info创建
static inline struct signal_info *signal_info_create(struct decl_info *info)
{
	pthread_mutexattr_t attr;
	struct signal_info *si;

	if (pthread_mutexattr_init(&attr) != 0)
		return NULL;
	if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0)
		return NULL;

	si = bmalloc(sizeof(struct signal_info));

	si->func       = *info;
	si->next       = NULL;
	si->signalling = false;
	da_init(si->callbacks);

	if (pthread_mutex_init(&si->mutex, &attr) != 0) {
		blog(LOG_ERROR, "Could not create signal");

		decl_info_free(&si->func);
		bfree(si);
		return NULL;
	}

	return si;
}

//信号info销毁
static inline void signal_info_destroy(struct signal_info *si)
{
	if (si) {
		pthread_mutex_destroy(&si->mutex);
		decl_info_free(&si->func);
		da_free(si->callbacks);
		bfree(si);
	}
}

//获取信号回调函数callback在signal_info的idx
static inline size_t signal_get_callback_idx(struct signal_info *si,
		signal_callback_t callback, void *data)
{
	for (size_t i = 0; i < si->callbacks.num; i++) {
		struct signal_callback *sc = si->callbacks.array+i;

		if (sc->callback == callback && sc->data == data)
			return i;
	}

	return DARRAY_INVALID;
}

//信号
struct signal_handler {
	struct signal_info *first;
	pthread_mutex_t    mutex;
};
//获取信号info
static struct signal_info *getsignal(signal_handler_t *handler,
		const char *name, struct signal_info **p_last)
{
	struct signal_info *signal, *last= NULL;

    signal = handler->first;//signal_handler的first指向signal_info
    while (signal != NULL) {//name比较
        if (strcmp(signal->func.name, name) == 0)//signal_info的func是一个decl_info含有信号名字
			break;

		last = signal;
		signal = signal->next;
	}

	if (p_last)
		*p_last = last;
	return signal;
}

/* ------------------------------------------------------------------------- */

//信号handler创建
signal_handler_t *signal_handler_create(void)
{
	struct signal_handler *handler = bmalloc(sizeof(struct signal_handler));
	handler->first = NULL;

	if (pthread_mutex_init(&handler->mutex, NULL) != 0) {
		blog(LOG_ERROR, "Couldn't create signal handler!");
		bfree(handler);
		return NULL;
	}

	return handler;
}
//信号handler销毁
void signal_handler_destroy(signal_handler_t *handler)
{
	if (handler) {
		struct signal_info *sig = handler->first;
		while (sig != NULL) {
			struct signal_info *next = sig->next;
			signal_info_destroy(sig);
			sig = next;
		}

		pthread_mutex_destroy(&handler->mutex);
		bfree(handler);
	}
}

////信号handler添加
bool signal_handler_add(signal_handler_t *handler, const char *signal_decl)
{
	struct decl_info func = {0};
	struct signal_info *sig, *last;
	bool success = true;

	if (!parse_decl_string(&func, signal_decl)) {
		blog(LOG_ERROR, "Signal declaration invalid: %s", signal_decl);
		return false;
	}

	pthread_mutex_lock(&handler->mutex);

	sig = getsignal(handler, func.name, &last);
	if (sig) {
		blog(LOG_WARNING, "Signal declaration '%s' exists", func.name);
		decl_info_free(&func);
		success = false;
	} else {
		sig = signal_info_create(&func);
		if (!last)
			handler->first = sig;
		else
			last->next = sig;
	}

	pthread_mutex_unlock(&handler->mutex);

	return success;
}

//信号handler链接(检查信号是否有,回调函数添加到signal_callback)
void signal_handler_connect(signal_handler_t *handler, const char *signal,
		signal_callback_t callback, void *data)
{
	struct signal_info *sig, *last;
	struct signal_callback cb_data = {callback, data, false};
	size_t idx;

	if (!handler)
		return;

	pthread_mutex_lock(&handler->mutex);
    sig = getsignal(handler, signal, &last);//获取信号(handler是否含有信号signal)
	pthread_mutex_unlock(&handler->mutex);

	if (!sig) {
		blog(LOG_WARNING, "signal_handler_connect: "
		                  "signal '%s' not found", signal);
		return;
	}

	/* -------------- */

	pthread_mutex_lock(&sig->mutex);

	idx = signal_get_callback_idx(sig, callback, data);
    if (idx == DARRAY_INVALID)//如果signal_callback没找到对应回调函数callback则添加
        da_push_back(sig->callbacks, &cb_data);//添加sig->callbacks到signal_callback
	
	pthread_mutex_unlock(&sig->mutex);
}

static inline struct signal_info *getsignal_locked(signal_handler_t *handler,
		const char *name)
{
	struct signal_info *sig;

	if (!handler)
		return NULL;

	pthread_mutex_lock(&handler->mutex);
	sig = getsignal(handler, name, NULL);
	pthread_mutex_unlock(&handler->mutex);

	return sig;
}

//断开链接
void signal_handler_disconnect(signal_handler_t *handler, const char *signal,
		signal_callback_t callback, void *data)
{
	struct signal_info *sig = getsignal_locked(handler, signal);
	size_t idx;

	if (!sig)
		return;

	pthread_mutex_lock(&sig->mutex);

	idx = signal_get_callback_idx(sig, callback, data);
	if (idx != DARRAY_INVALID) {
		if (sig->signalling)
			sig->callbacks.array[idx].remove = true;
		else
			da_erase(sig->callbacks, idx);
	}
	
	pthread_mutex_unlock(&sig->mutex);
}
//信号处理,会调用回调函数
void signal_handler_signal(signal_handler_t *handler, const char *signal,
		calldata_t *params)
{
	struct signal_info *sig = getsignal_locked(handler, signal);

	if (!sig)
		return;

	pthread_mutex_lock(&sig->mutex);
	sig->signalling = true;
//signal_info的callbacks指向signal_callback
    for (size_t i = 0; i < sig->callbacks.num; i++) {//回调函数signal_callback遍历
		struct signal_callback *cb = sig->callbacks.array+i;
		if (!cb->remove)
            cb->callback(cb->data, params);// 调用回调
	}

	for (size_t i = sig->callbacks.num; i > 0; i--) {
		struct signal_callback *cb = sig->callbacks.array+i-1;
		if (cb->remove)
			da_erase(sig->callbacks, i-1);
	}

	sig->signalling = false;
	pthread_mutex_unlock(&sig->mutex);
}
