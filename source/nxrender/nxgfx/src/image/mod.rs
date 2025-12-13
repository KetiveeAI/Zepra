//! Image Module
//!
//! Provides image loading and texture management:
//! - PNG, JPEG, WebP, GIF, BMP loading
//! - Texture caching for GPU
//! - Image manipulation utilities

pub mod loader;
pub mod cache;

pub use loader::{ImageData, ImageFormat};
pub use cache::{TextureCache, TextureKey, TextureHandle, TextureCacheConfig, TextureCacheStats};
