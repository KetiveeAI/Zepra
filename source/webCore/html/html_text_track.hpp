/**
 * @file html_text_track.hpp
 * @brief Text Track API for video/audio captions
 */

#pragma once

#include <string>
#include <vector>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief Text track kind
 */
enum class TextTrackKind {
    Subtitles,
    Captions,
    Descriptions,
    Chapters,
    Metadata
};

/**
 * @brief Text track mode
 */
enum class TextTrackMode {
    Disabled,
    Hidden,
    Showing
};

/**
 * @brief VTT cue
 */
class VTTCue {
public:
    VTTCue(double startTime, double endTime, const std::string& text);
    ~VTTCue() = default;
    
    // Timing
    double startTime() const { return startTime_; }
    void setStartTime(double t) { startTime_ = t; }
    
    double endTime() const { return endTime_; }
    void setEndTime(double t) { endTime_ = t; }
    
    bool pauseOnExit() const { return pauseOnExit_; }
    void setPauseOnExit(bool p) { pauseOnExit_ = p; }
    
    // Content
    std::string text() const { return text_; }
    void setText(const std::string& t) { text_ = t; }
    
    // Positioning
    std::string vertical() const { return vertical_; }
    void setVertical(const std::string& v) { vertical_ = v; }
    
    bool snapToLines() const { return snapToLines_; }
    void setSnapToLines(bool s) { snapToLines_ = s; }
    
    double line() const { return line_; }
    void setLine(double l) { line_ = l; }
    
    std::string lineAlign() const { return lineAlign_; }
    void setLineAlign(const std::string& a) { lineAlign_ = a; }
    
    double position() const { return position_; }
    void setPosition(double p) { position_ = p; }
    
    std::string positionAlign() const { return positionAlign_; }
    void setPositionAlign(const std::string& a) { positionAlign_ = a; }
    
    double size() const { return size_; }
    void setSize(double s) { size_ = s; }
    
    std::string align() const { return align_; }
    void setAlign(const std::string& a) { align_ = a; }
    
    // Rendering
    std::string region() const { return region_; }
    void setRegion(const std::string& r) { region_ = r; }
    
    // Events
    std::function<void()> onEnter;
    std::function<void()> onExit;
    
private:
    double startTime_;
    double endTime_;
    std::string text_;
    bool pauseOnExit_ = false;
    std::string vertical_;
    bool snapToLines_ = true;
    double line_ = -1;
    std::string lineAlign_ = "start";
    double position_ = 50;
    std::string positionAlign_ = "auto";
    double size_ = 100;
    std::string align_ = "center";
    std::string region_;
};

/**
 * @brief Text track cue list
 */
class TextTrackCueList {
public:
    size_t length() const { return cues_.size(); }
    VTTCue* operator[](size_t index) { return cues_[index]; }
    VTTCue* getCueById(const std::string& id);
    
    void add(VTTCue* cue) { cues_.push_back(cue); }
    void remove(VTTCue* cue);
    
private:
    std::vector<VTTCue*> cues_;
};

/**
 * @brief Text track
 */
class TextTrack {
public:
    TextTrack(TextTrackKind kind, const std::string& label, 
              const std::string& language);
    ~TextTrack() = default;
    
    // Properties
    TextTrackKind kind() const { return kind_; }
    std::string label() const { return label_; }
    std::string language() const { return language_; }
    std::string id() const { return id_; }
    
    TextTrackMode mode() const { return mode_; }
    void setMode(TextTrackMode m) { mode_ = m; }
    
    // Cues
    TextTrackCueList* cues() { return &cues_; }
    TextTrackCueList* activeCues() { return &activeCues_; }
    
    void addCue(VTTCue* cue);
    void removeCue(VTTCue* cue);
    
    // Events
    std::function<void()> onCueChange;
    
private:
    TextTrackKind kind_;
    std::string label_;
    std::string language_;
    std::string id_;
    TextTrackMode mode_ = TextTrackMode::Disabled;
    TextTrackCueList cues_;
    TextTrackCueList activeCues_;
};

/**
 * @brief Text track list
 */
class TextTrackList {
public:
    size_t length() const { return tracks_.size(); }
    TextTrack* operator[](size_t index) { return tracks_[index]; }
    TextTrack* getTrackById(const std::string& id);
    
    void add(TextTrack* track) { tracks_.push_back(track); }
    void remove(TextTrack* track);
    
    std::function<void()> onAddTrack;
    std::function<void()> onRemoveTrack;
    std::function<void()> onChange;
    
private:
    std::vector<TextTrack*> tracks_;
};

} // namespace Zepra::WebCore
