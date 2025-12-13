//! Keyboard Input Handling

use crate::events::{Event, KeyCode, Modifiers};
use std::collections::HashSet;

pub struct KeyboardState {
    pressed_keys: HashSet<KeyCode>,
    modifiers: Modifiers,
}

impl Default for KeyboardState {
    fn default() -> Self {
        Self::new()
    }
}

impl KeyboardState {
    pub fn new() -> Self {
        Self {
            pressed_keys: HashSet::new(),
            modifiers: Modifiers::NONE,
        }
    }
    
    pub fn is_key_down(&self, key: KeyCode) -> bool {
        self.pressed_keys.contains(&key)
    }
    
    pub fn modifiers(&self) -> Modifiers { self.modifiers }
    
    pub fn is_shift(&self) -> bool { self.modifiers.shift }
    pub fn is_ctrl(&self) -> bool { self.modifiers.ctrl }
    pub fn is_alt(&self) -> bool { self.modifiers.alt }
    
    pub fn process_event(&mut self, event: &Event) {
        match event {
            Event::KeyDown { key, modifiers } => {
                self.pressed_keys.insert(*key);
                self.modifiers = *modifiers;
            }
            Event::KeyUp { key, modifiers } => {
                self.pressed_keys.remove(key);
                self.modifiers = *modifiers;
            }
            _ => {}
        }
    }
    
    pub fn is_shortcut(&self, key: KeyCode, ctrl: bool, shift: bool, alt: bool) -> bool {
        self.is_key_down(key) && 
        self.modifiers.ctrl == ctrl && 
        self.modifiers.shift == shift && 
        self.modifiers.alt == alt
    }
}

#[derive(Debug, Clone)]
pub struct KeyboardShortcut {
    pub key: KeyCode,
    pub ctrl: bool,
    pub shift: bool,
    pub alt: bool,
}

impl KeyboardShortcut {
    pub fn new(key: KeyCode) -> Self {
        Self { key, ctrl: false, shift: false, alt: false }
    }
    
    pub fn ctrl(mut self) -> Self { self.ctrl = true; self }
    pub fn shift(mut self) -> Self { self.shift = true; self }
    pub fn alt(mut self) -> Self { self.alt = true; self }
    
    pub fn matches(&self, state: &KeyboardState) -> bool {
        state.is_shortcut(self.key, self.ctrl, self.shift, self.alt)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_keyboard_state() {
        let mut state = KeyboardState::new();
        
        state.process_event(&Event::KeyDown { 
            key: KeyCode::A, 
            modifiers: Modifiers::NONE 
        });
        
        assert!(state.is_key_down(KeyCode::A));
        assert!(!state.is_key_down(KeyCode::B));
    }
    
    #[test]
    fn test_shortcut() {
        let shortcut = KeyboardShortcut::new(KeyCode::S).ctrl();
        let mut state = KeyboardState::new();
        
        state.process_event(&Event::KeyDown {
            key: KeyCode::S,
            modifiers: Modifiers::NONE.with_ctrl(),
        });
        
        assert!(shortcut.matches(&state));
    }
}
