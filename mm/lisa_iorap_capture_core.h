/* SPDX-License-Identifier: GPL-2.0-only */
/* Bounded event state shared by the kernel adapter and native boundary tests. */
#ifndef LISA_IORAP_CAPTURE_CORE_H
#define LISA_IORAP_CAPTURE_CORE_H

struct lisa_iorap_event {
	void *file;
	unsigned long index;
	void *mapping; /* Pinned by the stored file reference. */
};

struct lisa_iorap_capture_state {
	struct lisa_iorap_event *events;
	unsigned int *seen; /* Optional open-addressed event-index table. */
	unsigned long seen_capacity;
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
	if (s->seen) {
		unsigned long i;
		if (!s->seen_capacity || (s->seen_capacity & (s->seen_capacity - 1)) ||
		    s->seen_capacity / 2 < s->capacity || s->capacity > ~0U)
			return LISA_IORAP_INVALID;
		for (i = 0; i < s->seen_capacity; i++)
			s->seen[i] = 0;
	}
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

static inline int lisa_iorap_state_record_mapping(struct lisa_iorap_capture_state *s,
			unsigned int uid, void *file, void *mapping, unsigned long index)
{
	struct lisa_iorap_event *last;
	unsigned long slot = 0, probe, value;

	if (!s->active || uid != s->uid)
		return LISA_IORAP_IGNORED;
	if (!file || !mapping || index >= s->max_pages) {
		lisa_iorap_count_drop(s);
		return LISA_IORAP_INVALID;
	}
	if (s->seen) {
		value = (unsigned long)mapping ^ (index * 0x9e3779b9UL);
		value ^= value >> 16;
		value *= 0x7feb352dUL;
		value ^= value >> 15;
		value *= 0x846ca68bUL;
		value ^= value >> 16;
		slot = value & (s->seen_capacity - 1);
		for (probe = 0; probe < s->seen_capacity; probe++) {
			unsigned int entry = s->seen[slot];
			if (!entry)
				break;
			last = &s->events[entry - 1];
			if (last->mapping == mapping && last->index == index)
				return LISA_IORAP_IGNORED;
			slot = (slot + 1) & (s->seen_capacity - 1);
		}
		if (probe == s->seen_capacity) {
			lisa_iorap_count_drop(s);
			return LISA_IORAP_OVERFLOW;
		}
	} else if (s->count) {
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
	s->events[s->count].mapping = mapping;
	s->count++;
	if (s->seen)
		s->seen[slot] = (unsigned int)s->count;
	return LISA_IORAP_RECORDED;
}

/* Existing callers use the file identity; kernel supplies address_space. */
static inline int lisa_iorap_state_record(struct lisa_iorap_capture_state *s,
		unsigned int uid, void *file, unsigned long index)
{
	return lisa_iorap_state_record_mapping(s, uid, file, file, index);
}

static inline int lisa_iorap_state_end(struct lisa_iorap_capture_state *s)
{
	if (!s->active)
		return LISA_IORAP_INVALID;
	s->active = 0;
	return 0;
}
#endif
