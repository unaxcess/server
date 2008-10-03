/*
** UNaXcess II Conferencing System
** (c) 1998 Michael Wood (mike@compsoc.man.ac.uk)
**
** Concepts based on Bradford UNaXcess (c) 1984-87 Brandon S Allbery
** Extensions (c) 1989, 1990 Andrew G Minter
** Manchester UNaXcess extensions by Rob Partington, Gryn Davies,
** Michael Wood, Andrew Armitage, Francis Cook, Brian Widdas
**
** The look and feel was reproduced. No code taken from the original
** UA was someone else's inspiration. Copyright and 'nuff respect due
**
** ua.h: Definitions for UA message protocol
*/

#ifndef _UA_H_
#define _UA_H_

// MessageSpec start

// Protocol
#define PROTOCOL "2.8-alpha"

// String lengths (UA_ prefix introduced in v2.4-beta8 due to conflicts with MySQL)
#define UA_NAME_LEN 20
#define UA_SHORTMSG_LEN 100
#define UA_ACCESSNAME_LEN 10

// User types
#define USERTYPE_NONE 0
#define USERTYPE_DELETED 2
#define USERTYPE_AGENT 1
#define USERTYPE_TEMP 4

// Access levels
#define LEVEL_NONE 0
#define LEVEL_GUEST 1
#define LEVEL_MESSAGES 2
#define LEVEL_EDITOR 3
#define LEVEL_WITNESS 4
#define LEVEL_SYSOP 5

// Gender types
#define GENDER_PERSON 0
#define GENDER_MALE 1
#define GENDER_FEMALE 2
#define GENDER_NONE 3

// Login status code flags
#define LOGIN_OFF 0
#define LOGIN_ON 1
#define LOGIN_BUSY 2
#define LOGIN_IDLE 4
#define LOGIN_TALKING 8
#define LOGIN_AGENT 16 // Deprecated in v2.2a
#define LOGIN_GONE 32 // Deprecated in v2.0
#define LOGIN_SILENT 64
#define LOGIN_NOCONTACT 128 // Added in v2.5-alpha, mapped to LOGIN_SHADOW in v2.6-beta3
#define LOGIN_SHADOW 256 // Added in v2.5-alpha
#define LOGIN_BUSYFRIENDS 512 // Added in v2.6-beta4

// Detail types
#define DETAIL_PUBLIC 1
#define DETAIL_VALID 2
#define DETAIL_OWNER 4
#define DETAIL_PROXY 8

// Access rights (expanded from folder-only rights in v2.5-beta5)
#define ACCMODE_SUB_READ 1
#define ACCMODE_SUB_WRITE 2
#define ACCMODE_MEM_READ 16
#define ACCMODE_MEM_WRITE 32
#define ACCMODE_PRIVATE 256

#define ACCMODE_READONLY ACCMODE_SUB_READ

// Folder specific, changed in v2.6-beta6
#define FOLDERMODE_SUB_SDEL 4
#define FOLDERMODE_SUB_ADEL 8
#define FOLDERMODE_SUB_MOVE 512
#define FOLDERMODE_MEM_SDEL 64
#define FOLDERMODE_MEM_ADEL 128
#define FOLDERMODE_MEM_MOVE 1024
#define FOLDERMODE_CONTENT 2048 // Added in v2.7-alpha
#define FOLDERMODE_PERSIST 4096 // Added in v2.7-alpha

#define FOLDERMODE_NORMAL ACCMODE_SUB_READ + ACCMODE_SUB_WRITE + FOLDERMODE_SUB_SDEL
#define FOLDERMODE_RESTRICTED ACCMODE_MEM_READ + ACCMODE_MEM_WRITE + FOLDERMODE_MEM_SDEL

// #define ACCMODE_NORMAL FOLDERMODE_NORMAL // Deprecated in v2.6-alpha, use FOLDERMODE_NORMAL
// #define ACCMODE_RESTRICTED FOLDERMODE_RESTRICTED // Deprecated in v2.6-alpha, use FOLDERMODE_RESTRICTED

// Channel specific (added in v2.6-alpha)
#define CHANNELMODE_PERMANENT 512
#define CHANNELMODE_LOGGED 1024
#define CHANNELMODE_LOGIN 2048

#define CHANNELMODE_NORMAL ACCMODE_SUB_READ + ACCMODE_SUB_WRITE
#define CHANNELMODE_RESTRICTED ACCMODE_MEM_READ + ACCMODE_MEM_WRITE
#define CHANNELMODE_DEBATE ACCMODE_MEM_READ + ACCMODE_MEM_WRITE + ACCMODE_SUB_READ

// Message types (expanded v2.5-beta6)
#define MSGTYPE_NONE 0
#define MSGTYPE_DELETED 1
#define MSGTYPE_VOTE 2
#define MSGTYPE_ARCHIVE 4

