//! OpenGL ES 3.0 Backend
//!
//! Primary rendering backend using OpenGL for cross-platform support.

use glow::HasContext;
use crate::{GfxError, Result};
use crate::primitives::{Color, Rect, Point, Size};

/// OpenGL rendering backend
pub struct OpenGLBackend {
    gl: glow::Context,
    shader_program: glow::Program,
    vao: glow::VertexArray,
    vbo: glow::Buffer,
    screen_size: Size,
}

// Vertex shader - transforms vertices
const VERTEX_SHADER: &str = r#"
#version 330 core
layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec4 a_color;

out vec4 v_color;

uniform vec2 u_resolution;

void main() {
    // Convert pixel coordinates to normalized device coordinates
    vec2 ndc = (a_pos / u_resolution) * 2.0 - 1.0;
    ndc.y = -ndc.y; // Flip Y axis (screen coordinates have Y down)
    gl_Position = vec4(ndc, 0.0, 1.0);
    v_color = a_color;
}
"#;

// Fragment shader - outputs color
const FRAGMENT_SHADER: &str = r#"
#version 330 core
in vec4 v_color;
out vec4 frag_color;

void main() {
    frag_color = v_color;
}
"#;

/// Vertex with position and color
#[repr(C)]
#[derive(Clone, Copy)]
struct Vertex {
    pos: [f32; 2],
    color: [f32; 4],
}

impl Vertex {
    fn new(x: f32, y: f32, color: Color) -> Self {
        Self {
            pos: [x, y],
            color: color.to_f32_array(),
        }
    }
}

impl OpenGLBackend {
    /// Create a new OpenGL backend from an existing glow context
    pub fn new(gl: glow::Context, screen_size: Size) -> Result<Self> {
        unsafe {
            // Create shader program
            let shader_program = Self::create_shader_program(&gl)?;
            
            // Create VAO and VBO
            let vao = gl.create_vertex_array()
                .map_err(|e| GfxError::InitializationFailed(e))?;
            let vbo = gl.create_buffer()
                .map_err(|e| GfxError::InitializationFailed(e))?;
            
            gl.bind_vertex_array(Some(vao));
            gl.bind_buffer(glow::ARRAY_BUFFER, Some(vbo));
            
            // Position attribute (location 0)
            gl.enable_vertex_attrib_array(0);
            gl.vertex_attrib_pointer_f32(
                0,
                2,
                glow::FLOAT,
                false,
                std::mem::size_of::<Vertex>() as i32,
                0,
            );
            
            // Color attribute (location 1)
            gl.enable_vertex_attrib_array(1);
            gl.vertex_attrib_pointer_f32(
                1,
                4,
                glow::FLOAT,
                false,
                std::mem::size_of::<Vertex>() as i32,
                8, // offset after position (2 floats * 4 bytes)
            );
            
            gl.bind_vertex_array(None);
            
            // Enable blending for transparency
            gl.enable(glow::BLEND);
            gl.blend_func(glow::SRC_ALPHA, glow::ONE_MINUS_SRC_ALPHA);
            
            Ok(Self {
                gl,
                shader_program,
                vao,
                vbo,
                screen_size,
            })
        }
    }
    
    /// Create and compile shader program
    unsafe fn create_shader_program(gl: &glow::Context) -> Result<glow::Program> {
        // Compile vertex shader
        let vertex_shader = gl.create_shader(glow::VERTEX_SHADER)
            .map_err(|e| GfxError::ShaderCompilationFailed(e))?;
        gl.shader_source(vertex_shader, VERTEX_SHADER);
        gl.compile_shader(vertex_shader);
        
        if !gl.get_shader_compile_status(vertex_shader) {
            let log = gl.get_shader_info_log(vertex_shader);
            return Err(GfxError::ShaderCompilationFailed(format!("Vertex shader: {}", log)));
        }
        
        // Compile fragment shader
        let fragment_shader = gl.create_shader(glow::FRAGMENT_SHADER)
            .map_err(|e| GfxError::ShaderCompilationFailed(e))?;
        gl.shader_source(fragment_shader, FRAGMENT_SHADER);
        gl.compile_shader(fragment_shader);
        
        if !gl.get_shader_compile_status(fragment_shader) {
            let log = gl.get_shader_info_log(fragment_shader);
            return Err(GfxError::ShaderCompilationFailed(format!("Fragment shader: {}", log)));
        }
        
        // Link program
        let program = gl.create_program()
            .map_err(|e| GfxError::ShaderCompilationFailed(e))?;
        gl.attach_shader(program, vertex_shader);
        gl.attach_shader(program, fragment_shader);
        gl.link_program(program);
        
        if !gl.get_program_link_status(program) {
            let log = gl.get_program_info_log(program);
            return Err(GfxError::ShaderCompilationFailed(format!("Program link: {}", log)));
        }
        
        // Clean up shaders (they're linked now)
        gl.delete_shader(vertex_shader);
        gl.delete_shader(fragment_shader);
        
        Ok(program)
    }
    
