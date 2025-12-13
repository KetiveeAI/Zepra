//! Texture Cache
//!
//! Manages GPU textures for images to avoid redundant uploads.

use std::collections::HashMap;
use std::hash::{Hash, Hasher};
use std::path::{Path, PathBuf};
use std::time::{Duration, Instant};

use super::loader::ImageData;

/// Unique identifier for a texture
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum TextureKey {
    /// Loaded from a file path
    Path(PathBuf),
    /// Generated with a name
    Named(String),
    /// Generated with a numeric ID
    Id(u64),
}

/// GPU texture handle (platform-specific)
#[derive(Debug, Clone, Copy)]
pub struct TextureHandle {
    /// OpenGL texture ID or similar
    pub id: u32,
    /// Width in pixels
    pub width: u32,
    /// Height in pixels  
    pub height: u32,
}

/// Cached texture entry
struct CacheEntry {
    handle: TextureHandle,
    key: TextureKey,
    last_used: Instant,
    size_bytes: usize,
}

/// Texture cache configuration
#[derive(Debug, Clone)]
pub struct TextureCacheConfig {
    /// Maximum cache size in bytes
    pub max_size_bytes: usize,
    /// Maximum number of textures
    pub max_textures: usize,
    /// Time after which unused textures may be evicted
    pub eviction_timeout: Duration,
}

impl Default for TextureCacheConfig {
    fn default() -> Self {
        Self {
            max_size_bytes: 128 * 1024 * 1024, // 128 MB
            max_textures: 256,
            eviction_timeout: Duration::from_secs(60),
        }
    }
}

/// Texture cache manager
pub struct TextureCache {
    textures: HashMap<TextureKey, CacheEntry>,
    config: TextureCacheConfig,
    current_size_bytes: usize,
    next_id: u64,
    // Callback for creating GPU textures (set by the backend)
    create_texture: Option<Box<dyn Fn(&ImageData) -> TextureHandle>>,
    delete_texture: Option<Box<dyn Fn(TextureHandle)>>,
}

impl TextureCache {
    /// Create a new texture cache
    pub fn new() -> Self {
        Self::with_config(TextureCacheConfig::default())
    }
    
    /// Create with custom configuration
    pub fn with_config(config: TextureCacheConfig) -> Self {
        Self {
            textures: HashMap::new(),
            config,
            current_size_bytes: 0,
            next_id: 0,
            create_texture: None,
            delete_texture: None,
        }
    }
    
    /// Set the texture creation callback
    pub fn set_create_callback<F>(&mut self, f: F)
    where
        F: Fn(&ImageData) -> TextureHandle + 'static,
    {
        self.create_texture = Some(Box::new(f));
    }
    
    /// Set the texture deletion callback
    pub fn set_delete_callback<F>(&mut self, f: F)
    where
        F: Fn(TextureHandle) + 'static,
    {
        self.delete_texture = Some(Box::new(f));
    }
    
    /// Get or create a texture for an image
    pub fn get_or_create(&mut self, key: TextureKey, image: &ImageData) -> Option<TextureHandle> {
        // Check cache
        if let Some(entry) = self.textures.get_mut(&key) {
            entry.last_used = Instant::now();
            return Some(entry.handle);
        }
        
        // Create new texture
        let create_fn = self.create_texture.as_ref()?;
        let handle = create_fn(image);
        
        let size_bytes = (image.width * image.height * 4) as usize;
        
        // Evict if necessary
        while self.current_size_bytes + size_bytes > self.config.max_size_bytes
            || self.textures.len() >= self.config.max_textures
        {
            if !self.evict_one() {
                break;
            }
        }
        
        // Add to cache
        self.textures.insert(key.clone(), CacheEntry {
            handle,
            key: key.clone(),
            last_used: Instant::now(),
            size_bytes,
        });
        
        self.current_size_bytes += size_bytes;
        
        Some(handle)
    }
    
    /// Get a texture from cache without creating
    pub fn get(&mut self, key: &TextureKey) -> Option<TextureHandle> {
        if let Some(entry) = self.textures.get_mut(key) {
            entry.last_used = Instant::now();
            Some(entry.handle)
        } else {
            None
        }
    }
    
    /// Check if a texture is cached
    pub fn contains(&self, key: &TextureKey) -> bool {
        self.textures.contains_key(key)
    }
    
    /// Remove a texture from cache
    pub fn remove(&mut self, key: &TextureKey) -> bool {
        if let Some(entry) = self.textures.remove(key) {
            self.current_size_bytes = self.current_size_bytes.saturating_sub(entry.size_bytes);
            
            if let Some(delete_fn) = &self.delete_texture {
                delete_fn(entry.handle);
            }
            
            true
        } else {
            false
        }
    }
    
    /// Evict the least recently used texture
    fn evict_one(&mut self) -> bool {
        let oldest_key = self.textures
            .iter()
            .min_by_key(|(_, entry)| entry.last_used)
            .map(|(key, _)| key.clone());
        
        if let Some(key) = oldest_key {
            self.remove(&key)
        } else {
            false
        }
    }
    
    /// Evict textures that haven't been used recently
    pub fn evict_stale(&mut self) {
        let now = Instant::now();
        let timeout = self.config.eviction_timeout;
        
        let stale_keys: Vec<_> = self.textures
            .iter()
            .filter(|(_, entry)| now.duration_since(entry.last_used) > timeout)
            .map(|(key, _)| key.clone())
            .collect();
        
        for key in stale_keys {
            self.remove(&key);
        }
    }
    
    /// Clear all cached textures
    pub fn clear(&mut self) {
        if let Some(delete_fn) = &self.delete_texture {
            for (_, entry) in self.textures.drain() {
                delete_fn(entry.handle);
            }
        } else {
            self.textures.clear();
        }
        
        self.current_size_bytes = 0;
    }
    
    /// Get cache statistics
    pub fn stats(&self) -> TextureCacheStats {
        TextureCacheStats {
            texture_count: self.textures.len(),
            size_bytes: self.current_size_bytes,
            max_size_bytes: self.config.max_size_bytes,
        }
    }
    
    /// Generate a unique texture ID
    pub fn next_id(&mut self) -> TextureKey {
        let id = self.next_id;
        self.next_id += 1;
        TextureKey::Id(id)
    }
}

impl Default for TextureCache {
    fn default() -> Self {
        Self::new()
    }
}

/// Cache statistics
#[derive(Debug, Clone)]
pub struct TextureCacheStats {
    pub texture_count: usize,
    pub size_bytes: usize,
    pub max_size_bytes: usize,
}

impl TextureCacheStats {
    pub fn usage_percent(&self) -> f32 {
        if self.max_size_bytes == 0 {
            0.0
        } else {
            (self.size_bytes as f32 / self.max_size_bytes as f32) * 100.0
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_texture_cache_stats() {
        let cache = TextureCache::new();
        let stats = cache.stats();
        assert_eq!(stats.texture_count, 0);
        assert_eq!(stats.size_bytes, 0);
    }
    
    #[test]
    fn test_texture_key() {
        let key1 = TextureKey::Path(PathBuf::from("/test.png"));
        let key2 = TextureKey::Named("test".into());
        let key3 = TextureKey::Id(42);
        
        assert_ne!(key1, key2);
        assert_ne!(key2, key3);
    }
}
