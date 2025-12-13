//! Animation Engine

use std::time::Duration;
use crate::easing::EasingFunction;

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum AnimationState {
    Pending,
    Running,
    Paused,
    Completed,
}

#[derive(Debug, Clone)]
pub struct Animation {
    id: u64,
    from: f32,
    to: f32,
    duration: Duration,
    elapsed: Duration,
    delay: Duration,
    easing: EasingFunction,
    state: AnimationState,
    repeat: bool,
    reverse: bool,
}

impl Animation {
    pub fn new(from: f32, to: f32, duration: Duration) -> Self {
        static NEXT_ID: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(1);
        
        Self {
            id: NEXT_ID.fetch_add(1, std::sync::atomic::Ordering::SeqCst),
            from,
            to,
            duration,
            elapsed: Duration::ZERO,
            delay: Duration::ZERO,
            easing: EasingFunction::EaseInOut,
            state: AnimationState::Pending,
            repeat: false,
            reverse: false,
        }
    }
    
    pub fn with_easing(mut self, easing: EasingFunction) -> Self {
        self.easing = easing;
        self
    }
    
    pub fn with_delay(mut self, delay: Duration) -> Self {
        self.delay = delay;
        self
    }
    
    pub fn repeat(mut self) -> Self {
        self.repeat = true;
        self
    }
    
    pub fn reverse(mut self) -> Self {
        self.reverse = true;
        self
    }
    
    pub fn id(&self) -> u64 { self.id }
    pub fn state(&self) -> AnimationState { self.state }
    pub fn is_complete(&self) -> bool { self.state == AnimationState::Completed }
    
    pub fn start(&mut self) {
        self.state = AnimationState::Running;
        self.elapsed = Duration::ZERO;
    }
    
    pub fn pause(&mut self) {
        if self.state == AnimationState::Running {
            self.state = AnimationState::Paused;
        }
    }
    
    pub fn resume(&mut self) {
        if self.state == AnimationState::Paused {
            self.state = AnimationState::Running;
        }
    }
    
    pub fn update(&mut self, dt: Duration) -> f32 {
        if self.state != AnimationState::Running {
            return self.current_value();
        }
        
        if self.elapsed < self.delay {
            self.elapsed += dt;
            return self.from;
        }
        
        let effective_elapsed = self.elapsed - self.delay;
        self.elapsed += dt;
        
        if effective_elapsed >= self.duration {
            if self.repeat {
                self.elapsed = self.delay;
                if self.reverse {
                    std::mem::swap(&mut self.from, &mut self.to);
                }
            } else {
                self.state = AnimationState::Completed;
                return self.to;
            }
        }
        
        self.current_value()
    }
    
    pub fn current_value(&self) -> f32 {
        if self.state == AnimationState::Pending { return self.from; }
        if self.state == AnimationState::Completed { return self.to; }
        
        let effective_elapsed = (self.elapsed.saturating_sub(self.delay)).as_secs_f32();
        let t = (effective_elapsed / self.duration.as_secs_f32()).clamp(0.0, 1.0);
        let eased = self.easing.apply(t);
        
        self.from + (self.to - self.from) * eased
    }
}

pub struct Animator {
    animations: Vec<Animation>,
}

impl Default for Animator {
    fn default() -> Self {
        Self::new()
    }
}

impl Animator {
    pub fn new() -> Self {
        Self { animations: Vec::new() }
    }
    
    pub fn add(&mut self, mut animation: Animation) -> u64 {
        animation.start();
        let id = animation.id();
        self.animations.push(animation);
        id
    }
    
    pub fn get(&self, id: u64) -> Option<&Animation> {
        self.animations.iter().find(|a| a.id() == id)
    }
    
    pub fn get_value(&self, id: u64) -> Option<f32> {
        self.get(id).map(|a| a.current_value())
    }
    
    pub fn update(&mut self, dt: Duration) {
        for anim in &mut self.animations {
            anim.update(dt);
        }
        self.animations.retain(|a| !a.is_complete());
    }
    
    pub fn is_animating(&self) -> bool {
        self.animations.iter().any(|a| a.state() == AnimationState::Running)
    }
    
    pub fn count(&self) -> usize {
        self.animations.len()
    }
    
    pub fn clear(&mut self) {
        self.animations.clear();
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_animation_creation() {
        let anim = Animation::new(0.0, 100.0, Duration::from_millis(500));
        assert_eq!(anim.state(), AnimationState::Pending);
    }
    
    #[test]
    fn test_animation_update() {
        let mut anim = Animation::new(0.0, 100.0, Duration::from_millis(1000));
        anim.start();
        
        anim.update(Duration::from_millis(500));
        let value = anim.current_value();
        
        assert!(value > 0.0 && value < 100.0);
    }
    
    #[test]
    fn test_animator() {
        let mut animator = Animator::new();
        
        let id = animator.add(Animation::new(0.0, 100.0, Duration::from_millis(100)));
        assert!(animator.is_animating());
        
        for _ in 0..10 {
            animator.update(Duration::from_millis(20));
        }
        
        assert!(!animator.is_animating());
    }
}