// Marking types
#define MARKING_ADD_PRIVATE 1
#define MARKING_ADD_PUBLIC 2
#define MARKING_EDIT_PRIVATE 4
#define MARKING_EDIT_PUBLIC 8

#define MARKED_READ 1
#define MARKED_UNREAD 2

// Subscription types (expanded from folder-only types v2.4-beta5)
#define SUBTYPE_SUB 1
#define SUBTYPE_MEMBER 2
#define SUBTYPE_EDITOR 3
#define SUBTYPE_ACTIVE 4 // Deprecated in v2.6-alpha

// Folder subscription types for backwards compatiblity (deprecated in v2.4-beta5)
// #define FOLDER_SUBTYPE_SUB SUBTYPE_SUB
// #define FOLDER_SUBTYPE_MEMBER SUBTYPE_MEMBER
// #define FOLDER_SUBTYPE_EDITOR SUBTYPE_EDITOR

#define SUBNAME_SUB "subscriber"
#define SUBNAME_MEMBER "member"
#define SUBNAME_EDITOR "editor"

// Backwards compatiblity (deprecated in v2.4-beta5)
// #define FOLDER_SUBNAME_SUB SUBNAME_SUB
// #define FOLDER_SUBNAME_MEMBER SUBNAME_MEMBER
// #define FOLDER_SUBNAME_EDITOR SUBNAME_EDITOR

// Voting types
#define VOTE_SIMPLE 1 // Deprecated in v2.2a
#define VOTE_COMPLEX 2 // Deprecated in v2.2a
#define VOTE_NAMED 1
// #define VOTE_CHOICE 2 // Deprecated in v2.6-beta6
#define VOTE_CHANGE 4
#define VOTE_MULTI 64 // Added in v2.6-beta4
#define VOTE_PUBLIC 8
#define VOTE_PUBLIC_CLOSE 16
#define VOTE_CLOSED 32
#define VOTE_INTVALUES 128 // Added in v2.6-beta16
#define VOTE_PERCENT 256 // Added in v2.6-beta16
#define VOTE_STRVALUES 512 // Added in v2.6-beta16
#define VOTE_FLOATVALUES 1024 // Added in v2.7-beta8
#define VOTE_FLOATPERCENT 2048 // Added in v2.7-beta8

// Message attachment types
#define MSGATT_EDF "text/x-edf"
#define MSGATT_ANNOTATION "text/x-ua-annotation"
#define MSGATT_LINK "text/x-ua-link"

#define MSGATT_FAILED 1
#define MSGATT_TOOBIG 2
#define MSGATT_MIMETYPE 3

// Message thread operation (mark_read, mark_unread, move, delete) types - added v2.6-beta5
#define THREAD_MSGCHILD 1
#define THREAD_CHILD 2

/*
** Channel types (deprecated in v2.4-beta5, see access rights)
#define CHANNEL_PUBLIC 1
#define CHANNEL_PRIVATE 2
#define CHANNEL_PERMANENT 4
#define CHANNEL_PEER 8
*/

/*
** Channel subscription types (deprecated in v2.4-beta5, see subscription types)
#define CHANNEL_MEMBER 1
#define CHANNEL_MODERATOR 2
#define CHANNEL_OWNER 4
#define CHANNEL_ACTIVE 8
*/

// Contact types (deprecated in v2.6-alpha - use services)
#define CONTACT_PAGE 0
#define CONTACT_EMAIL 1
#define CONTACT_SMS 2
#define CONTACT_POST 3 // Use divert flag

// Service types (added in v2.6-alpha)
#define SERVICE_CONTACT 1
#define SERVICE_LOGIN 2
#define SERVICE_TONAME 4

// Task repeat times
#define TASK_DAILY 1
#define TASK_WEEKDAY 2
#define TASK_WEEKEND 3
#define TASK_WEEKLY 4

// Message archiving (added in v2.7-beta7)
#define ARCHIVE_PUBLIC 1
#define ARCHIVE_PRIVATE 2

// Service actions (added in v2.6-beta2)
#define ACTION_LIST "list"
#define ACTION_LOGIN "login"
#define ACTION_LOGOUT "logout"
#define ACTION_LOGIN_INVALID "login_invalid"
#define ACTION_CONTACT_INVALID "contact_invalid"
#define ACTION_CONTACT_NOT_EXIST "contact_not_exist"
#define ACTION_CONTACT_NOT_ON "contact_not_on"
#define ACTION_CONTACT_LOGIN "contact_login"
#define ACTION_CONTACT_LOGOUT "contact_logout"
#define ACTION_CONTACT_STATUS "contact_status"



