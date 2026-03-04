/**
 * @file video_bindings.cpp
 * @brief JavaScript bindings for NXVideo via ZebraScript
 * 
 * Exposes video playback API to JavaScript running in the browser.
 * Uses ZebraScript's native binding system.
 */

#include "nxvideo/nxvideo_cpp.hpp"
#include "runtime/objects/value.hpp"
#include "runtime/objects/object.hpp"
#include "runtime/objects/function.hpp"
#include "runtime/context.hpp"
#include "host/native_function.hpp"

#include <unordered_map>
#include <memory>
#include <iostream>

namespace Zepra::Browser {

// =============================================================================
// Video Player JavaScript Object
// =============================================================================

class JSVideoPlayer {
public:
    explicit JSVideoPlayer(const nxvideo::Player::Config& config = {})
        : player_(config) {}
    
    nxvideo::Player& player() { return player_; }
    const nxvideo::Player& player() const { return player_; }
    
private:
    nxvideo::Player player_;
};

// Store active players by ID
static std::unordered_map<int, std::unique_ptr<JSVideoPlayer>> g_players;
static int g_nextPlayerId = 1;

// =============================================================================
// Native Functions
// =============================================================================

// -----------------------------------------------------------------------------
// Video.create(config?) -> playerId
// -----------------------------------------------------------------------------
Runtime::Value js_video_create(Runtime::Context& ctx, 
                                const std::vector<Runtime::Value>& args) {
    nxvideo::Player::Config config;
    
    if (!args.empty() && args[0].isObject()) {
        auto* obj = args[0].asObject();
        if (obj->has("hwDecode")) {
            config.hwDecode = obj->get("hwDecode").toBoolean();
        }
        if (obj->has("loop")) {
            config.loop = obj->get("loop").toBoolean();
        }
        if (obj->has("volume")) {
            config.volume = static_cast<float>(obj->get("volume").toNumber());
        }
        if (obj->has("playbackRate")) {
            config.playbackRate = static_cast<float>(obj->get("playbackRate").toNumber());
        }
        if (obj->has("muted")) {
            config.muted = obj->get("muted").toBoolean();
        }
    }
    
    try {
        int id = g_nextPlayerId++;
        g_players[id] = std::make_unique<JSVideoPlayer>(config);
        return Runtime::Value(static_cast<double>(id));
    } catch (const nxvideo::VideoException& e) {
        ctx.throwError("VideoError", e.what());
        return Runtime::Value::undefined();
    }
}

// -----------------------------------------------------------------------------
// Video.destroy(playerId)
// -----------------------------------------------------------------------------
Runtime::Value js_video_destroy(Runtime::Context& ctx,
                                 const std::vector<Runtime::Value>& args) {
    if (args.empty()) return Runtime::Value::undefined();
    
    int id = static_cast<int>(args[0].toNumber());
    g_players.erase(id);
    
    return Runtime::Value::undefined();
}

// -----------------------------------------------------------------------------
// Video.open(playerId, url)
// -----------------------------------------------------------------------------
Runtime::Value js_video_open(Runtime::Context& ctx,
                              const std::vector<Runtime::Value>& args) {
    if (args.size() < 2) return Runtime::Value(false);
    
    int id = static_cast<int>(args[0].toNumber());
    std::string url = args[1].toString();
    
    auto it = g_players.find(id);
    if (it == g_players.end()) {
        ctx.throwError("VideoError", "Invalid player ID");
        return Runtime::Value(false);
    }
    
    try {
        it->second->player().open(url);
        return Runtime::Value(true);
    } catch (const nxvideo::VideoException& e) {
        ctx.throwError("VideoError", e.what());
        return Runtime::Value(false);
    }
}

// -----------------------------------------------------------------------------
// Video.play(playerId)
// -----------------------------------------------------------------------------
Runtime::Value js_video_play(Runtime::Context& ctx,
                              const std::vector<Runtime::Value>& args) {
    if (args.empty()) return Runtime::Value::undefined();
    
    int id = static_cast<int>(args[0].toNumber());
    auto it = g_players.find(id);
    if (it == g_players.end()) return Runtime::Value::undefined();
    
    try {
        it->second->player().play();
    } catch (const nxvideo::VideoException& e) {
        ctx.throwError("VideoError", e.what());
    }
    
    return Runtime::Value::undefined();
}

// -----------------------------------------------------------------------------
// Video.pause(playerId)
// -----------------------------------------------------------------------------
Runtime::Value js_video_pause(Runtime::Context& ctx,
                               const std::vector<Runtime::Value>& args) {
    if (args.empty()) return Runtime::Value::undefined();
    
    int id = static_cast<int>(args[0].toNumber());
    auto it = g_players.find(id);
    if (it == g_players.end()) return Runtime::Value::undefined();
    
    try {
        it->second->player().pause();
    } catch (const nxvideo::VideoException& e) {
        ctx.throwError("VideoError", e.what());
    }
    
    return Runtime::Value::undefined();
}

// -----------------------------------------------------------------------------
// Video.stop(playerId)
// -----------------------------------------------------------------------------
Runtime::Value js_video_stop(Runtime::Context& ctx,
                              const std::vector<Runtime::Value>& args) {
    if (args.empty()) return Runtime::Value::undefined();
    
    int id = static_cast<int>(args[0].toNumber());
    auto it = g_players.find(id);
    if (it == g_players.end()) return Runtime::Value::undefined();
    
    try {
        it->second->player().stop();
    } catch (const nxvideo::VideoException& e) {
        ctx.throwError("VideoError", e.what());
    }
    
    return Runtime::Value::undefined();
}

// -----------------------------------------------------------------------------
// Video.seek(playerId, seconds)
// -----------------------------------------------------------------------------
Runtime::Value js_video_seek(Runtime::Context& ctx,
                              const std::vector<Runtime::Value>& args) {
    if (args.size() < 2) return Runtime::Value::undefined();
    
    int id = static_cast<int>(args[0].toNumber());
    double seconds = args[1].toNumber();
    
    auto it = g_players.find(id);
    if (it == g_players.end()) return Runtime::Value::undefined();
    
    try {
        it->second->player().seek(seconds);
    } catch (const nxvideo::VideoException& e) {
        ctx.throwError("VideoError", e.what());
    }
    
    return Runtime::Value::undefined();
}

// -----------------------------------------------------------------------------
// Video.position(playerId) -> number
// -----------------------------------------------------------------------------
Runtime::Value js_video_position(Runtime::Context& ctx,
                                  const std::vector<Runtime::Value>& args) {
    if (args.empty()) return Runtime::Value(0.0);
    
    int id = static_cast<int>(args[0].toNumber());
    auto it = g_players.find(id);
    if (it == g_players.end()) return Runtime::Value(0.0);
    
    return Runtime::Value(it->second->player().position());
}

// -----------------------------------------------------------------------------
// Video.duration(playerId) -> number
// -----------------------------------------------------------------------------
Runtime::Value js_video_duration(Runtime::Context& ctx,
                                  const std::vector<Runtime::Value>& args) {
    if (args.empty()) return Runtime::Value(0.0);
    
    int id = static_cast<int>(args[0].toNumber());
    auto it = g_players.find(id);
    if (it == g_players.end()) return Runtime::Value(0.0);
    
    return Runtime::Value(it->second->player().duration());
}

// -----------------------------------------------------------------------------
// Video.state(playerId) -> string
// -----------------------------------------------------------------------------
Runtime::Value js_video_state(Runtime::Context& ctx,
                               const std::vector<Runtime::Value>& args) {
    if (args.empty()) return Runtime::Value("unknown");
    
    int id = static_cast<int>(args[0].toNumber());
    auto it = g_players.find(id);
    if (it == g_players.end()) return Runtime::Value("unknown");
    
    auto state = it->second->player().state();
    switch (state) {
        case nxvideo::State::Stopped:   return Runtime::Value("stopped");
        case nxvideo::State::Playing:   return Runtime::Value("playing");
        case nxvideo::State::Paused:    return Runtime::Value("paused");
        case nxvideo::State::Buffering: return Runtime::Value("buffering");
        case nxvideo::State::Seeking:   return Runtime::Value("seeking");
        case nxvideo::State::Ended:     return Runtime::Value("ended");
        case nxvideo::State::Error:     return Runtime::Value("error");
    }
    return Runtime::Value("unknown");
}

// -----------------------------------------------------------------------------
// Video.setVolume(playerId, volume)
// -----------------------------------------------------------------------------
Runtime::Value js_video_set_volume(Runtime::Context& ctx,
                                    const std::vector<Runtime::Value>& args) {
    if (args.size() < 2) return Runtime::Value::undefined();
    
    int id = static_cast<int>(args[0].toNumber());
    float volume = static_cast<float>(args[1].toNumber());
    
    auto it = g_players.find(id);
    if (it != g_players.end()) {
        it->second->player().setVolume(volume);
    }
    
    return Runtime::Value::undefined();
}

// -----------------------------------------------------------------------------
// Video.setMuted(playerId, muted)
// -----------------------------------------------------------------------------
Runtime::Value js_video_set_muted(Runtime::Context& ctx,
                                   const std::vector<Runtime::Value>& args) {
    if (args.size() < 2) return Runtime::Value::undefined();
    
    int id = static_cast<int>(args[0].toNumber());
    bool muted = args[1].toBoolean();
    
    auto it = g_players.find(id);
    if (it != g_players.end()) {
        it->second->player().setMuted(muted);
    }
    
    return Runtime::Value::undefined();
}

// -----------------------------------------------------------------------------
// Video.setPlaybackRate(playerId, rate)
// -----------------------------------------------------------------------------
Runtime::Value js_video_set_rate(Runtime::Context& ctx,
                                  const std::vector<Runtime::Value>& args) {
    if (args.size() < 2) return Runtime::Value::undefined();
    
    int id = static_cast<int>(args[0].toNumber());
    float rate = static_cast<float>(args[1].toNumber());
    
    auto it = g_players.find(id);
    if (it != g_players.end()) {
        it->second->player().setPlaybackRate(rate);
    }
    
    return Runtime::Value::undefined();
}

// -----------------------------------------------------------------------------
// Video.info(playerId) -> object
// -----------------------------------------------------------------------------
Runtime::Value js_video_info(Runtime::Context& ctx,
                              const std::vector<Runtime::Value>& args) {
    if (args.empty()) return Runtime::Value::null();
    
    int id = static_cast<int>(args[0].toNumber());
    auto it = g_players.find(id);
    if (it == g_players.end()) return Runtime::Value::null();
    
    try {
        auto info = it->second->player().getInfo();
        
        // Create JS object
        auto* obj = ctx.createObject();
        obj->set("hasVideo", Runtime::Value(info.hasVideo));
        obj->set("hasAudio", Runtime::Value(info.hasAudio));
        obj->set("duration", Runtime::Value(info.duration));
        obj->set("container", Runtime::Value(info.container));
        obj->set("title", Runtime::Value(info.title));
        
        if (info.hasVideo) {
            auto* video = ctx.createObject();
            video->set("width", Runtime::Value(static_cast<double>(info.video.width)));
            video->set("height", Runtime::Value(static_cast<double>(info.video.height)));
            video->set("fps", Runtime::Value(info.video.fps));
            video->set("codec", Runtime::Value(info.video.codecName));
            obj->set("video", Runtime::Value(video));
        }
        
        if (info.hasAudio) {
            auto* audio = ctx.createObject();
            audio->set("sampleRate", Runtime::Value(static_cast<double>(info.audio.sampleRate)));
            audio->set("channels", Runtime::Value(static_cast<double>(info.audio.channels)));
            audio->set("codec", Runtime::Value(info.audio.codecName));
            obj->set("audio", Runtime::Value(audio));
        }
        
        return Runtime::Value(obj);
    } catch (const nxvideo::VideoException& e) {
        return Runtime::Value::null();
    }
}

// -----------------------------------------------------------------------------
// Video.hwAvailable() -> object
// -----------------------------------------------------------------------------
Runtime::Value js_video_hw_available(Runtime::Context& ctx,
                                       const std::vector<Runtime::Value>& args) {
    auto* obj = ctx.createObject();
    
    obj->set("h264", Runtime::Value(nxvideo::System::isHardwareAvailable(nxvideo::Codec::H264)));
    obj->set("h265", Runtime::Value(nxvideo::System::isHardwareAvailable(nxvideo::Codec::H265)));
    obj->set("vp9", Runtime::Value(nxvideo::System::isHardwareAvailable(nxvideo::Codec::VP9)));
    obj->set("av1", Runtime::Value(nxvideo::System::isHardwareAvailable(nxvideo::Codec::AV1)));
    
    return Runtime::Value(obj);
}

// =============================================================================
// Register Bindings
// =============================================================================

void registerVideoBindings(Runtime::Context& ctx) {
    // Initialize NXVideo
    try {
        nxvideo::getVideoSystem();
    } catch (const nxvideo::VideoException& e) {
        std::cerr << "[VideoBindings] Failed to init NXVideo: " << e.what() << std::endl;
    }
    
    // Create Video namespace object
    auto* videoNS = ctx.createObject();
    
    // Register functions
    videoNS->set("create", ctx.createNativeFunction(js_video_create));
    videoNS->set("destroy", ctx.createNativeFunction(js_video_destroy));
    videoNS->set("open", ctx.createNativeFunction(js_video_open));
    videoNS->set("play", ctx.createNativeFunction(js_video_play));
    videoNS->set("pause", ctx.createNativeFunction(js_video_pause));
    videoNS->set("stop", ctx.createNativeFunction(js_video_stop));
    videoNS->set("seek", ctx.createNativeFunction(js_video_seek));
    videoNS->set("position", ctx.createNativeFunction(js_video_position));
    videoNS->set("duration", ctx.createNativeFunction(js_video_duration));
    videoNS->set("state", ctx.createNativeFunction(js_video_state));
    videoNS->set("setVolume", ctx.createNativeFunction(js_video_set_volume));
    videoNS->set("setMuted", ctx.createNativeFunction(js_video_set_muted));
    videoNS->set("setPlaybackRate", ctx.createNativeFunction(js_video_set_rate));
    videoNS->set("info", ctx.createNativeFunction(js_video_info));
    videoNS->set("hwAvailable", ctx.createNativeFunction(js_video_hw_available));
    
    // Register in global scope
    ctx.global()->set("Video", Runtime::Value(videoNS));
    
    std::cout << "[VideoBindings] Registered Video API" << std::endl;
}

} // namespace Zepra::Browser
