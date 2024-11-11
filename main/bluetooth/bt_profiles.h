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
void bt_gap_led_set_weak();
void bt_gap_led_set_fast();


/*
 * AVRCP (Audio/Video Remote Control Profile)
 */
/* transaction labels */
#define APP_RC_CT_TL_GET_CAPS            (0)
#define APP_RC_CT_TL_GET_META_DATA       (1)
#define APP_RC_CT_TL_RN_TRACK_CHANGE     (2)
#define APP_RC_CT_TL_RN_PLAYBACK_CHANGE  (3)
#define APP_RC_CT_TL_RN_PLAY_POS_CHANGE  (4)
void bt_avrcp_init();


/*
 * A2DP (Advanced Audio Distribution Profile)
 */
void bt_a2dp_init();


#endif /* __BT_PROFILES_H__ */
