//! Text Layout
//!
//! Provides text layout capabilities including word wrapping,
//! alignment, and multi-line support.

use crate::primitives::{Point, Rect, Size};
use super::font::{Font, TextMetrics};

/// Text alignment
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum TextAlign {
    #[default]
    Left,
    Center,
    Right,
}

/// Vertical text alignment
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum VerticalAlign {
    #[default]
    Top,
    Middle,
    Bottom,
    Baseline,
}

/// Text overflow handling
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum TextOverflow {
    #[default]
    Clip,
    Ellipsis,
    Visible,
}

/// Text wrapping mode
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum TextWrap {
    #[default]
    None,
    Word,
    Character,
}

/// Text layout options
#[derive(Debug, Clone)]
pub struct TextLayoutOptions {
    /// Maximum width for text (None = unlimited)
    pub max_width: Option<f32>,
    /// Maximum height for text (None = unlimited)
    pub max_height: Option<f32>,
    /// Horizontal alignment
    pub align: TextAlign,
    /// Vertical alignment
    pub vertical_align: VerticalAlign,
    /// Line height multiplier (1.0 = default)
    pub line_height: f32,
    /// Letter spacing in pixels
    pub letter_spacing: f32,
    /// Word spacing in pixels
    pub word_spacing: f32,
    /// Text wrapping mode
    pub wrap: TextWrap,
    /// Overflow handling
    pub overflow: TextOverflow,
}

impl Default for TextLayoutOptions {
    fn default() -> Self {
        Self {
            max_width: None,
            max_height: None,
            align: TextAlign::Left,
            vertical_align: VerticalAlign::Top,
            line_height: 1.2,
            letter_spacing: 0.0,
            word_spacing: 0.0,
            wrap: TextWrap::None,
            overflow: TextOverflow::Clip,
        }
    }
}

/// A positioned glyph in the layout
#[derive(Debug, Clone)]
pub struct LayoutGlyph {
    /// The character
    pub character: char,
    /// X position
    pub x: f32,
    /// Y position (baseline)
    pub y: f32,
    /// Line index
    pub line: usize,
}

/// A line of text in the layout
#[derive(Debug, Clone)]
pub struct LayoutLine {
    /// Start index in the original text
    pub start: usize,
    /// End index in the original text
    pub end: usize,
    /// Y position of this line
    pub y: f32,
    /// Width of this line
    pub width: f32,
    /// Height of this line
    pub height: f32,
}

/// Result of text layout
#[derive(Debug, Clone)]
pub struct TextLayout {
    /// Positioned glyphs
    pub glyphs: Vec<LayoutGlyph>,
    /// Lines
    pub lines: Vec<LayoutLine>,
    /// Total width
    pub width: f32,
    /// Total height
    pub height: f32,
    /// Whether text was truncated
    pub truncated: bool,
}