    /// Resize the viewport
    pub fn resize(&mut self, width: f32, height: f32) {
        self.screen_size = Size::new(width, height);
        unsafe {
            self.gl.viewport(0, 0, width as i32, height as i32);
        }
    }
    
    /// Clear the screen
    pub fn clear(&self, color: Color) {
        let [r, g, b, a] = color.to_f32_array();
        unsafe {
            self.gl.clear_color(r, g, b, a);
            self.gl.clear(glow::COLOR_BUFFER_BIT);
        }
    }
    
    /// Begin a frame
    pub fn begin_frame(&self) {
        unsafe {
            self.gl.use_program(Some(self.shader_program));
            
            // Set resolution uniform
            let loc = self.gl.get_uniform_location(self.shader_program, "u_resolution");
            self.gl.uniform_2_f32(loc.as_ref(), self.screen_size.width, self.screen_size.height);
        }
    }
    
    /// Fill a rectangle
    pub fn fill_rect(&self, rect: Rect, color: Color) {
        let vertices = [
            Vertex::new(rect.x, rect.y, color),
            Vertex::new(rect.x + rect.width, rect.y, color),
            Vertex::new(rect.x + rect.width, rect.y + rect.height, color),
            Vertex::new(rect.x, rect.y, color),
            Vertex::new(rect.x + rect.width, rect.y + rect.height, color),
            Vertex::new(rect.x, rect.y + rect.height, color),
        ];
        
        self.draw_vertices(&vertices, glow::TRIANGLES);
    }
    
    /// Stroke a rectangle (outline only)
    pub fn stroke_rect(&self, rect: Rect, color: Color, width: f32) {
        // Top edge
        self.fill_rect(Rect::new(rect.x, rect.y, rect.width, width), color);
        // Bottom edge
        self.fill_rect(Rect::new(rect.x, rect.y + rect.height - width, rect.width, width), color);
        // Left edge
        self.fill_rect(Rect::new(rect.x, rect.y, width, rect.height), color);
        // Right edge
        self.fill_rect(Rect::new(rect.x + rect.width - width, rect.y, width, rect.height), color);
    }
    
    /// Fill a rounded rectangle
    pub fn fill_rounded_rect(&self, rect: Rect, color: Color, radius: f32) {
        // Clamp radius to half of smallest dimension
        let radius = radius.min(rect.width / 2.0).min(rect.height / 2.0);
        
        if radius <= 0.0 {
            return self.fill_rect(rect, color);
        }
        
        let mut vertices = Vec::new();
        
        // Center rectangle (full height, reduced width)
        let center = Rect::new(rect.x + radius, rect.y, rect.width - 2.0 * radius, rect.height);
        self.add_rect_vertices(&mut vertices, center, color);
        
        // Left rectangle
        let left = Rect::new(rect.x, rect.y + radius, radius, rect.height - 2.0 * radius);
        self.add_rect_vertices(&mut vertices, left, color);
        
        // Right rectangle
        let right = Rect::new(rect.x + rect.width - radius, rect.y + radius, radius, rect.height - 2.0 * radius);
        self.add_rect_vertices(&mut vertices, right, color);
        
        // Corners (approximated with triangles)
        let segments = 8;
        
        // Top-left corner
        self.add_corner_vertices(&mut vertices, 
            Point::new(rect.x + radius, rect.y + radius), 
            radius, color, segments, 180.0, 270.0);
        
        // Top-right corner
        self.add_corner_vertices(&mut vertices,
            Point::new(rect.x + rect.width - radius, rect.y + radius),
            radius, color, segments, 270.0, 360.0);
        
        // Bottom-right corner
        self.add_corner_vertices(&mut vertices,
            Point::new(rect.x + rect.width - radius, rect.y + rect.height - radius),
            radius, color, segments, 0.0, 90.0);
        
        // Bottom-left corner
        self.add_corner_vertices(&mut vertices,
            Point::new(rect.x + radius, rect.y + rect.height - radius),
            radius, color, segments, 90.0, 180.0);
        
        self.draw_vertices(&vertices, glow::TRIANGLES);
    }
    
