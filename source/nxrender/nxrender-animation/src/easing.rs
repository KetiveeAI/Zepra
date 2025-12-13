//! Easing Functions

use std::f32::consts::PI;

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum EasingFunction {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad,
    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic,
    EaseInQuart,
    EaseOutQuart,
    EaseInOutQuart,
    EaseInSine,
    EaseOutSine,
    EaseInOutSine,
    EaseInExpo,
    EaseOutExpo,
    EaseInOutExpo,
    EaseInBack,
    EaseOutBack,
    EaseInOutBack,
    EaseInElastic,
    EaseOutElastic,
    EaseOutBounce,
    CubicBezier(f32, f32, f32, f32),
}

impl Default for EasingFunction {
    fn default() -> Self {
        Self::EaseInOut
    }
}

impl EasingFunction {
    pub fn apply(&self, t: f32) -> f32 {
        let t = t.clamp(0.0, 1.0);
        
        match self {
            Self::Linear => t,
            Self::EaseIn => Self::ease_in_cubic(t),
            Self::EaseOut => Self::ease_out_cubic(t),
            Self::EaseInOut => Self::ease_in_out_cubic(t),
            Self::EaseInQuad => t * t,
            Self::EaseOutQuad => 1.0 - (1.0 - t) * (1.0 - t),
            Self::EaseInOutQuad => {
                if t < 0.5 { 2.0 * t * t } 
                else { 1.0 - (-2.0 * t + 2.0).powi(2) / 2.0 }
            }
            Self::EaseInCubic => Self::ease_in_cubic(t),
            Self::EaseOutCubic => Self::ease_out_cubic(t),
            Self::EaseInOutCubic => Self::ease_in_out_cubic(t),
            Self::EaseInQuart => t * t * t * t,
            Self::EaseOutQuart => 1.0 - (1.0 - t).powi(4),
            Self::EaseInOutQuart => {
                if t < 0.5 { 8.0 * t.powi(4) }
                else { 1.0 - (-2.0 * t + 2.0).powi(4) / 2.0 }
            }
            Self::EaseInSine => 1.0 - (t * PI / 2.0).cos(),
            Self::EaseOutSine => (t * PI / 2.0).sin(),
            Self::EaseInOutSine => -(((t * PI).cos() - 1.0) / 2.0),
            Self::EaseInExpo => if t == 0.0 { 0.0 } else { 2.0_f32.powf(10.0 * t - 10.0) },
            Self::EaseOutExpo => if t == 1.0 { 1.0 } else { 1.0 - 2.0_f32.powf(-10.0 * t) },
            Self::EaseInOutExpo => {
                if t == 0.0 { 0.0 }
                else if t == 1.0 { 1.0 }
                else if t < 0.5 { 2.0_f32.powf(20.0 * t - 10.0) / 2.0 }
                else { (2.0 - 2.0_f32.powf(-20.0 * t + 10.0)) / 2.0 }
            }
            Self::EaseInBack => {
                let c = 1.70158;
                (c + 1.0) * t.powi(3) - c * t.powi(2)
            }
            Self::EaseOutBack => {
                let c = 1.70158;
                1.0 + (c + 1.0) * (t - 1.0).powi(3) + c * (t - 1.0).powi(2)
            }
            Self::EaseInOutBack => {
                let c = 1.70158 * 1.525;
                if t < 0.5 {
                    (2.0 * t).powi(2) * ((c + 1.0) * 2.0 * t - c) / 2.0
                } else {
                    ((2.0 * t - 2.0).powi(2) * ((c + 1.0) * (t * 2.0 - 2.0) + c) + 2.0) / 2.0
                }
            }
            Self::EaseInElastic => {
                if t == 0.0 { 0.0 }
                else if t == 1.0 { 1.0 }
                else { -2.0_f32.powf(10.0 * t - 10.0) * ((t * 10.0 - 10.75) * (2.0 * PI / 3.0)).sin() }
            }
            Self::EaseOutElastic => {
                if t == 0.0 { 0.0 }
                else if t == 1.0 { 1.0 }
                else { 2.0_f32.powf(-10.0 * t) * ((t * 10.0 - 0.75) * (2.0 * PI / 3.0)).sin() + 1.0 }
            }
            Self::EaseOutBounce => Self::ease_out_bounce(t),
            Self::CubicBezier(x1, y1, x2, y2) => Self::cubic_bezier(t, *x1, *y1, *x2, *y2),
        }
    }
    
    fn ease_in_cubic(t: f32) -> f32 { t * t * t }
    fn ease_out_cubic(t: f32) -> f32 { 1.0 - (1.0 - t).powi(3) }
    fn ease_in_out_cubic(t: f32) -> f32 {
        if t < 0.5 { 4.0 * t * t * t }
        else { 1.0 - (-2.0 * t + 2.0).powi(3) / 2.0 }
    }
    
    fn ease_out_bounce(t: f32) -> f32 {
        let n1 = 7.5625;
        let d1 = 2.75;
        
        if t < 1.0 / d1 { n1 * t * t }
        else if t < 2.0 / d1 { let t = t - 1.5 / d1; n1 * t * t + 0.75 }
        else if t < 2.5 / d1 { let t = t - 2.25 / d1; n1 * t * t + 0.9375 }
        else { let t = t - 2.625 / d1; n1 * t * t + 0.984375 }
    }
    
    fn cubic_bezier(t: f32, x1: f32, y1: f32, x2: f32, y2: f32) -> f32 {
        let cx = 3.0 * x1;
        let bx = 3.0 * (x2 - x1) - cx;
        let ax = 1.0 - cx - bx;
        let cy = 3.0 * y1;
        let by = 3.0 * (y2 - y1) - cy;
        let ay = 1.0 - cy - by;
        
        let sample_x = |t: f32| ((ax * t + bx) * t + cx) * t;
        let sample_y = |t: f32| ((ay * t + by) * t + cy) * t;
        
        let mut t_guess = t;
        for _ in 0..8 {
            let x = sample_x(t_guess) - t;
            if x.abs() < 0.0001 { break; }
            let dx = (3.0 * ax * t_guess + 2.0 * bx) * t_guess + cx;
            if dx.abs() < 0.0001 { break; }
            t_guess -= x / dx;
        }
        
        sample_y(t_guess.clamp(0.0, 1.0))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_linear() {
        assert_eq!(EasingFunction::Linear.apply(0.0), 0.0);
        assert_eq!(EasingFunction::Linear.apply(0.5), 0.5);
        assert_eq!(EasingFunction::Linear.apply(1.0), 1.0);
    }
    
    #[test]
    fn test_ease_in_out() {
        let ease = EasingFunction::EaseInOut;
        assert!(ease.apply(0.25) < 0.25);
        assert!((ease.apply(0.5) - 0.5).abs() < 0.01);
        assert!(ease.apply(0.75) > 0.75);
    }
    
    #[test]
    fn test_boundaries() {
        for easing in [
            EasingFunction::EaseIn,
            EasingFunction::EaseOut,
            EasingFunction::EaseInOutQuad,
            EasingFunction::EaseOutBounce,
        ] {
            assert!((easing.apply(0.0) - 0.0).abs() < 0.01);
            assert!((easing.apply(1.0) - 1.0).abs() < 0.01);
        }
    }
}
