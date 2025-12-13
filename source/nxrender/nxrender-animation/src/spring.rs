//! Spring Physics Animation

#[derive(Debug, Clone, Copy)]
pub struct SpringConfig {
    pub stiffness: f32,
    pub damping: f32,
    pub mass: f32,
}

impl Default for SpringConfig {
    fn default() -> Self {
        Self::DEFAULT
    }
}

impl SpringConfig {
    pub const DEFAULT: Self = Self { stiffness: 100.0, damping: 10.0, mass: 1.0 };
    pub const GENTLE: Self = Self { stiffness: 50.0, damping: 14.0, mass: 1.0 };
    pub const STIFF: Self = Self { stiffness: 300.0, damping: 20.0, mass: 1.0 };
    pub const BOUNCY: Self = Self { stiffness: 200.0, damping: 5.0, mass: 1.0 };
    
    pub fn new(stiffness: f32, damping: f32) -> Self {
        Self { stiffness, damping, mass: 1.0 }
    }
}

#[derive(Debug, Clone)]
pub struct Spring {
    config: SpringConfig,
    position: f32,
    velocity: f32,
    target: f32,
}

impl Spring {
    pub fn new(initial: f32) -> Self {
        Self {
            config: SpringConfig::DEFAULT,
            position: initial,
            velocity: 0.0,
            target: initial,
        }
    }
    
    pub fn with_config(mut self, config: SpringConfig) -> Self {
        self.config = config;
        self
    }
    
    pub fn set_target(&mut self, target: f32) {
        self.target = target;
    }
    
    pub fn position(&self) -> f32 { self.position }
    pub fn velocity(&self) -> f32 { self.velocity }
    pub fn target(&self) -> f32 { self.target }
    
    pub fn is_at_rest(&self) -> bool {
        (self.position - self.target).abs() < 0.001 && self.velocity.abs() < 0.001
    }
    
    pub fn update(&mut self, dt: f32) -> f32 {
        let spring_force = -self.config.stiffness * (self.position - self.target);
        let damping_force = -self.config.damping * self.velocity;
        let acceleration = (spring_force + damping_force) / self.config.mass;
        
        self.velocity += acceleration * dt;
        self.position += self.velocity * dt;
        
        if self.is_at_rest() {
            self.position = self.target;
            self.velocity = 0.0;
        }
        
        self.position
    }
    
    pub fn snap(&mut self, value: f32) {
        self.position = value;
        self.target = value;
        self.velocity = 0.0;
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_spring_at_rest() {
        let spring = Spring::new(0.0);
        assert!(spring.is_at_rest());
    }
    
    #[test]
    fn test_spring_update() {
        let mut spring = Spring::new(0.0);
        spring.set_target(100.0);
        
        for _ in 0..100 {
            spring.update(0.016);
        }
        
        assert!((spring.position() - 100.0).abs() < 1.0);
    }
    
    #[test]
    fn test_spring_snap() {
        let mut spring = Spring::new(0.0);
        spring.snap(50.0);
        
        assert_eq!(spring.position(), 50.0);
        assert!(spring.is_at_rest());
    }
}
