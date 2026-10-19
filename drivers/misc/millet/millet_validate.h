/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LISA_MILLET_VALIDATE_H
#define LISA_MILLET_VALIDATE_H
static inline bool millet_frame_valid(unsigned int uid, unsigned int actual,
 unsigned int claimed, unsigned int port, unsigned int claimed_port)
{
 return (uid == 0 || uid == 1000) && actual >= 96 && claimed >= 96 &&
        claimed <= actual && port && claimed_port;
}
static inline bool millet_sender_valid(unsigned int port,
 unsigned int claimed_port, const struct millet_userconf *p)
{
 /* libmillet_comm's shared JNI socket binds PID, but sends PID + owner.
  * Dedicated monitor sockets bind PID + type and use an exact match.
  * Only commands accept the JNI convention; registration and reply routing
  * always use the kernel-authenticated NETLINK_CB(skb).portid.
  */
 return port == claimed_port ||
        (p->msg_type == MSG_TO_KERN && TYPE_VALID(p->owner) &&
         claimed_port > port && claimed_port - port == p->owner);
}
static inline bool millet_payload_valid(const struct millet_userconf *p)
{
 return p->src_port == MILLET_USER_ID && p->dst_port == MILLET_KERNEL_ID &&
        TYPE_VALID(p->owner) &&
        (p->msg_type == LOOPBACK_MSG || p->msg_type == MSG_TO_KERN);
}
#endif
