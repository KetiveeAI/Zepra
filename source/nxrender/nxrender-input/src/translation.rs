//! Gesture Translation
//!
//! Cross-platform gesture mapping and translation.

use nxgfx::Point;

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum GestureAction {
    Back,
    Forward,
    Refresh,
    Home,
    Search,
    ZoomIn,
    ZoomOut,
    ZoomReset,
    ScrollUp,
    ScrollDown,
    ScrollLeft,
    ScrollRight,
    NextTab,
    PrevTab,
    CloseTab,
    NewTab,
    FullScreen,
    ExitFullScreen,
    Custom(u32),
}

#[derive(Debug, Clone)]
pub struct GestureBinding {
    pub pattern: GesturePattern,
    pub action: GestureAction,
    pub fingers: u8,
}

#[derive(Debug, Clone, PartialEq)]
pub enum GesturePattern {
    SwipeLeft,
    SwipeRight,
    SwipeUp,
    SwipeDown,
    PinchIn,
    PinchOut,
    Rotate(f32),
    DoubleTap,
    TripleTap,
    LongPress,
    TwoFingerSwipeLeft,
    TwoFingerSwipeRight,
    ThreeFingerSwipeUp,
    ThreeFingerSwipeDown,
    EdgeSwipeLeft,
    EdgeSwipeRight,
}

pub struct GestureTranslator {
    bindings: Vec<GestureBinding>,
    screen_width: f32,
    edge_threshold: f32,
}

impl Default for GestureTranslator {
    fn default() -> Self {
        Self::new()
    }
}

impl GestureTranslator {
    pub fn new() -> Self {
        let mut translator = Self {
            bindings: Vec::new(),
            screen_width: 1920.0,
            edge_threshold: 50.0,
        };
        translator.setup_defaults();
        translator
    }
    
    fn setup_defaults(&mut self) {
        self.bindings = vec![
            GestureBinding { pattern: GesturePattern::SwipeRight, action: GestureAction::Back, fingers: 2 },
            GestureBinding { pattern: GesturePattern::SwipeLeft, action: GestureAction::Forward, fingers: 2 },
            GestureBinding { pattern: GesturePattern::PinchIn, action: GestureAction::ZoomOut, fingers: 2 },
            GestureBinding { pattern: GesturePattern::PinchOut, action: GestureAction::ZoomIn, fingers: 2 },
            GestureBinding { pattern: GesturePattern::DoubleTap, action: GestureAction::ZoomReset, fingers: 2 },
            GestureBinding { pattern: GesturePattern::ThreeFingerSwipeUp, action: GestureAction::FullScreen, fingers: 3 },
            GestureBinding { pattern: GesturePattern::ThreeFingerSwipeDown, action: GestureAction::ExitFullScreen, fingers: 3 },
            GestureBinding { pattern: GesturePattern::SwipeLeft, action: GestureAction::NextTab, fingers: 3 },
            GestureBinding { pattern: GesturePattern::SwipeRight, action: GestureAction::PrevTab, fingers: 3 },
            GestureBinding { pattern: GesturePattern::EdgeSwipeLeft, action: GestureAction::Back, fingers: 1 },
            GestureBinding { pattern: GesturePattern::EdgeSwipeRight, action: GestureAction::Forward, fingers: 1 },
        ];
    }
    
    pub fn translate(&self, pattern: GesturePattern, fingers: u8, start_pos: Option<Point>) -> Option<GestureAction> {
        // Check edge gestures
        if let Some(pos) = start_pos {
            if pos.x < self.edge_threshold && pattern == GesturePattern::SwipeRight {
                return Some(GestureAction::Back);
            }
            if pos.x > self.screen_width - self.edge_threshold && pattern == GesturePattern::SwipeLeft {
                return Some(GestureAction::Forward);
            }
        }
        
        // Find matching binding
        self.bindings.iter()
            .find(|b| b.pattern == pattern && b.fingers == fingers)
            .map(|b| b.action)
    }
    
    pub fn add_binding(&mut self, pattern: GesturePattern, action: GestureAction, fingers: u8) {
        self.bindings.push(GestureBinding { pattern, action, fingers });
    }
    
    pub fn set_screen_width(&mut self, width: f32) {
        self.screen_width = width;
    }
    
    pub fn set_edge_threshold(&mut self, threshold: f32) {
        self.edge_threshold = threshold;
    }
}

#[derive(Debug, Clone)]
pub struct GestureVelocity {
    pub x: f32,
    pub y: f32,
    pub angular: f32,
    pub scale: f32,
}

impl GestureVelocity {
    pub fn magnitude(&self) -> f32 {
        (self.x * self.x + self.y * self.y).sqrt()
    }
    
    pub fn direction_degrees(&self) -> f32 {
        self.y.atan2(self.x).to_degrees()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_gesture_translation() {
        let translator = GestureTranslator::new();
        
        let action = translator.translate(GesturePattern::SwipeRight, 2, None);
        assert_eq!(action, Some(GestureAction::Back));
    }
    
    #[test]
    fn test_edge_gesture() {
        let translator = GestureTranslator::new();
        
        // Edge swipe from left
        let action = translator.translate(
            GesturePattern::SwipeRight, 
            1, 
            Some(Point::new(20.0, 500.0))
        );
        assert_eq!(action, Some(GestureAction::Back));
    }
    
    #[test]
    fn test_pinch_zoom() {
        let translator = GestureTranslator::new();
        
        assert_eq!(
            translator.translate(GesturePattern::PinchOut, 2, None),
            Some(GestureAction::ZoomIn)
        );
    }
}
