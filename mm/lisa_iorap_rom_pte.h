/* SPDX-License-Identifier: GPL-2.0-only */
/* ROM-specific anonymous swap-in advice, not page-table/content export.
 * Only the already authorized root/system device endpoint can enter here.
 */
#ifdef CONFIG_LISA_IORAP_ROM_PTE
#include <linux/bitmap.h>
#include <linux/huge_mm.h>
#include <linux/hugetlb.h>
#include <linux/pagewalk.h>
#include <linux/pid.h>
#include <linux/rmap.h>
#include <linux/sched/mm.h>
#include <linux/sched/signal.h>
#define IORAP_PTE_CHUNKS 2048U
#define IORAP_PTE_PAGES 131072UL /* At most 512 MiB of remembered pages. */
#define IORAP_PTE_SLOTS 8U
#define IORAP_PTE_RANGE_PAGES 128UL
struct iorap_pte_chunk {
	unsigned long base, vma_start, vma_end;
	struct anon_vma *anon;
	DECLARE_BITMAP(bits, PTRS_PER_PTE);
};
struct iorap_pte_plan {
	struct lisa_iorap_legacy_request key;
	struct mm_struct *mm;
	unsigned int count;
	unsigned long pages, used;
	struct iorap_pte_chunk chunks[];
};
struct iorap_pte_walk {
	struct iorap_pte_plan *plan;
	unsigned long deadline;
};
static DEFINE_MUTEX(pte_control);
static DEFINE_SPINLOCK(pte_active_lock);
static struct iorap_pte_plan *pte_plans[IORAP_PTE_SLOTS];
static struct lisa_iorap_legacy_request pte_active_key;
static bool pte_running;
static atomic_t pte_cancel = ATOMIC_INIT(0);
static bool pte_key_equal(const struct lisa_iorap_legacy_request *a,
		const struct lisa_iorap_legacy_request *b)
{
	return a->pid == b->pid && lisa_iorap_legacy_same_app(a, b);
}
static void pte_plan_put(struct iorap_pte_plan *p)
{
	unsigned int i;
	if (!p)
		return;
	for (i = 0; i < p->count; i++)
		put_anon_vma(p->chunks[i].anon);
	if (p->mm)
		mmdrop(p->mm); /* Only mm_count, never keep an exited task's pages. */
	kvfree(p);
}
static int pte_task_mm(const struct lisa_iorap_legacy_request *key,
		struct task_struct **target, struct mm_struct **mm)
{
	struct pid *pid;
	struct task_struct *task;
	if (!key->pid)
		return -EINVAL;
	pid = find_get_pid(key->pid);
	if (!pid)
		return -ESRCH;
	task = get_pid_task(pid, PIDTYPE_PID);
	put_pid(pid);
	if (!task)
		return -ESRCH;
	if (!uid_eq(task_uid(task), make_kuid(&init_user_ns, key->uid))) {
		put_task_struct(task);
		return -EPERM;
	}
	*mm = get_task_mm(task);
	if (!*mm) {
		put_task_struct(task);
		return -ESRCH;
	}
	/* A concurrent credential change must not authorize a different UID. */
	if (!uid_eq(task_uid(task), make_kuid(&init_user_ns, key->uid))) {
		mmput(*mm);
		put_task_struct(task);
		return -EPERM;
	}
	*target = task;
	return 0;
}
static int pte_walk_test(unsigned long start, unsigned long end, struct mm_walk *walk)
{
	struct vm_area_struct *vma = walk->vma;
	(void)start;
	(void)end;
	return vma->vm_file || !(vma->vm_flags & VM_READ) ||
		(vma->vm_flags & (VM_IO | VM_PFNMAP | VM_HUGETLB)) ? 1 : 0;
}
static int pte_walk_pmd(pmd_t *pmd, unsigned long start, unsigned long end,
		struct mm_walk *walk)
{
	struct iorap_pte_walk *ctx = walk->private;
	struct iorap_pte_plan *plan = ctx->plan;
	struct iorap_pte_chunk *chunk;
	struct iorap_pte_chunk scratch;
	struct vm_area_struct *vma = walk->vma;
	unsigned long addr, n;
	spinlock_t *ptl;
	pte_t *first, *ptep;
	if (fatal_signal_pending(current))
		return -EINTR;
	if (time_after_eq(jiffies, ctx->deadline))
		return -ETIMEDOUT;
	if (pmd_none(*pmd))
		return 0;
	chunk = plan->count < IORAP_PTE_CHUNKS ?
		&plan->chunks[plan->count] : &scratch;
	chunk->base = start & PMD_MASK;
	chunk->vma_start = vma->vm_start;
	chunk->vma_end = vma->vm_end;
	chunk->anon = vma->anon_vma;
	bitmap_zero(chunk->bits, PTRS_PER_PTE);
	ptl = pmd_trans_huge_lock(pmd, vma);
	if (ptl) {
		if (pmd_present(*pmd) && pmd_trans_huge(*pmd) &&
		    PageAnon(pmd_page(*pmd)))
			for (addr = start; addr < end; addr += PAGE_SIZE)
				__set_bit((addr - chunk->base) >> PAGE_SHIFT, chunk->bits);
		spin_unlock(ptl);
	} else {
		if (pmd_bad(*pmd) || pmd_trans_unstable(pmd))
			return 0;
		first = pte_offset_map_lock(walk->mm, pmd, start, &ptl);
		ptep = first;
		for (addr = start; addr < end; addr += PAGE_SIZE, ptep++) {
			pte_t entry = READ_ONCE(*ptep);
			struct page *page;
			if (!pte_present(entry) || pte_special(entry))
				continue;
			page = vm_normal_page(vma, addr, entry);
			if (page && PageAnon(page))
				__set_bit((addr - chunk->base) >> PAGE_SHIFT, chunk->bits);
		}
		pte_unmap_unlock(first, ptl);
	}
	n = bitmap_weight(chunk->bits, PTRS_PER_PTE);
	if (n > IORAP_PTE_PAGES - plan->pages)
		return -E2BIG;
	if (n) {
		if (plan->count == IORAP_PTE_CHUNKS)
			return -ENOSPC;
		if (WARN_ON(!chunk->anon))
			return -ESTALE;
		get_anon_vma(chunk->anon);
		plan->count++;
		plan->pages += n;
	}
	cond_resched();
	return 0;
}
static int pte_walk_pud(pud_t *pud, unsigned long start, unsigned long end,
		struct mm_walk *walk)
{
	(void)pud; (void)start; (void)end; (void)walk;
	/* Never split or modify a huge PUD merely to take a snapshot. */
	return -EOPNOTSUPP;
}
static const struct mm_walk_ops iorap_pte_walk_ops = {
	.test_walk = pte_walk_test,
	.pmd_entry = pte_walk_pmd,
	.pud_entry = pte_walk_pud,
};
static int lisa_iorap_pte_snapshot(const struct lisa_iorap_legacy_request *key)
{
	struct task_struct *task;
	struct mm_struct *mm;
	struct iorap_pte_plan *plan;
	struct iorap_pte_walk ctx;
	unsigned int i, slot = IORAP_PTE_SLOTS;
	int error = pte_task_mm(key, &task, &mm);
	if (error)
		return error;
	mutex_lock(&pte_control);
	plan = kvzalloc(struct_size(plan, chunks, IORAP_PTE_CHUNKS), GFP_KERNEL);
	if (!plan) { error = -ENOMEM; goto mm; }
	plan->key = *key;
	ctx.plan = plan;
	ctx.deadline = jiffies + msecs_to_jiffies(500);
	down_read(&mm->mmap_sem);
	error = walk_page_range(mm, 0, TASK_SIZE, &iorap_pte_walk_ops, &ctx);
	up_read(&mm->mmap_sem);
	if (error || !plan->pages) {
		error = error ? error : -ENODATA;
		goto plan;
	}
	mmgrab(mm);
	plan->mm = mm;
	plan->used = jiffies;
	for (i = 0; i < IORAP_PTE_SLOTS; i++)
		if (pte_plans[i] && pte_key_equal(&pte_plans[i]->key, key)) {
			slot = i;
			break;
		}
	if (slot == IORAP_PTE_SLOTS)
		for (i = 0; i < IORAP_PTE_SLOTS; i++)
			if (!pte_plans[i]) { slot = i; break; }
	if (slot == IORAP_PTE_SLOTS) {
		slot = 0;
		for (i = 1; i < IORAP_PTE_SLOTS; i++)
			if (time_before(pte_plans[i]->used, pte_plans[slot]->used))
				slot = i;
	}
	pte_plan_put(pte_plans[slot]);
	pte_plans[slot] = plan;
	pr_info_ratelimited("lisa_iorap: pte snapshot uid=%u pid=%u chunks=%u pages=%lu\n",
		key->uid, key->pid, plan->count, plan->pages);
	plan = NULL;
plan:
	pte_plan_put(plan);
mm:
	mutex_unlock(&pte_control);
	mmput(mm);
	put_task_struct(task);
	return error;
}
static int lisa_iorap_pte_stop(const struct lisa_iorap_legacy_request *key)
{
	unsigned long flags;
	spin_lock_irqsave(&pte_active_lock, flags);
	if (pte_running && pte_key_equal(&pte_active_key, key))
		atomic_set(&pte_cancel, 1);
	spin_unlock_irqrestore(&pte_active_lock, flags);
	return 0; /* Idempotent cancellation; no invented preload result. */
}
static void lisa_iorap_pte_clear(const struct lisa_iorap_legacy_request *key)
{
	unsigned int i;
	unsigned long flags;
	/* Clear is an app-wide operation; a zero PID must still cancel that
	 * app's in-flight preread before waiting for its plan lock.
	 */
	spin_lock_irqsave(&pte_active_lock, flags);
	if (pte_running && lisa_iorap_legacy_same_app(&pte_active_key, key))
		atomic_set(&pte_cancel, 1);
	spin_unlock_irqrestore(&pte_active_lock, flags);
	mutex_lock(&pte_control);
	for (i = 0; i < IORAP_PTE_SLOTS; i++)
		if (pte_plans[i] && lisa_iorap_legacy_same_app(&pte_plans[i]->key, key)) {
			pte_plan_put(pte_plans[i]);
			pte_plans[i] = NULL;
		}
	mutex_unlock(&pte_control);
}
static int lisa_iorap_pte_preread(const struct lisa_iorap_legacy_request *key)
{
	struct task_struct *task;
	struct mm_struct *mm;
	struct iorap_pte_plan *plan = NULL;
	unsigned long flags, first, end, addr, deadline;
	unsigned int i;
	int error = pte_task_mm(key, &task, &mm);
	if (error)
		return error;
	mutex_lock(&pte_control);
	for (i = 0; i < IORAP_PTE_SLOTS; i++)
		if (pte_plans[i] && pte_key_equal(&pte_plans[i]->key, key)) {
			plan = pte_plans[i];
			break;
		}
	if (!plan) {
		pr_warn_ratelimited("lisa_iorap: pte reject stage=missing uid=%u pid=%u\n",
			key->uid, key->pid);
		error = -ENOENT; goto out;
	}
	if (plan->mm != mm) {
		pr_warn_ratelimited("lisa_iorap: pte reject stage=mm uid=%u pid=%u chunks=%u pages=%lu\n",
			key->uid, key->pid, plan->count, plan->pages);
		error = -ESTALE; goto out;
	}
	spin_lock_irqsave(&pte_active_lock, flags);
	pte_active_key = *key;
	atomic_set(&pte_cancel, 0);
	pte_running = true;
	spin_unlock_irqrestore(&pte_active_lock, flags);
	deadline = jiffies + msecs_to_jiffies(500);
	for (i = 0; i < plan->count && !error; i++) {
		struct iorap_pte_chunk *chunk = &plan->chunks[i];
		first = find_first_bit(chunk->bits, PTRS_PER_PTE);
		while (first < PTRS_PER_PTE) {
			struct vm_area_struct *vma;
			if (atomic_read(&pte_cancel) || fatal_signal_pending(current)) {
				error = -EINTR; break;
			}
			if (time_after_eq(jiffies, deadline)) {
				error = -ETIMEDOUT; break;
			}
			end = find_next_zero_bit(chunk->bits, PTRS_PER_PTE, first);
			end = min(end, first + IORAP_PTE_RANGE_PAGES);
			addr = chunk->base + (first << PAGE_SHIFT);
			down_read(&mm->mmap_sem);
			vma = find_vma(mm, addr);
			if (!vma || vma->vm_start != chunk->vma_start ||
			    vma->vm_end != chunk->vma_end || vma->anon_vma != chunk->anon ||
			    vma->vm_file || !(vma->vm_flags & VM_READ)) {
				pr_warn_ratelimited("lisa_iorap: pte reject stage=vma uid=%u pid=%u chunk=%u missing=%u start_changed=%u end_changed=%u anon_changed=%u file=%u unreadable=%u\n",
					key->uid, key->pid, i, !vma,
					!!(vma && vma->vm_start != chunk->vma_start),
					!!(vma && vma->vm_end != chunk->vma_end),
					!!(vma && vma->anon_vma != chunk->anon),
					!!(vma && vma->vm_file),
					!!(vma && !(vma->vm_flags & VM_READ)));
				error = -ESTALE;
			} else {
				error = lisa_iorap_anon_willneed(vma, addr,
					chunk->base + (end << PAGE_SHIFT));
				if (error)
					pr_warn_ratelimited("lisa_iorap: pte reject stage=advice uid=%u pid=%u chunk=%u error=%d\n",
						key->uid, key->pid, i, error);
			}
			up_read(&mm->mmap_sem);
			if (error) break;
			first = find_next_bit(chunk->bits, PTRS_PER_PTE, end);
			cond_resched();
		}
	}
	spin_lock_irqsave(&pte_active_lock, flags);
	pte_running = false;
	spin_unlock_irqrestore(&pte_active_lock, flags);
	plan->used = jiffies;
	if (!error)
		pr_info_ratelimited("lisa_iorap: pte preread completed uid=%u pid=%u chunks=%u pages=%lu\n",
			key->uid, key->pid, plan->count, plan->pages);
out:
	mutex_unlock(&pte_control);
	mmput(mm);
	put_task_struct(task);
	return error;
}
#endif
