#pragma once

// ALL WM_USER messages are to be declared here *after* 'WM_FIRST_EMULE_USER_MSG'
enum EUserWndMessages
{
    // *) Do *NOT* place any message *before* WM_FIRST_EMULE_USER_MSG !!
    // *) Do *NOT* use any WM_USER messages in the range WM_USER - WM_USER+0x100 !!
    UM_FIRST_EMULE_USER_MSG = (WM_USER + 0x100 + 1),

    // Taskbar
    UM_TASKBARNOTIFIERCLICKED,
    UM_TRAY_ICON_NOTIFY_MESSAGE,
    UM_CLOSE_MINIMULE,

    // Webserver
    WEB_GUI_INTERACTION,
    WEB_CLEAR_COMPLETED,
    WEB_FILE_RENAME,
    WEB_ADDDOWNLOADS,
    WEB_CATPRIO,
    WEB_ADDREMOVEFRIEND,

    // VC
    //UM_VERSIONCHECK_RESPONSE,

    // PC
    UM_PEERCHACHE_RESPONSE,

    UM_CLOSETAB,
    UM_QUERYTAB,
    UM_DBLCLICKTAB,

    UM_CPN_SELCHANGE,
    UM_CPN_DROPDOWN,
    UM_CPN_CLOSEUP,
    UM_CPN_SELENDOK,
    UM_CPN_SELENDCANCEL,

    UM_MEDIA_INFO_RESULT,
    UM_ITEMSTATECHANGED,
    UM_SPN_SIZED,
    UM_TABMOVED,
    UM_TREEOPTSCTRL_NOTIFY,
    UM_DATA_CHANGED,
    UM_OSCOPEPOSITION,
    UM_DELAYED_EVALUATE,
    UM_ARCHIVESCANDONE,

    // UPnP
    UM_UPNP_RESULT,
#ifdef USE_NAT_PMP
    UM_NATPMP_RESULT //>>> WiZaRd::NAT-PMP
#endif
};
