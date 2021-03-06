#ifndef SYSTEM_TIMER_H
#define SYSTEM_TIMER_H

#include <locking/spinlock.h>
#include <shared/list.h>
#include <stdint.h>

#define TIMER_MAGIC 0x4b87ad6e

struct timer_list
{
	uint64_t expires;
	void (*function)(struct timer_list *);
	struct list_head sibling;
	// TODO: MQ 2020-07-02 If there is the issue, which timer is deleted and iterated, considering using lock (better mutex)
	spinlock_t lock;
	uint32_t magic;
};

#define TIMER_INITIALIZER(_function, _expires) \
	{                                          \
		.function = (_function),               \
		.expires = (_expires),                 \
		.lock = 0,                             \
		.magic = TIMER_MAGIC                   \
	}

#define from_timer(var, callback_timer, timer_fieldname) \
	container_of(callback_timer, typeof(*var), timer_fieldname)

void add_timer(struct timer_list *timer);
void del_timer(struct timer_list *timer);
void mod_timer(struct timer_list *timer, uint64_t expires);
bool is_actived_timer(struct timer_list *timer);
void timer_init();

#endif
