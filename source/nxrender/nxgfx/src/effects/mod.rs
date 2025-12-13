//! Visual Effects Module
//!
//! Provides GPU-accelerated effects like gradients,
//! shadows, and blur.

pub mod gradient;
pub mod shadow;
pub mod blur;

pub use gradient::{LinearGradient, RadialGradient, ColorStop, GradientDirection};
pub use shadow::{Shadow, BoxShadows};
pub use blur::{Blur, BlurQuality, box_blur_rgba, box_blur_rgba_multi};
