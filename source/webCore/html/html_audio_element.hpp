/**
 * @file html_audio_element.hpp
 * @brief HTMLAudioElement - Audio playback
 *
 * Implements the <audio> element per HTML Living Standard.
 * Extends HTMLMediaElement for audio-specific behavior.
 *
 * @see https://html.spec.whatwg.org/multipage/media.html#the-audio-element
 */

#pragma once

#include "html/html_media_element.hpp"
#include <string>
#include <memory>

namespace Zepra::WebCore {

/**
 * @brief HTMLAudioElement - audio playback
 *
 * The <audio> element is used to embed sound content.
 * It inherits most functionality from HTMLMediaElement.
 */
class HTMLAudioElement : public HTMLMediaElement {
public:
    HTMLAudioElement();
    ~HTMLAudioElement() override;

    // Audio element has no additional properties beyond HTMLMediaElement
    // All audio functionality is inherited

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Audio constructor helper
 *
 * Creates an HTMLAudioElement with optional source.
 */
HTMLAudioElement* createAudio(const std::string& src = "");

} // namespace Zepra::WebCore
