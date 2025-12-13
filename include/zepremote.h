// ZepRemote - Remote Desktop for ZepraBrowser
// Similar to Chrome Remote Desktop

#ifndef ZEPREMOTE_H
#define ZEPREMOTE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============= TYPES =============

typedef struct ZepRemoteServer ZepRemoteServer;
typedef struct ZepRemoteClient ZepRemoteClient;
typedef struct ZepRemoteSession ZepRemoteSession;

typedef enum {
    ZEPREMOTE_OK = 0,
    ZEPREMOTE_ERROR_NETWORK = 1,
    ZEPREMOTE_ERROR_AUTH = 2,
    ZEPREMOTE_ERROR_TIMEOUT = 3,
    ZEPREMOTE_ERROR_BUSY = 4,
} ZepRemoteError;

typedef enum {
    ZEPREMOTE_QUALITY_LOW = 0,     // 720p, low bitrate
    ZEPREMOTE_QUALITY_MEDIUM = 1,  // 1080p, medium bitrate
    ZEPREMOTE_QUALITY_HIGH = 2,    // 1080p, high bitrate
    ZEPREMOTE_QUALITY_ULTRA = 3,   // 4K, maximum quality
} ZepRemoteQuality;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    ZepRemoteQuality quality;
    bool hardware_acceleration;
    bool enable_audio;
    bool enable_clipboard;
    bool enable_file_transfer;
} ZepRemoteConfig;

typedef struct {
    const char* host;
    uint16_t port;
    const char* access_code;
    const char* session_id;
    bool connected;
    uint32_t latency_ms;
    uint32_t bandwidth_kbps;
} ZepRemoteConnectionInfo;

// Callbacks
typedef void (*ZepRemoteFrameCallback)(const uint8_t* data, uint32_t width, uint32_t height, void* userdata);
typedef void (*ZepRemoteEventCallback)(const char* event_type, const char* event_data, void* userdata);
typedef void (*ZepRemoteConnectionCallback)(bool connected, const char* error, void* userdata);

// ============= SERVER =============

ZepRemoteServer* zepremote_server_create(void);
void zepremote_server_destroy(ZepRemoteServer* server);
ZepRemoteError zepremote_server_start(ZepRemoteServer* server, uint16_t port);
void zepremote_server_stop(ZepRemoteServer* server);
const char* zepremote_server_get_access_code(const ZepRemoteServer* server);
void zepremote_server_set_config(ZepRemoteServer* server, const ZepRemoteConfig* config);
bool zepremote_server_is_running(const ZepRemoteServer* server);

// ============= CLIENT =============

ZepRemoteClient* zepremote_client_create(void);
void zepremote_client_destroy(ZepRemoteClient* client);
ZepRemoteError zepremote_client_connect(ZepRemoteClient* client, const char* host, uint16_t port, const char* access_code);
void zepremote_client_disconnect(ZepRemoteClient* client);
bool zepremote_client_is_connected(const ZepRemoteClient* client);
ZepRemoteConnectionInfo zepremote_client_get_info(const ZepRemoteClient* client);

// Input forwarding
void zepremote_client_send_mouse_move(ZepRemoteClient* client, float x, float y);
void zepremote_client_send_mouse_button(ZepRemoteClient* client, int button, bool pressed);
void zepremote_client_send_mouse_scroll(ZepRemoteClient* client, float delta_x, float delta_y);
void zepremote_client_send_key(ZepRemoteClient* client, uint32_t keycode, bool pressed, uint32_t modifiers);
void zepremote_client_send_touch(ZepRemoteClient* client, uint32_t id, float x, float y, int action);

// Callbacks
void zepremote_client_set_frame_callback(ZepRemoteClient* client, ZepRemoteFrameCallback callback, void* userdata);
void zepremote_client_set_event_callback(ZepRemoteClient* client, ZepRemoteEventCallback callback, void* userdata);
void zepremote_client_set_connection_callback(ZepRemoteClient* client, ZepRemoteConnectionCallback callback, void* userdata);

// ============= SESSION =============

ZepRemoteSession* zepremote_session_create(void);
void zepremote_session_destroy(ZepRemoteSession* session);
const char* zepremote_session_generate_code(ZepRemoteSession* session);
bool zepremote_session_validate_code(ZepRemoteSession* session, const char* code);

#ifdef __cplusplus
}
#endif

#endif // ZEPREMOTE_H
