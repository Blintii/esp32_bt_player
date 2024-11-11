/*
 * Bluetooth AVRCP handler (Audio/Video Remote Control Profile)
 */

#ifndef __BT_AVRCP_H__
#define __BT_AVRCP_H__


/* AVRCP used transaction labels */
#define APP_RC_CT_TL_GET_CAPS            (0)
#define APP_RC_CT_TL_GET_META_DATA       (1)
#define APP_RC_CT_TL_RN_TRACK_CHANGE     (2)
#define APP_RC_CT_TL_RN_PLAYBACK_CHANGE  (3)
#define APP_RC_CT_TL_RN_PLAY_POS_CHANGE  (4)


void bt_avrcp_init();


#endif /* __BT_AVRCP_H__ */
