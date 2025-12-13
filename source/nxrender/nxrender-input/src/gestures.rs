//! Gesture Recognition

use nxgfx::Point;
use crate::touch::TouchState;

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum GestureType {
    Tap,
    DoubleTap,
    LongPress,
    Pan,
    Swipe(SwipeDirection),
    Pinch,
    Rotate,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum SwipeDirection {
    Left,
    Right,
    Up,
    Down,
}

#[derive(Debug, Clone)]
pub struct Gesture {
    pub gesture_type: GestureType,
    pub position: Point,
    pub delta: Point,
    pub scale: f32,
    pub rotation: f32,
    pub velocity: Point,
}

impl Gesture {
    pub fn tap(pos: Point) -> Self {
        Self { gesture_type: GestureType::Tap, position: pos, delta: Point::ZERO, scale: 1.0, rotation: 0.0, velocity: Point::ZERO }
    }
    
    pub fn pan(pos: Point, delta: Point) -> Self {
        Self { gesture_type: GestureType::Pan, position: pos, delta, scale: 1.0, rotation: 0.0, velocity: Point::ZERO }
    }
    
    pub fn swipe(pos: Point, direction: SwipeDirection, velocity: Point) -> Self {
        Self { gesture_type: GestureType::Swipe(direction), position: pos, delta: Point::ZERO, scale: 1.0, rotation: 0.0, velocity }
    }
    
    pub fn pinch(pos: Point, scale: f32) -> Self {
        Self { gesture_type: GestureType::Pinch, position: pos, delta: Point::ZERO, scale, rotation: 0.0, velocity: Point::ZERO }
    }
}

pub struct GestureRecognizer {
    tap_threshold: f32,
    swipe_threshold: f32,
    long_press_duration: std::time::Duration,
}

impl Default for GestureRecognizer {
    fn default() -> Self {
        Self::new()
    }
}

impl GestureRecognizer {
    pub fn new() -> Self {
        Self {
            tap_threshold: 10.0,
            swipe_threshold: 50.0,
            long_press_duration: std::time::Duration::from_millis(500),
        }
    }
    
    pub fn detect_from_touch(&self, touch_state: &TouchState) -> Option<Gesture> {
        let count = touch_state.active_count();
        
        if count == 0 {
            return None;
        }
        
        if count == 1 {
            if let Some(touch) = touch_state.all_touches().next() {
                let dx = touch.current_pos.x - touch.start_pos.x;
                let dy = touch.current_pos.y - touch.start_pos.y;
                let distance = (dx * dx + dy * dy).sqrt();
                
                if distance > self.swipe_threshold {
                    let direction = if dx.abs() > dy.abs() {
                        if dx > 0.0 { SwipeDirection::Right } else { SwipeDirection::Left }
                    } else {
                        if dy > 0.0 { SwipeDirection::Down } else { SwipeDirection::Up }
                    };
                    return Some(Gesture::swipe(touch.current_pos, direction, Point::new(dx, dy)));
                } else if distance > self.tap_threshold {
                    return Some(Gesture::pan(touch.current_pos, Point::new(dx, dy)));
                }
            }
        }
        
        if count == 2 {
            let touches: Vec<_> = touch_state.all_touches().collect();
            if touches.len() >= 2 {
                let start_dist = distance(touches[0].start_pos, touches[1].start_pos);
                let current_dist = distance(touches[0].current_pos, touches[1].current_pos);
                
                if start_dist > 0.0 {
                    let scale = current_dist / start_dist;
                    if let Some(center) = touch_state.center() {
                        return Some(Gesture::pinch(center, scale));
                    }
                }
            }
        }
        
        None
    }
}

fn distance(a: Point, b: Point) -> f32 {
    let dx = b.x - a.x;
    let dy = b.y - a.y;
    (dx * dx + dy * dy).sqrt()
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_gesture_tap() {
        let gesture = Gesture::tap(Point::new(100.0, 100.0));
        assert_eq!(gesture.gesture_type, GestureType::Tap);
    }
    
    #[test]
    fn test_swipe_direction() {
        let gesture = Gesture::swipe(Point::ZERO, SwipeDirection::Right, Point::new(100.0, 0.0));
        assert_eq!(gesture.gesture_type, GestureType::Swipe(SwipeDirection::Right));
    }
}