// Request / reply / announce messages
#define MSG_SYSTEM_EDIT "system_edit"
#define MSG_SYSTEM_LIST "system_list"
#define MSG_SYSTEM_WRITE "system_write"
#define MSG_SYSTEM_MESSAGE "system_message"
#define MSG_SYSTEM_SHUTDOWN "system_shutdown"
#define MSG_SYSTEM_MAINTENANCE "system_maintenance"
#define MSG_SYSTEM_CONTACT "system_contact" // Added in v2.6-alpha

#define MSG_CONNECTION_CLOSE "connection_close"
#define MSG_CONNECTION_DROP "connection_drop" // Deprecated in v2.5-beta4, use MSG_CONNECTION_CLOSE

#define MSG_BULLETIN_ADD "bulletin_add"
#define MSG_BULLETIN_DELETE "bulletin_delete"
#define MSG_BULLETIN_EDIT "bulletin_edit"
#define MSG_BULLETIN_LIST "bulletin_list"

// Added in v2.6-beta11
#define MSG_SERVICE_ADD "service_add"
#define MSG_SERVICE_DELETE "service_delete"
#define MSG_SERVICE_EDIT "service_edit"
#define MSG_SERVICE_LIST "service_list"
#define MSG_SERVICE_SUBSCRIBE "service_subscribe"
#define MSG_SERVICE_UNSUBSCRIBE "service_unsubscribe"

#define MSG_LOCATION_ADD "location_add"
#define MSG_LOCATION_DELETE "location_delete"
#define MSG_LOCATION_EDIT "location_edit"
#define MSG_LOCATION_LIST "location_list"
#define MSG_LOCATION_SORT "location_sort" // Added in v2.5-beta7
#define MSG_LOCATION_LOOKUP "location_lookup" // Added in v2.5-beta8

#define MSG_HELP_ADD "help_add"
#define MSG_HELP_DELETE "help_delete"
#define MSG_HELP_EDIT "help_edit"
#define MSG_HELP_LIST "help_list"

// Task scheduling (added in v2.5-beta2)
#define MSG_TASK_ADD "task_add"
#define MSG_TASK_DELETE "task_delete"
#define MSG_TASK_EDIT "task_edit"
#define MSG_TASK_LIST "task_list"

#define MSG_FOLDER_ADD "folder_add"
#define MSG_FOLDER_DELETE "folder_delete"
#define MSG_FOLDER_EDIT "folder_edit"
#define MSG_FOLDER_LIST "folder_list"
#define MSG_FOLDER_SUBSCRIBE "folder_subscribe"
#define MSG_FOLDER_UNSUBSCRIBE "folder_unsubscribe"
#define MSG_FOLDER_ACTIVATE "folder_activate"
#define MSG_FOLDER_DEACTIVATE "folder_deactivate"
#define MSG_MESSAGE_ADD "message_add"
#define MSG_MESSAGE_DELETE "message_delete"
#define MSG_MESSAGE_EDIT "message_edit"
#define MSG_MESSAGE_LIST "message_list"
#define MSG_MESSAGE_MOVE "message_move"
#define MSG_MESSAGE_MARK_READ "message_mark_read"
#define MSG_MESSAGE_MARK_UNREAD "message_mark_unread"
#define MSG_MESSAGE_VOTE "message_vote"
#define MSG_CONTENT_ADD "content_add" //Added in v2.7-alpha
#define MSG_CONTENT_DELETE "content_delete" // Added in v2.7-alpha
#define MSG_CONTENT_EDIT "content_edit" // Added in v2.7-alpha
#define MSG_CONTENT_LIST "content_list" // Added in v2.7-alpha

#define MSG_CHANNEL_ADD "channel_add"
#define MSG_CHANNEL_DELETE "channel_delete"
#define MSG_CHANNEL_EDIT "channel_edit"
#define MSG_CHANNEL_LIST "channel_list"
#define MSG_CHANNEL_SUBSCRIBE "channel_subscribe"
#define MSG_CHANNEL_UNSUBSCRIBE "channel_unsubscribe"
#define MSG_CHANNEL_SEND "channel_send"

#define MSG_USER_ADD "user_add"
#define MSG_USER_DELETE "user_delete"
#define MSG_USER_EDIT "user_edit"
#define MSG_USER_LIST "user_list"
#define MSG_USER_STATS "user_stats" // Deprecated in v2.0, use MSG_SYSTEM_LIST
#define MSG_USER_LOGIN "user_login"
#define MSG_USER_LOGIN_QUERY "user_login_query" // Added in v2.6-beta13
#define MSG_USER_LOGOUT "user_logout"
#define MSG_USER_PAGE "user_page" // Deprecated in v2.6-beta11, use MSG_USER_CONTACT
#define MSG_USER_AGENT "user_agent" // Deprecated in v2.6-beta11, use MSG_USER_CONTACT
#define MSG_USER_CONTACT "user_contact"
#define MSG_USER_SYSOP "user_sysop"
#define MSG_USER_LOGOUT_LIST "user_logout_list"

