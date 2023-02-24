/*
 *  Copyright 2013 Sony Corporation.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  version 2 of the  License.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */

#pragma once

#include <linux/time.h>
#include <linux/spinlock.h>

typedef struct log_info {
	unsigned int info_offset;
	unsigned int buf_offset;
	unsigned int buf_size;
	volatile unsigned int write_offset;
	volatile unsigned int read_offset;
	unsigned int overflows;
	unsigned int overflowed_readoffset;
} log_info;

typedef struct log_header {
	volatile unsigned int prev_offset;
	volatile unsigned int next_offset;
	volatile unsigned int status;
	struct timespec time;
	volatile unsigned int size;
	volatile unsigned int string_offset;
} log_header;

extern char *log_buffer;
extern spinlock_t log_lock;
extern int enabled_writing;
extern int (*write_log_driver) (const char *printbuf, size_t size);

extern void log_disable_write(void);
extern void log_enable_write(void);

