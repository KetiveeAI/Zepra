//! Text Shaping
//!
//! Basic text shaping. For complex scripts, HarfBuzz integration
//! would be added here.

use unicode_normalization::UnicodeNormalization;

/// Shape text for rendering
/// 
/// This performs basic shaping including:
/// - Unicode normalization (NFC)
/// - Basic ligature support (future)
/// - RTL/LTR detection (future)
pub fn shape_text(text: &str) -> String {
    // Normalize to NFC (composed form)
    text.nfc().collect()
}

/// Text direction
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum TextDirection {
    #[default]
    LeftToRight,
    RightToLeft,
    TopToBottom,
}

/// Detect the primary text direction
pub fn detect_direction(text: &str) -> TextDirection {
    // Simple heuristic: check first strong character
    for c in text.chars() {
        if is_rtl_char(c) {
            return TextDirection::RightToLeft;
        }
        if c.is_alphabetic() {
            return TextDirection::LeftToRight;
        }
    }
    TextDirection::LeftToRight
}

fn is_rtl_char(c: char) -> bool {
    // Arabic, Hebrew, and other RTL ranges
    matches!(c,
        '\u{0590}'..='\u{05FF}' |  // Hebrew
        '\u{0600}'..='\u{06FF}' |  // Arabic
        '\u{0700}'..='\u{074F}' |  // Syriac
        '\u{0750}'..='\u{077F}' |  // Arabic Supplement
        '\u{08A0}'..='\u{08FF}' |  // Arabic Extended-A
        '\u{FB50}'..='\u{FDFF}' |  // Arabic Presentation Forms-A
        '\u{FE70}'..='\u{FEFF}'    // Arabic Presentation Forms-B
    )
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_shape_text() {
        let text = "café";
        let shaped = shape_text(text);
        assert!(!shaped.is_empty());
    }
    
    #[test]
    fn test_detect_direction_ltr() {
        assert_eq!(detect_direction("Hello"), TextDirection::LeftToRight);
    }
    
    #[test]
    fn test_detect_direction_rtl() {
        assert_eq!(detect_direction("שלום"), TextDirection::RightToLeft);
    }
}