// Error messages
#define MSG_ACCESS_INVALID "access_invalid"

#define MSG_SYSTEM_CONTACT_INVALID "system_contact_invalid" // Added in v2.6-alpha

#define MSG_CONNECTION_NOT_EXIST "connection_not_exist"

#define MSG_TASK_NOT_EXIST "task_not_exist" // Added in v2.5-beta2

#define MSG_BULLETIN_NOT_EXIST "bulletin_not_exist"
#define MSG_BULLETIN_ACCESS_INVALID "bulletin_access_invalid" // Added in v2.6-alpha

#define MSG_SERVICE_ACCESS_INVALID "service_access_invalid"
#define MSG_SERVICE_NOT_EXIST "service_not_exist"
#define MSG_SERVICE_INVALID "service_invalid"

#define MSG_LOCATION_NOT_EXIST "location_not_exist"
#define MSG_LOCATION_INVALID "location_invalid"

#define MSG_HELP_NOT_EXIST "help_not_exist"

#define MSG_FOLDER_NOT_EXIST "folder_not_exist"
#define MSG_FOLDER_EXIST "folder_exist" // Deprecated in v2.5-beta3
#define MSG_FOLDER_SUBSCRIBED "folder_subscribed" // Deprecated in v2.5-alpha, use MSG_FOLDER_SUBSCRIBE
#define MSG_FOLDER_UNSUBSCRIBED "folder_unsubscribed" // Deprecated in v2.5-alpha, use MSG_FOLDER_SUBSCRIBE
#define MSG_FOLDER_ACCESS_INVALID "folder_access_invalid"
#define MSG_FOLDER_INVALID "folder_invalid"

#define MSG_MESSAGE_NOT_EXIST "message_not_exist"
// #define MSG_MESSAGE_REPLIES_EXCEEDED "message_replies_exceeded" // Deprecated in v2.5-beta3, use MSG_MESSAGE_ACCESS_INVALID
#define MSG_MESSAGE_MOVE_INVALID "message_move_invalid" // Deprecated in v2.5-beta3, use MSG_MESSAGE_ACCESS_INVALID
// #define MSG_MESSAGE_VOTE_INVALID "message_vote_invalid" // Deprecated in v2.5-beta3, use MSG_MESSAGE_ACCESS_INVALID
#define MSG_MESSAGE_ACCESS_INVALID "message_access_invalid"

#define MSG_CONTENT_NOT_EXIST "content_not_exist" // Added in v2.7-alpha
#define MSG_CONTENT_ACCESS_INVALID "content_access_invalid" // Added in v2.7-alpha

#define MSG_CHANNEL_NOT_EXIST "channel_not_exist"
#define MSG_CHANNEL_EXIST "channel_exist"
// #define MSG_CHANNEL_INVALID "channel_invalid" // Deprecated in v2.5-alpha
#define MSG_CHANNEL_ACCESS_INVALID "channel_access_invalid" // Added in v2.6-alpha

#define MSG_USER_NOT_EXIST "user_not_exist"
#define MSG_USER_EXIST "user_exist"
#define MSG_USER_NOT_ON "user_not_on"
#define MSG_USER_BUSY "user_busy"
#define MSG_USER_CONTACT_NOCONTACT "user_contact_nocontact"
#define MSG_USER_LOGIN_INVALID "user_login_invalid"
#define MSG_USER_LOGIN_ALREADY_ON "user_login_already_on"
#define MSG_USER_CONTACT_INVALID "user_contact_invalid"
#define MSG_USER_INVALID "user_invalid"

// Announce messages
#define MSG_CONNECTION_OPEN "connection_open"
#define MSG_CONNECTION_DENIED "connection_denied"

#define MSG_SYSTEM_RELOAD "system_reload"

// #define MSG_CHANNEL_JOIN "channel_join" Deprecated in v2.5-alpha

#define MSG_USER_STATUS "user_status"
#define MSG_USER_LOGIN_DENIED "user_login_denied"

// MessageSpec stop

#define IS_INT_VOTE(x) \
(mask(x, VOTE_INTVALUES) == true || mask(x, VOTE_PERCENT) == true)

#define IS_FLOAT_VOTE(x) \
(mask(x, VOTE_FLOATVALUES) == true || mask(x, VOTE_FLOATPERCENT) == true)

#define IS_VALUE_VOTE(x) \
(IS_INT_VOTE(x) == true || IS_FLOAT_VOTE(x) == true || mask(x, VOTE_STRVALUES) == true)

// Common functions
bool NameValid(const char *szName);
char *AccessName(int iLevel, int iType = -1);

char *SubTypeStr(int iSubType);
int SubTypeInt(const char *szSubType);
// int ProtocolVersion(const char *szVersion);
int ProtocolCompare(const char *szVersion1, const char *szVersion2);

#endif
