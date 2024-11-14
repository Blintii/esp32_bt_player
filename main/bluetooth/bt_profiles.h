/*
 * Bluetooth profiles
 */

#ifndef __BT_PROFILES_H__
#define __BT_PROFILES_H__


/*
 * GAP (Generic Access Profile)
 */
void bt_gap_init();
void bt_gap_show();
void bt_gap_hide();


/*
 * AVRCP (Audio/Video Remote Control Profile)
 */
/* transaction labels:
 *  AVRCP specific AV/C command and an AVRCP specific browsing command may be outstanding
 *  at the same time with the same transaction label. The AVCTP transaction label shall
 *  be used to match commands with responses. The application shall provide label values
 *  that permit differentiation between packets of different messages.
 *  (from AVCTP, AVDTP and AVRCP specification)*/
#define BT_AVRCP_TL_GET_CAPS 0
void bt_avrcp_init();


/*
 * A2DP (Advanced Audio Distribution Profile)
 */
void bt_a2dp_init();


/*
 * L2CAP (Logical Link Control and Adaptation Protocol)
 *  in menuconfig can be enabled, but not needed because
 *  already internally used by A2DP and AVRCP
 */


#endif /* __BT_PROFILES_H__ */
