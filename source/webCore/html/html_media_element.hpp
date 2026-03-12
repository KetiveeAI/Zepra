/**
 * @file html_media_element.hpp
 * @brief HTMLMediaElement interface - base for audio/video
 *
 * Implements media playback functionality per HTML Living Standard.
 *
 * @see https://developer.mozilla.org/en-US/docs/Web/API/HTMLMediaElement
 * @see https://html.spec.whatwg.org/multipage/media.html
 */

#pragma once

#include "html/html_element.hpp"
#include <vector>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief Network state for media loading
 */
enum class MediaNetworkState : unsigned short {
    Empty = 0,      ///< No data yet
    Idle = 1,       ///< Resource selected, not using network
    Loading = 2,    ///< Still downloading data
    NoSource = 3    ///< No source found
};

/**
 * @brief Ready state for media playback
 */
enum class MediaReadyState : unsigned short {
    HaveNothing = 0,        ///< No data available
    HaveMetadata = 1,       ///< Metadata loaded
    HaveCurrentData = 2,    ///< Current frame available
    HaveFutureData = 3,     ///< Future frames available
    HaveEnoughData = 4      ///< Enough data to play
};

/**
 * @brief Preload mode
 */
enum class MediaPreload {
    None,       ///< Don't preload
    Metadata,   ///< Preload metadata only
    Auto        ///< Let browser decide
};

/**
 * @brief Time range structure
 */
struct TimeRange {
    double start;
    double end;
};

/**
 * @brief TimeRanges collection
 */
class TimeRanges {
public:
    TimeRanges() = default;
    
    /// Number of ranges
    size_t length() const { return ranges_.size(); }
    
    /// Get start of range at index
    double start(size_t index) const {
        return index < ranges_.size() ? ranges_[index].start : 0;
    }
    
    /// Get end of range at index
    double end(size_t index) const {
        return index < ranges_.size() ? ranges_[index].end : 0;
    }
    
    /// Add a range
    void addRange(double start, double end) {
        ranges_.push_back({start, end});
    }
    
private:
    std::vector<TimeRange> ranges_;
};

/**
 * @brief Media error information
 */
class MediaError {
public:
    enum Code : unsigned short {
        Aborted = 1,        ///< Fetching aborted by user
        Network = 2,        ///< Network error
        Decode = 3,         ///< Decoding error
        SrcNotSupported = 4 ///< Source not supported
    };
    
    MediaError(Code code, const std::string& message = "")
        : code_(code), message_(message) {}
    
    Code code() const { return code_; }
    std::string message() const { return message_; }
    
private:
    Code code_;
    std::string message_;
};

/**
 * @brief HTMLMediaElement - base for audio and video elements
 *
 * Provides common media playback functionality.
 */
class HTMLMediaElement : public HTMLElement {
public:
    explicit HTMLMediaElement(const std::string& tagName);
    ~HTMLMediaElement() override;

    // =========================================================================
    // Source Properties
    // =========================================================================

    /// Media source URL
    std::string src() const;
    void setSrc(const std::string& src);

    /// Current source URL (from src or <source>)
    std::string currentSrc() const;

    /// Cross-origin setting
    std::string crossOrigin() const;
    void setCrossOrigin(const std::string& mode);

    // =========================================================================
    // Playback State
    // =========================================================================

    /// Current playback position in seconds
    double currentTime() const;
    void setCurrentTime(double time);

    /// Duration of the media in seconds
    double duration() const;

    /// Whether playback has ended
    bool ended() const;

    /// Whether playback is paused
    bool paused() const;

    /// Whether media is seeking
    bool seeking() const;

    // =========================================================================
    // Network State
    // =========================================================================

    /// Current network state
    MediaNetworkState networkState() const;

    /// Current ready state
    MediaReadyState readyState() const;

    /// Buffered time ranges
    TimeRanges* buffered();
    const TimeRanges* buffered() const;

    /// Played time ranges
    TimeRanges* played();
    const TimeRanges* played() const;

    /// Seekable time ranges
    TimeRanges* seekable();
    const TimeRanges* seekable() const;

    /// Preload setting
    std::string preload() const;
    void setPreload(const std::string& preload);

    // =========================================================================
    // Playback Control
    // =========================================================================

    /// Whether to autoplay
    bool autoplay() const;
    void setAutoplay(bool autoplay);

    /// Whether to loop
    bool loop() const;
    void setLoop(bool loop);

    /// Whether to show controls
    bool controls() const;
    void setControls(bool controls);

    /// Controls list (tokens)
    DOMTokenList* controlsList();

    /// Current volume (0.0 to 1.0)
    double volume() const;
    void setVolume(double volume);

    /// Whether audio is muted
    bool muted() const;
    void setMuted(bool muted);

    /// Default muted state
    bool defaultMuted() const;
    void setDefaultMuted(bool muted);

    /// Playback rate (1.0 = normal)
    double playbackRate() const;
    void setPlaybackRate(double rate);

    /// Default playback rate
    double defaultPlaybackRate() const;
    void setDefaultPlaybackRate(double rate);

    /// Whether to preserve pitch when rate changes
    bool preservesPitch() const;
    void setPreservesPitch(bool preserve);

    // =========================================================================
    // Error Handling
    // =========================================================================

    /// Last error (or nullptr)
    const MediaError* error() const;

    // =========================================================================
    // Methods
    // =========================================================================

    /// Start or resume playback
    void play();

    /// Pause playback
    void pause();

    /// Reload the media
    void load();

    /// Fast seek to time
    void fastSeek(double time);

    /// Check if media type can be played
    /// Returns "probably", "maybe", or ""
    std::string canPlayType(const std::string& mediaType) const;

    // =========================================================================
    // Event Handlers
    // =========================================================================

    void setOnPlay(EventListener callback);
    void setOnPause(EventListener callback);
    void setOnPlaying(EventListener callback);
    void setOnEnded(EventListener callback);
    void setOnTimeUpdate(EventListener callback);
    void setOnDurationChange(EventListener callback);
    void setOnVolumeChange(EventListener callback);
    void setOnLoadStart(EventListener callback);
    void setOnProgress(EventListener callback);
    void setOnSuspend(EventListener callback);
    void setOnAbort(EventListener callback);
    void setOnError(EventListener callback);
    void setOnEmptied(EventListener callback);
    void setOnStalled(EventListener callback);
    void setOnLoadedMetadata(EventListener callback);
    void setOnLoadedData(EventListener callback);
    void setOnCanPlay(EventListener callback);
    void setOnCanPlayThrough(EventListener callback);
    void setOnSeeking(EventListener callback);
    void setOnSeeked(EventListener callback);
    void setOnWaiting(EventListener callback);
    void setOnRateChange(EventListener callback);

protected:
    /// Copy media properties to clone
    void copyMediaElementProperties(HTMLMediaElement* target) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// Note: HTMLVideoElement is defined in html_video_element.hpp
// Note: HTMLAudioElement is defined in html_audio_element.hpp

} // namespace Zepra::WebCore

