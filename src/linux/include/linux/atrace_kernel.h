/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _ATRACE_H
#define _ATRACE_H
#ifdef ATRACE_KERNEL_DEBUG
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/module.h>

#define atrace_init(format, ...)
/**
 * Trace the beginning of a context.  name is used to identify the context.
 * This is often used to time function execution.
 */
void atrace_begin(const char *name);

/**
 * Trace the end of a context.
 * This should match up (and occur after) a corresponding ATRACE_BEGIN.
 */
void atrace_end(const char *name);

/**
 * Trace the beginning of an asynchronous event. Unlike ATRACE_BEGIN/ATRACE_END
 * contexts, asynchronous events do not need to be nested. The name describes
 * the event, and the cookie provides a unique identifier for distinguishing
 * simultaneous events. The name and cookie used to begin an event must be
 * used to end it.
 */
void atrace_async_begin(const char *name, int32_t cookie);

/**
 * Trace the end of an asynchronous event.
 * This should have a corresponding ATRACE_ASYNC_BEGIN.
 */
void atrace_async_end(const char *name, int32_t cookie);

/**
 * Traces an integer counter value.  name is used to identify the counter.
 * This can be used to track how a value changes over time.
 */
void atrace_int(const char *name, int32_t value);
/**
 * Traces a 64-bit integer counter value.  name is used to identify the
 * counter. This can be used to track how a value changes over time.
 */
void atrace_int64(const char *name, int64_t value);
#else /* ATRACE_KERNEL_DEBUG */
#define atrace_init(format, ...)
#define atrace_begin(format, ...)
#define atrace_end(format, ...)
#define atrace_async_begin(format, ...)
#define atrace_async_end(format, ...)
#define atrace_int(format, ...)
#define atrace_int64(format, ...)
#endif /* ATRACE_KERNEL_DEBUG */

#endif /* ATRACE_H */
