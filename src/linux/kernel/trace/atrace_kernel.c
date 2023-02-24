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
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/module.h>
#ifdef MET_SUPPORT
#define MET_OFFSET	65535
#else
#define MET_OFFSET 0
#endif
/**
 * Trace the beginning of a context.  name is used to identify the context.
 * This is often used to time function execution.
 */
#define TRACE_BUF_SIZE 500
static void tracing_mark_write(const char *fmt, ...)
{
	char buffer[TRACE_BUF_SIZE];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buffer, TRACE_BUF_SIZE, fmt, ap);
	va_end(ap);
	/*printk("adam buffer = %s\n", buffer);*/
	trace_printk("%s", buffer);

}

void atrace_begin(const char *name)
{
	tracing_mark_write("B|%d|%s\n", (current->tgid + MET_OFFSET), name);
}
EXPORT_SYMBOL(atrace_begin);

/**
 * Trace the end of a context.
 * This should match up (and occur after) a corresponding ATRACE_BEGIN.
 */
void atrace_end(const char *name)
{
	tracing_mark_write("E|%s\n", name);
}
EXPORT_SYMBOL(atrace_end);

/**
 * Trace the beginning of an asynchronous event. Unlike ATRACE_BEGIN/ATRACE_END
 * contexts, asynchronous events do not need to be nested. The name describes
 * the event, and the cookie provides a unique identifier for distinguishing
 * simultaneous events. The name and cookie used to begin an event must be
 * used to end it.
 */
void atrace_async_begin(const char *name, int32_t cookie)
{
	tracing_mark_write("S|%d|%s|%d\n", (current->tgid + MET_OFFSET), name,
			   cookie);
}
EXPORT_SYMBOL(atrace_async_begin);

/**
 * Trace the end of an asynchronous event.
 * This should have a corresponding ATRACE_ASYNC_BEGIN.
 */
void atrace_async_end(const char *name, int32_t cookie)
{
	tracing_mark_write("F|%d|%s|%d\n", (current->tgid + MET_OFFSET), name,
			   cookie);
}
EXPORT_SYMBOL(atrace_async_end);

/**
 * Traces an integer counter value.  name is used to identify the counter.
 * This can be used to track how a value changes over time.
 */
void atrace_int(const char *name, int32_t value)
{
	tracing_mark_write("C|%d|%s|%d\n", (current->tgid + MET_OFFSET), name,
			   value);
}
EXPORT_SYMBOL(atrace_int);

/**
 * Traces a 64-bit integer counter value.  name is used to identify the
 * counter. This can be used to track how a value changes over time.
 */
void atrace_int64(const char *name, int64_t value)
{
	tracing_mark_write("C|%d|%s|%lld\n", (current->tgid + MET_OFFSET), name,
			   value);
}
EXPORT_SYMBOL(atrace_int64);
