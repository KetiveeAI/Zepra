/**
 * @file voice_input.h
 * @brief Voice listening and speech-to-text integration
 */

#ifndef ZEPRA_INPUT_VOICE_INPUT_H
#define ZEPRA_INPUT_VOICE_INPUT_H

#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace zepra {
namespace input {

/**
 * Voice input state
 */
enum class VoiceState {
    Idle,
    Listening,
    Processing,
    Error
};

/**
 * Speech recognition result
 */
struct VoiceResult {
    std::string text;           // Recognized text
    float confidence;           // 0.0 to 1.0
    bool isFinal;               // Final result vs interim
    std::string language;       // Detected language
};

/**
 * Voice input configuration
 */
struct VoiceConfig {
    std::string language = "en-US";
    float silenceTimeout = 2.0f;    // Seconds of silence before stopping
    float maxDuration = 30.0f;      // Maximum recording duration
    bool continuous = false;         // Keep listening after result
    bool interimResults = true;      // Provide partial results
};

/**
 * Audio level data
 */
struct AudioLevel {
    float level;                // 0.0 to 1.0
    uint32_t timestamp;
};

/**
 * VoiceInput - Speech-to-text system
 * 
 * Features:
 * - Start/stop voice recording
 * - Speech recognition
 * - Audio level monitoring
 * - Multiple language support
 * 
 * Note: This is a placeholder interface. Actual implementation
 * requires integration with a speech recognition backend like
 * Vosk, Google Speech API, or system speech services.
 */
class VoiceInput {
public:
    using ResultCallback = std::function<void(const VoiceResult& result)>;
    using StateCallback = std::function<void(VoiceState state)>;
    using LevelCallback = std::function<void(const AudioLevel& level)>;
    using ErrorCallback = std::function<void(const std::string& error)>;

    VoiceInput();
    explicit VoiceInput(const VoiceConfig& config);
    ~VoiceInput();

    // Configuration
    void setConfig(const VoiceConfig& config);
    VoiceConfig getConfig() const;

    // Availability
    static bool isAvailable();
    static std::vector<std::string> getAvailableLanguages();
    static bool requestPermission();
    static bool hasPermission();

    // Control
    bool start();
    void stop();
    void cancel();
    
    // State
    VoiceState getState() const;
    bool isListening() const;
    bool isProcessing() const;

    // Callbacks
    void setResultCallback(ResultCallback callback);
    void setStateCallback(StateCallback callback);
    void setLevelCallback(LevelCallback callback);
    void setErrorCallback(ErrorCallback callback);

    // Update (call each frame for level updates)
    void update();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace input
} // namespace zepra

#endif // ZEPRA_INPUT_VOICE_INPUT_H
