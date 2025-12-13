//! NXRender Animation System

pub mod animator;
pub mod easing;
pub mod spring;

pub use animator::{Animator, Animation, AnimationState};
pub use easing::EasingFunction;
pub use spring::{Spring, SpringConfig};
