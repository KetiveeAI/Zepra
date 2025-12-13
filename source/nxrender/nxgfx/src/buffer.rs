//! GPU Buffer Management
//!
//! Handles allocation and management of GPU memory buffers
//! for vertex data, textures, and uniforms.

/// Memory type for GPU allocations
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum MemoryType {
    /// Device-local memory (fast GPU access)
    DeviceLocal,
    /// Host-visible memory (CPU can read/write)
    HostVisible,
    /// Staging buffer for transfers
    Staging,
}

/// Handle to a GPU buffer
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct BufferHandle(pub(crate) u32);

/// GPU buffer for storing vertex/index/uniform data
pub struct GpuBuffer {
    handle: BufferHandle,
    size: usize,
    mem_type: MemoryType,
}

impl GpuBuffer {
    /// Create a new GPU buffer
    pub fn new(size: usize, mem_type: MemoryType) -> Self {
        // TODO: Implement actual GPU allocation
        Self {
            handle: BufferHandle(0),
            size,
            mem_type,
        }
    }
    
    /// Get the buffer handle
    pub fn handle(&self) -> BufferHandle {
        self.handle
    }
    
    /// Get the buffer size in bytes
    pub fn size(&self) -> usize {
        self.size
    }
    
    /// Get the memory type
    pub fn memory_type(&self) -> MemoryType {
        self.mem_type
    }
    
    /// Upload data to the buffer
    pub fn upload<T: Copy>(&mut self, data: &[T]) {
        let byte_size = std::mem::size_of_val(data);
        assert!(byte_size <= self.size, "Data exceeds buffer size");
        // TODO: Implement actual GPU upload
    }
}

/// GPU memory allocator
pub struct MemoryAllocator {
    allocations: Vec<Allocation>,
    next_handle: u32,
}

struct Allocation {
    handle: BufferHandle,
    size: usize,
    mem_type: MemoryType,
}

impl MemoryAllocator {
    /// Create a new memory allocator
    pub fn new() -> Self {
        Self {
            allocations: Vec::new(),
            next_handle: 1,
        }
    }
    
    /// Allocate a GPU buffer
    pub fn allocate(&mut self, size: usize, mem_type: MemoryType) -> GpuBuffer {
        let handle = BufferHandle(self.next_handle);
        self.next_handle += 1;
        
        self.allocations.push(Allocation {
            handle,
            size,
            mem_type,
        });
        
        GpuBuffer {
            handle,
            size,
            mem_type,
        }
    }
    
    /// Free a GPU buffer
    pub fn free(&mut self, buffer: GpuBuffer) {
        self.allocations.retain(|a| a.handle != buffer.handle);
    }
    
    /// Get total allocated memory
    pub fn total_allocated(&self) -> usize {
        self.allocations.iter().map(|a| a.size).sum()
    }
}

impl Default for MemoryAllocator {
    fn default() -> Self {
        Self::new()
    }
}
