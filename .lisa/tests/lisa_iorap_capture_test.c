/* SPDX-License-Identifier: GPL-2.0-only */
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include "../../mm/lisa_iorap_capture_core.h"

static struct lisa_iorap_event events[4096];
static struct lisa_iorap_capture_state state = {
	.events = events, .capacity = 4096, .max_pages = 524288,
};
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned long accepted;
static int file_tokens[4];

static void *record_thread(void *arg)
{
	unsigned long id = (unsigned long)arg;
	unsigned long i;
	for (i = 0; i < 3000; i++) {
		pthread_mutex_lock(&lock);
		if (lisa_iorap_state_record(&state, 10001, &file_tokens[id], i)
		    == LISA_IORAP_RECORDED)
			accepted++;
		pthread_mutex_unlock(&lock);
	}
	return NULL;
}

int main(void)
{
	pthread_t threads[4];
	unsigned long i;

	assert(lisa_iorap_state_record(&state, 10001, &file_tokens[0], 0) == 0);
	assert(lisa_iorap_state_end(&state) == LISA_IORAP_INVALID);
	assert(lisa_iorap_state_begin(&state, 10001) == 0);
	assert(lisa_iorap_state_begin(&state, 10002) != 0);
	assert(lisa_iorap_state_record(&state, 10002, &file_tokens[0], 0) == 0);
	assert(lisa_iorap_state_record(&state, 10001, NULL, 0) == LISA_IORAP_INVALID);
	assert(lisa_iorap_state_record(&state, 10001, &file_tokens[0], 524288) == LISA_IORAP_INVALID);
	assert(state.dropped == 2 && state.count == 0);
	assert(lisa_iorap_state_record(&state, 10001, &file_tokens[0], 1) == 1);
	assert(lisa_iorap_state_record(&state, 10001, &file_tokens[0], 1) == 0);
	assert(state.count == 1);
	assert(lisa_iorap_state_end(&state) == 0);
	assert(lisa_iorap_state_record(&state, 10001, &file_tokens[0], 2) == 0);
	assert(lisa_iorap_state_begin(&state, 10001) != 0); /* Undrained refs. */
	state.count = 0;
	assert(lisa_iorap_state_begin(&state, 10001) == 0);
	for (i = 0; i < 4; i++)
		assert(pthread_create(&threads[i], NULL, record_thread, (void *)i) == 0);
	for (i = 0; i < 4; i++)
		assert(pthread_join(threads[i], NULL) == 0);
	assert(accepted == 4096 && state.count == 4096);
	assert(state.dropped == 12000 - 4096);
	assert(lisa_iorap_state_end(&state) == 0);
	state.count = 0;
	assert(lisa_iorap_state_begin(&state, 10002) == 0);
	assert(state.dropped == 0);
	state.dropped = ~0UL;
	assert(lisa_iorap_state_record(&state, 10002, NULL, 0) == LISA_IORAP_INVALID);
	assert(state.dropped == ~0UL);
	assert(lisa_iorap_state_end(&state) == 0);
	puts("PASS capture core: UID, lifecycle, duplicate, quota, range, saturation, 4-thread/12000 events");
	return 0;
}
