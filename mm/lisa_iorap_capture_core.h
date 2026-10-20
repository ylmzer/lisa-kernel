/* SPDX-License-Identifier: GPL-2.0-only */
/* Bounded event state shared by the kernel adapter and native boundary tests. */
#ifndef LISA_IORAP_CAPTURE_CORE_H
#define LISA_IORAP_CAPTURE_CORE_H

struct lisa_iorap_event {
	void *file;
	unsigned long index;
};

struct lisa_iorap_capture_state {
	struct lisa_iorap_event *events;
	unsigned long capacity;
	unsigned long count;
	unsigned long dropped;
	unsigned long max_pages;
	unsigned int uid;
	int active;
};

enum lisa_iorap_record_result {
	LISA_IORAP_IGNORED = 0,
	LISA_IORAP_RECORDED = 1,
	LISA_IORAP_OVERFLOW = -1,
	LISA_IORAP_INVALID = -2,
};

/* All state access, including reference acquisition, is serialized by caller. */
static inline int lisa_iorap_state_begin(struct lisa_iorap_capture_state *s,
					 unsigned int uid)
{
	if (!s->events || !s->capacity || !s->max_pages)
		return LISA_IORAP_INVALID;
	if (s->active || s->count)
		return LISA_IORAP_OVERFLOW;
	s->uid = uid;
	s->dropped = 0;
	s->active = 1;
	return 0;
}

static inline void lisa_iorap_count_drop(struct lisa_iorap_capture_state *s)
{
	if (s->dropped != ~0UL)
		s->dropped++;
}

static inline int lisa_iorap_state_record(struct lisa_iorap_capture_state *s,
			unsigned int uid, void *file, unsigned long index)
{
	struct lisa_iorap_event *last;

	if (!s->active || uid != s->uid)
		return LISA_IORAP_IGNORED;
	if (!file || index >= s->max_pages) {
		lisa_iorap_count_drop(s);
		return LISA_IORAP_INVALID;
	}
	if (s->count) {
		last = &s->events[s->count - 1];
		if (last->file == file && last->index == index)
			return LISA_IORAP_IGNORED;
	}
	if (s->count == s->capacity) {
		lisa_iorap_count_drop(s);
		return LISA_IORAP_OVERFLOW;
	}
	s->events[s->count].file = file;
	s->events[s->count].index = index;
	s->count++;
	return LISA_IORAP_RECORDED;
}

static inline int lisa_iorap_state_end(struct lisa_iorap_capture_state *s)
{
	if (!s->active)
		return LISA_IORAP_INVALID;
	s->active = 0;
	return 0;
}
#endif