    /// Add rectangle vertices to a buffer
    fn add_rect_vertices(&self, vertices: &mut Vec<Vertex>, rect: Rect, color: Color) {
        vertices.push(Vertex::new(rect.x, rect.y, color));
        vertices.push(Vertex::new(rect.x + rect.width, rect.y, color));
        vertices.push(Vertex::new(rect.x + rect.width, rect.y + rect.height, color));
        vertices.push(Vertex::new(rect.x, rect.y, color));
        vertices.push(Vertex::new(rect.x + rect.width, rect.y + rect.height, color));
        vertices.push(Vertex::new(rect.x, rect.y + rect.height, color));
    }
    
    /// Add corner arc vertices
    fn add_corner_vertices(&self, vertices: &mut Vec<Vertex>, center: Point, radius: f32, 
                           color: Color, segments: u32, start_angle: f32, end_angle: f32) {
        let step = (end_angle - start_angle) / segments as f32;
        
        for i in 0..segments {
            let angle1 = (start_angle + step * i as f32).to_radians();
            let angle2 = (start_angle + step * (i + 1) as f32).to_radians();
            
            vertices.push(Vertex::new(center.x, center.y, color));
            vertices.push(Vertex::new(
                center.x + radius * angle1.cos(),
                center.y + radius * angle1.sin(),
                color,
            ));
            vertices.push(Vertex::new(
                center.x + radius * angle2.cos(),
                center.y + radius * angle2.sin(),
                color,
            ));
        }
    }
    
    /// Fill a circle
    pub fn fill_circle(&self, center: Point, radius: f32, color: Color) {
        let segments = 32;
        let mut vertices = Vec::with_capacity(segments as usize * 3);
        
        let step = 360.0 / segments as f32;
        
        for i in 0..segments {
            let angle1 = (step * i as f32).to_radians();
            let angle2 = (step * (i + 1) as f32).to_radians();
            
            vertices.push(Vertex::new(center.x, center.y, color));
            vertices.push(Vertex::new(
                center.x + radius * angle1.cos(),
                center.y + radius * angle1.sin(),
                color,
            ));
            vertices.push(Vertex::new(
                center.x + radius * angle2.cos(),
                center.y + radius * angle2.sin(),
                color,
            ));
        }
        
        self.draw_vertices(&vertices, glow::TRIANGLES);
    }
    
    /// Draw a line
    pub fn draw_line(&self, p1: Point, p2: Point, color: Color, width: f32) {
        // Calculate perpendicular vector
        let dx = p2.x - p1.x;
        let dy = p2.y - p1.y;
        let len = (dx * dx + dy * dy).sqrt();
        
        if len == 0.0 {
            return;
        }
        
        let nx = -dy / len * width / 2.0;
        let ny = dx / len * width / 2.0;
        
        let vertices = [
            Vertex::new(p1.x + nx, p1.y + ny, color),
            Vertex::new(p1.x - nx, p1.y - ny, color),
            Vertex::new(p2.x - nx, p2.y - ny, color),
            Vertex::new(p1.x + nx, p1.y + ny, color),
            Vertex::new(p2.x - nx, p2.y - ny, color),
            Vertex::new(p2.x + nx, p2.y + ny, color),
        ];
        
        self.draw_vertices(&vertices, glow::TRIANGLES);
    }
    
    /// Draw vertices with a specific primitive type
    fn draw_vertices(&self, vertices: &[Vertex], mode: u32) {
        if vertices.is_empty() {
            return;
        }
        
        unsafe {
            self.gl.bind_vertex_array(Some(self.vao));
            self.gl.bind_buffer(glow::ARRAY_BUFFER, Some(self.vbo));
            
            // Upload vertex data
            let byte_data: &[u8] = std::slice::from_raw_parts(
                vertices.as_ptr() as *const u8,
                vertices.len() * std::mem::size_of::<Vertex>(),
            );
            
            self.gl.buffer_data_u8_slice(glow::ARRAY_BUFFER, byte_data, glow::DYNAMIC_DRAW);
            
            // Draw
            self.gl.draw_arrays(mode, 0, vertices.len() as i32);
            
            self.gl.bind_vertex_array(None);
        }
    }
}

impl Drop for OpenGLBackend {
    fn drop(&mut self) {
        unsafe {
            self.gl.delete_program(self.shader_program);
            self.gl.delete_vertex_array(self.vao);
            self.gl.delete_buffer(self.vbo);
        }
    }
}
