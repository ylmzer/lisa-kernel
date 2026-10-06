/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LISA_MILLET_VALIDATE_H
#define LISA_MILLET_VALIDATE_H
static inline bool millet_frame_valid(unsigned int uid, unsigned int actual,
 unsigned int claimed, unsigned int port, unsigned int claimed_port)
{
 return (uid == 0 || uid == 1000) && actual >= 96 && claimed >= 96 &&
        claimed <= actual && port && port == claimed_port;
}
static inline bool millet_payload_valid(const struct millet_userconf *p)
{
 return p->src_port == MILLET_USER_ID && p->dst_port == MILLET_KERNEL_ID &&
        TYPE_VALID(p->owner) &&
        (p->msg_type == LOOPBACK_MSG || p->msg_type == MSG_TO_KERN);
}
#endif