impl TextLayout {
    /// Create a new text layout
    pub fn new(
        text: &str,
        font: &Font,
        size: f32,
        options: &TextLayoutOptions,
    ) -> Self {
        let metrics = font.metrics(size);
        let line_height = metrics.line_height * options.line_height;
        
        let mut glyphs: Vec<LayoutGlyph> = Vec::new();
        let mut lines: Vec<LayoutLine> = Vec::new();
        let mut current_line_start = 0;
        let mut current_line_width = 0.0f32;
        let mut x = 0.0f32;
        let mut y = metrics.ascent;
        let mut max_width = 0.0f32;
        let mut truncated = false;
        
        let max_w = options.max_width.unwrap_or(f32::MAX);
        let max_h = options.max_height.unwrap_or(f32::MAX);
        
        let chars: Vec<char> = text.chars().collect();
        let mut i = 0;
        
        while i < chars.len() {
            let c = chars[i];
            
            // Check for newline
            if c == '\n' {
                // End current line
                lines.push(LayoutLine {
                    start: current_line_start,
                    end: i,
                    y: y - metrics.ascent,
                    width: current_line_width,
                    height: line_height,
                });
                
                max_width = max_width.max(current_line_width);
                
                // Start new line
                x = 0.0;
                y += line_height;
                current_line_start = i + 1;
                current_line_width = 0.0;
                
                // Check height limit
                if y > max_h {
                    truncated = true;
                    break;
                }
                
                i += 1;
                continue;
            }
            
            // Measure character
            let char_metrics = font.measure_text(&c.to_string(), size);
            let char_width = char_metrics.width + options.letter_spacing;
            
            // Extra space for space characters
            let extra_space = if c == ' ' { options.word_spacing } else { 0.0 };
            let total_width = char_width + extra_space;
            
            // Check if we need to wrap
            let should_wrap = options.wrap != TextWrap::None
                && x + total_width > max_w
                && x > 0.0;
            
            if should_wrap {
                // Handle word wrap
                if options.wrap == TextWrap::Word {
                    // Find word start
                    let mut word_start = glyphs.len();
                    while word_start > 0 && glyphs[word_start - 1].character != ' ' {
                        word_start -= 1;
                    }
                    
                    if word_start > 0 && word_start < glyphs.len() {
                        // Move word to next line
                        let word_x = glyphs[word_start].x;
                        
                        // Adjust line end
                        current_line_width = word_x;
                        
                        lines.push(LayoutLine {
                            start: current_line_start,
                            end: word_start + current_line_start,
                            y: y - metrics.ascent,
                            width: current_line_width,
                            height: line_height,
                        });
                        
                        max_width = max_width.max(current_line_width);
                        y += line_height;
                        
                        // Reposition word glyphs
                        x = 0.0;
                        for g in &mut glyphs[word_start..] {
                            g.x = x;
                            g.y = y;
                            g.line = lines.len();
                            x += font.measure_text(&g.character.to_string(), size).width
                                + options.letter_spacing;
                        }
                        
                        current_line_start = word_start + current_line_start;
                        current_line_width = x;
                    }
                } else {
                    // Character wrap
                    lines.push(LayoutLine {
                        start: current_line_start,
                        end: i,
                        y: y - metrics.ascent,
                        width: current_line_width,
                        height: line_height,
                    });
                    
                    max_width = max_width.max(current_line_width);
                    x = 0.0;
                    y += line_height;
                    current_line_start = i;
                    current_line_width = 0.0;
                }
                
                // Check height limit
                if y > max_h {
                    truncated = true;
                    break;
                }
            }
            
            // Add glyph
            glyphs.push(LayoutGlyph {
                character: c,
                x,
                y,
                line: lines.len(),
            });
            
            x += total_width;
            current_line_width = x;
            i += 1;
        }
        
        // Add final line
        if current_line_start <= chars.len() && !truncated {
            lines.push(LayoutLine {
                start: current_line_start,
                end: chars.len(),
                y: y - metrics.ascent,
                width: current_line_width,
                height: line_height,
            });
            max_width = max_width.max(current_line_width);
        }
        
        let total_height = if lines.is_empty() {
            0.0
        } else {
            lines.last().unwrap().y + line_height
        };
        
        // Apply horizontal alignment
        for line in &lines {
            let offset = match options.align {
                TextAlign::Left => 0.0,
                TextAlign::Center => (max_width - line.width) / 2.0,
                TextAlign::Right => max_width - line.width,
            };
            
            if offset != 0.0 {
                for glyph in &mut glyphs {
                    if glyph.line == lines.iter().position(|l| l.start == line.start).unwrap_or(0) {
                        glyph.x += offset;
                    }
                }
            }
        }
        
        Self {
            glyphs,
            lines,
            width: max_width,
            height: total_height,
            truncated,
        }
    }
    
    /// Get the bounding box of the layout
    pub fn bounds(&self) -> Rect {
        Rect::new(0.0, 0.0, self.width, self.height)
    }
    
    /// Check if the layout is empty
    pub fn is_empty(&self) -> bool {
        self.glyphs.is_empty()
    }
    
    /// Get number of lines
    pub fn line_count(&self) -> usize {
        self.lines.len()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_text_layout_options_default() {
        let opts = TextLayoutOptions::default();
        assert_eq!(opts.align, TextAlign::Left);
        assert_eq!(opts.wrap, TextWrap::None);
        assert_eq!(opts.line_height, 1.2);
    }
}
