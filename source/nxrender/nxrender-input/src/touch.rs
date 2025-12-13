//! Touch Input Handling

use nxgfx::Point;
use crate::events::Event;
use std::collections::HashMap;

#[derive(Debug, Clone, Copy)]
pub struct TouchPoint {
    pub id: u64,
    pub start_pos: Point,
    pub current_pos: Point,
    pub start_time: std::time::Instant,
}

pub struct TouchState {
    active_touches: HashMap<u64, TouchPoint>,
}

impl Default for TouchState {
    fn default() -> Self {
        Self::new()
    }
}

impl TouchState {
    pub fn new() -> Self {
        Self { active_touches: HashMap::new() }
    }
    
    pub fn active_count(&self) -> usize {
        self.active_touches.len()
    }
    
    pub fn get_touch(&self, id: u64) -> Option<&TouchPoint> {
        self.active_touches.get(&id)
    }
    
    pub fn all_touches(&self) -> impl Iterator<Item = &TouchPoint> {
        self.active_touches.values()
    }
    
    pub fn process_event(&mut self, event: &Event) {
        match event {
            Event::TouchStart { id, pos } => {
                self.active_touches.insert(*id, TouchPoint {
                    id: *id,
                    start_pos: *pos,
                    current_pos: *pos,
                    start_time: std::time::Instant::now(),
                });
            }
            Event::TouchMove { id, pos } => {
                if let Some(touch) = self.active_touches.get_mut(id) {
                    touch.current_pos = *pos;
                }
            }
            Event::TouchEnd { id, .. } | Event::TouchCancel { id } => {
                self.active_touches.remove(id);
            }
            _ => {}
        }
    }
    
    pub fn center(&self) -> Option<Point> {
        if self.active_touches.is_empty() {
            return None;
        }
        let sum: Point = self.active_touches.values()
            .fold(Point::ZERO, |acc, t| Point::new(acc.x + t.current_pos.x, acc.y + t.current_pos.y));
        let count = self.active_touches.len() as f32;
        Some(Point::new(sum.x / count, sum.y / count))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_touch_state() {
        let mut state = TouchState::new();
        
        state.process_event(&Event::TouchStart { id: 1, pos: Point::new(100.0, 100.0) });
        assert_eq!(state.active_count(), 1);
        
        state.process_event(&Event::TouchEnd { id: 1, pos: Point::new(100.0, 100.0) });
        assert_eq!(state.active_count(), 0);
    }
}
