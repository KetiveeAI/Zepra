/**
 * @file IntlSegmenterAPI.h
 * @brief Intl.Segmenter Implementation
 */

#pragma once

#include <string>
#include <vector>
#include <optional>

namespace Zepra::Runtime {

// =============================================================================
// Segment
// =============================================================================

struct Segment {
    std::string segment;
    size_t index;
    std::string input;
    bool isWordLike;
};

// =============================================================================
// Segmenter Granularity
// =============================================================================

enum class SegmenterGranularity { Grapheme, Word, Sentence };

// =============================================================================
// Segmenter Options
// =============================================================================

struct SegmenterOptions {
    std::string locale = "en";
    SegmenterGranularity granularity = SegmenterGranularity::Grapheme;
};

// =============================================================================
// Segments Iterator
// =============================================================================

class Segments {
public:
    Segments(const std::string& input, SegmenterGranularity granularity)
        : input_(input), granularity_(granularity), position_(0) {
        computeSegments();
    }
    
    std::optional<Segment> next() {
        if (position_ >= segments_.size()) return std::nullopt;
        return segments_[position_++];
    }
    
    const std::vector<Segment>& all() const { return segments_; }
    
    std::optional<Segment> containing(size_t index) const {
        for (const auto& seg : segments_) {
            if (index >= seg.index && index < seg.index + seg.segment.length()) {
                return seg;
            }
        }
        return std::nullopt;
    }

private:
    void computeSegments() {
        switch (granularity_) {
            case SegmenterGranularity::Grapheme:
                segmentByGrapheme();
                break;
            case SegmenterGranularity::Word:
                segmentByWord();
                break;
            case SegmenterGranularity::Sentence:
                segmentBySentence();
                break;
        }
    }
    
    void segmentByGrapheme() {
        for (size_t i = 0; i < input_.length(); ++i) {
            Segment seg;
            seg.segment = std::string(1, input_[i]);
            seg.index = i;
            seg.input = input_;
            seg.isWordLike = false;
            segments_.push_back(seg);
        }
    }
    
    void segmentByWord() {
        size_t start = 0;
        bool inWord = false;
        
        for (size_t i = 0; i <= input_.length(); ++i) {
            bool isWordChar = i < input_.length() && (std::isalnum(input_[i]) || input_[i] == '\'');
            
            if (isWordChar && !inWord) {
                start = i;
                inWord = true;
            } else if (!isWordChar && inWord) {
                Segment seg;
                seg.segment = input_.substr(start, i - start);
                seg.index = start;
                seg.input = input_;
                seg.isWordLike = true;
                segments_.push_back(seg);
                inWord = false;
            }
            
            if (!isWordChar && i < input_.length()) {
                Segment seg;
                seg.segment = std::string(1, input_[i]);
                seg.index = i;
                seg.input = input_;
                seg.isWordLike = false;
                segments_.push_back(seg);
            }
        }
    }
    
    void segmentBySentence() {
        size_t start = 0;
        
        for (size_t i = 0; i < input_.length(); ++i) {
            if (input_[i] == '.' || input_[i] == '!' || input_[i] == '?') {
                size_t end = i + 1;
                while (end < input_.length() && std::isspace(input_[end])) ++end;
                
                Segment seg;
                seg.segment = input_.substr(start, end - start);
                seg.index = start;
                seg.input = input_;
                seg.isWordLike = false;
                segments_.push_back(seg);
                
                start = end;
                i = end - 1;
            }
        }
        
        if (start < input_.length()) {
            Segment seg;
            seg.segment = input_.substr(start);
            seg.index = start;
            seg.input = input_;
            seg.isWordLike = false;
            segments_.push_back(seg);
        }
    }
    
    std::string input_;
    SegmenterGranularity granularity_;
    std::vector<Segment> segments_;
    size_t position_;
};

// =============================================================================
// Intl.Segmenter
// =============================================================================

class Segmenter {
public:
    Segmenter(const std::string& locale = "en", SegmenterOptions options = {})
        : locale_(locale), options_(std::move(options)) {}
    
    Segments segment(const std::string& input) const {
        return Segments(input, options_.granularity);
    }
    
    struct ResolvedOptions {
        std::string locale;
        std::string granularity;
    };
    
    ResolvedOptions resolvedOptions() const {
        ResolvedOptions opts;
        opts.locale = locale_;
        switch (options_.granularity) {
            case SegmenterGranularity::Grapheme: opts.granularity = "grapheme"; break;
            case SegmenterGranularity::Word: opts.granularity = "word"; break;
            case SegmenterGranularity::Sentence: opts.granularity = "sentence"; break;
        }
        return opts;
    }
    
    static std::vector<std::string> supportedLocales() {
        return {"en", "en-US", "es", "fr", "de", "ja", "zh", "ko"};
    }

private:
    std::string locale_;
    SegmenterOptions options_;
};

} // namespace Zepra::Runtime
