//! NXRENDER C FFI Bindings
//!
//! Complete FFI for GPU, Theme, Input, Touch, Gesture, Window

use std::ffi::{CStr, CString};
use std::os::raw::c_char;

use nxgfx::{GpuContext, Color, Rect, Point, Size};
use nxgfx::sysinfo::SystemInfo;
use nxrender_theme::Theme;
use nxrender_input::{
    Event, MouseButton, Modifiers, KeyCode,
    MouseState, KeyboardState, TouchState,
    GestureRecognizer, GesturePattern, GestureAction, GestureTranslator,
};

// ============= TYPES =============

#[repr(C)]
pub struct NxColor { pub r: u8, pub g: u8, pub b: u8, pub a: u8 }

#[repr(C)]
pub struct NxRect { pub x: f32, pub y: f32, pub width: f32, pub height: f32 }

#[repr(C)]
pub struct NxPoint { pub x: f32, pub y: f32 }

#[repr(C)]
pub struct NxSystemInfo {
    pub gpu_name: *mut c_char,
    pub gpu_vendor: *mut c_char,
    pub gpu_driver: *mut c_char,
    pub display_width: u32,
    pub display_height: u32,
    pub display_refresh: u32,
    pub display_name: *mut c_char,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub enum NxMouseButton { Left = 0, Right = 1, Middle = 2 }

#[repr(C)]
#[derive(Clone, Copy)]
pub enum NxGestureType {
    None = 0,
    Tap = 1,
    DoubleTap = 2,
    LongPress = 3,
    Pan = 4,
    SwipeLeft = 5,
    SwipeRight = 6,
    SwipeUp = 7,
    SwipeDown = 8,
    PinchIn = 9,
    PinchOut = 10,
    Rotate = 11,
}

#[repr(C)]
pub struct NxGestureResult {
    pub gesture_type: NxGestureType,
    pub x: f32,
    pub y: f32,
    pub scale: f32,
    pub rotation: f32,
    pub velocity_x: f32,
    pub velocity_y: f32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub enum NxGestureAction {
    None = 0,
    Back = 1,
    Forward = 2,
    Refresh = 3,
    ZoomIn = 4,
    ZoomOut = 5,
    ZoomReset = 6,
    NextTab = 7,
    PrevTab = 8,
    CloseTab = 9,
    FullScreen = 10,
}

// ============= SYSTEM DETECTION =============

#[no_mangle]
pub extern "C" fn nx_detect_system() -> *mut NxSystemInfo {
    let sys = SystemInfo::detect();
    let gpu = sys.gpus.first().cloned().unwrap_or_else(|| nxgfx::sysinfo::GpuInfo {
        name: "Unknown".into(), vendor: "Unknown".into(), driver: "Unknown".into(), vram_mb: 0,
    });
    let display = sys.displays.first().cloned().unwrap_or_else(|| nxgfx::sysinfo::DisplayInfo {
        name: "Unknown".into(), resolution: (1920, 1080), refresh_rate: 60, primary: true, connected: true,
    });
    Box::into_raw(Box::new(NxSystemInfo {
        gpu_name: CString::new(gpu.name).unwrap().into_raw(),
        gpu_vendor: CString::new(gpu.vendor).unwrap().into_raw(),
        gpu_driver: CString::new(gpu.driver).unwrap().into_raw(),
        display_width: display.resolution.0,
        display_height: display.resolution.1,
        display_refresh: display.refresh_rate,
        display_name: CString::new(display.name).unwrap().into_raw(),
    }))
}

#[no_mangle]
pub extern "C" fn nx_free_system_info(info: *mut NxSystemInfo) {
    if info.is_null() { return; }
    unsafe {
        let info = Box::from_raw(info);
        let _ = CString::from_raw(info.gpu_name);
        let _ = CString::from_raw(info.gpu_vendor);
        let _ = CString::from_raw(info.gpu_driver);
        let _ = CString::from_raw(info.display_name);
    }
}

// ============= GPU CONTEXT =============

#[no_mangle]
pub extern "C" fn nx_gpu_create() -> *mut GpuContext {
    GpuContext::new().ok().map(|c| Box::into_raw(Box::new(c))).unwrap_or(std::ptr::null_mut())
}

#[no_mangle]
pub extern "C" fn nx_gpu_create_with_size(width: u32, height: u32) -> *mut GpuContext {
    GpuContext::with_size(Size::new(width as f32, height as f32))
        .ok().map(|c| Box::into_raw(Box::new(c))).unwrap_or(std::ptr::null_mut())
}

#[no_mangle]
pub extern "C" fn nx_gpu_destroy(ctx: *mut GpuContext) {
    if !ctx.is_null() { unsafe { let _ = Box::from_raw(ctx); } }
}

#[no_mangle]
pub extern "C" fn nx_gpu_fill_rect(ctx: *mut GpuContext, rect: NxRect, color: NxColor) {
    if ctx.is_null() { return; }
    unsafe { (*ctx).fill_rect(Rect::new(rect.x, rect.y, rect.width, rect.height), Color::rgba(color.r, color.g, color.b, color.a)); }
}

#[no_mangle]
pub extern "C" fn nx_gpu_fill_rounded_rect(ctx: *mut GpuContext, rect: NxRect, color: NxColor, radius: f32) {
    if ctx.is_null() { return; }
    unsafe { (*ctx).fill_rounded_rect(Rect::new(rect.x, rect.y, rect.width, rect.height), Color::rgba(color.r, color.g, color.b, color.a), radius); }
}

#[no_mangle]
pub extern "C" fn nx_gpu_fill_circle(ctx: *mut GpuContext, x: f32, y: f32, radius: f32, color: NxColor) {
    if ctx.is_null() { return; }
    unsafe { (*ctx).fill_circle(Point::new(x, y), radius, Color::rgba(color.r, color.g, color.b, color.a)); }
}

#[no_mangle]
pub extern "C" fn nx_gpu_draw_text(ctx: *mut GpuContext, text: *const c_char, x: f32, y: f32, color: NxColor) {
    if ctx.is_null() || text.is_null() { return; }
    unsafe { (*ctx).draw_text(&CStr::from_ptr(text).to_string_lossy(), Point::new(x, y), Color::rgba(color.r, color.g, color.b, color.a)); }
}

// draw_line - TODO: implement in GpuContext
// #[no_mangle]
// pub extern "C" fn nx_gpu_draw_line(...)

#[no_mangle]
pub extern "C" fn nx_gpu_present(ctx: *mut GpuContext) {
    if !ctx.is_null() { unsafe { (*ctx).present(); } }
}

#[no_mangle]
pub extern "C" fn nx_gpu_clear(ctx: *mut GpuContext, color: NxColor) {
    if ctx.is_null() { return; }
    unsafe { (*ctx).set_clear_color(Color::rgba(color.r, color.g, color.b, color.a)); (*ctx).clear(); }
}

#[no_mangle]
pub extern "C" fn nx_gpu_resize(ctx: *mut GpuContext, width: u32, height: u32) {
    if ctx.is_null() { return; }
    unsafe { (*ctx).resize(Size::new(width as f32, height as f32)); }
}

// ============= THEME =============

#[no_mangle]
pub extern "C" fn nx_theme_light() -> *mut Theme { Box::into_raw(Box::new(Theme::light())) }

#[no_mangle]
pub extern "C" fn nx_theme_dark() -> *mut Theme { Box::into_raw(Box::new(Theme::dark())) }

#[no_mangle]
pub extern "C" fn nx_theme_destroy(theme: *mut Theme) { if !theme.is_null() { unsafe { let _ = Box::from_raw(theme); } } }

#[no_mangle]
pub extern "C" fn nx_theme_get_primary_color(theme: *const Theme) -> NxColor {
    if theme.is_null() { return NxColor { r: 59, g: 130, b: 246, a: 255 }; }
    unsafe { let c = (*theme).colors.semantic.primary; NxColor { r: c.r, g: c.g, b: c.b, a: c.a } }
}

#[no_mangle]
pub extern "C" fn nx_theme_get_background_color(theme: *const Theme) -> NxColor {
    if theme.is_null() { return NxColor { r: 255, g: 255, b: 255, a: 255 }; }
    unsafe { let c = (*theme).colors.surface.background; NxColor { r: c.r, g: c.g, b: c.b, a: c.a } }
}

#[no_mangle]
pub extern "C" fn nx_theme_get_surface_color(theme: *const Theme) -> NxColor {
    if theme.is_null() { return NxColor { r: 255, g: 255, b: 255, a: 255 }; }
    unsafe { let c = (*theme).colors.surface.surface; NxColor { r: c.r, g: c.g, b: c.b, a: c.a } }
}

#[no_mangle]
pub extern "C" fn nx_theme_get_text_color(theme: *const Theme) -> NxColor {
    if theme.is_null() { return NxColor { r: 0, g: 0, b: 0, a: 255 }; }
    unsafe { let c = (*theme).colors.text.primary; NxColor { r: c.r, g: c.g, b: c.b, a: c.a } }
}

// ============= MOUSE =============

#[no_mangle]
pub extern "C" fn nx_mouse_create() -> *mut MouseState { Box::into_raw(Box::new(MouseState::new())) }

#[no_mangle]
pub extern "C" fn nx_mouse_destroy(mouse: *mut MouseState) { if !mouse.is_null() { unsafe { let _ = Box::from_raw(mouse); } } }

#[no_mangle]
pub extern "C" fn nx_mouse_get_position(mouse: *const MouseState, x: *mut f32, y: *mut f32) {
    if mouse.is_null() { return; }
    unsafe { let pos = (*mouse).position(); *x = pos.x; *y = pos.y; }
}

#[no_mangle]
pub extern "C" fn nx_mouse_is_button_down(mouse: *const MouseState, button: NxMouseButton) -> bool {
    if mouse.is_null() { return false; }
    let btn = match button { NxMouseButton::Left => MouseButton::Left, NxMouseButton::Right => MouseButton::Right, NxMouseButton::Middle => MouseButton::Middle };
    unsafe { (*mouse).is_button_down(btn) }
}

#[no_mangle]
pub extern "C" fn nx_mouse_move(mouse: *mut MouseState, x: f32, y: f32) {
    if mouse.is_null() { return; }
    unsafe { (*mouse).process_event(&Event::MouseMove { pos: Point::new(x, y), modifiers: Modifiers::NONE }); }
}

#[no_mangle]
pub extern "C" fn nx_mouse_button_down(mouse: *mut MouseState, x: f32, y: f32, button: NxMouseButton) {
    if mouse.is_null() { return; }
    let btn = match button { NxMouseButton::Left => MouseButton::Left, NxMouseButton::Right => MouseButton::Right, NxMouseButton::Middle => MouseButton::Middle };
    unsafe { (*mouse).process_event(&Event::MouseDown { pos: Point::new(x, y), button: btn, modifiers: Modifiers::NONE }); }
}

#[no_mangle]
pub extern "C" fn nx_mouse_button_up(mouse: *mut MouseState, x: f32, y: f32, button: NxMouseButton) {
    if mouse.is_null() { return; }
    let btn = match button { NxMouseButton::Left => MouseButton::Left, NxMouseButton::Right => MouseButton::Right, NxMouseButton::Middle => MouseButton::Middle };
    unsafe { (*mouse).process_event(&Event::MouseUp { pos: Point::new(x, y), button: btn, modifiers: Modifiers::NONE }); }
}

// ============= TOUCH =============

#[no_mangle]
pub extern "C" fn nx_touch_create() -> *mut TouchState { Box::into_raw(Box::new(TouchState::new())) }

#[no_mangle]
pub extern "C" fn nx_touch_destroy(touch: *mut TouchState) { if !touch.is_null() { unsafe { let _ = Box::from_raw(touch); } } }

#[no_mangle]
pub extern "C" fn nx_touch_count(touch: *const TouchState) -> u32 {
    if touch.is_null() { return 0; }
    unsafe { (*touch).active_count() as u32 }
}

#[no_mangle]
pub extern "C" fn nx_touch_start(touch: *mut TouchState, id: u64, x: f32, y: f32) {
    if touch.is_null() { return; }
    unsafe { (*touch).process_event(&Event::TouchStart { id, pos: Point::new(x, y) }); }
}

#[no_mangle]
pub extern "C" fn nx_touch_move(touch: *mut TouchState, id: u64, x: f32, y: f32) {
    if touch.is_null() { return; }
    unsafe { (*touch).process_event(&Event::TouchMove { id, pos: Point::new(x, y) }); }
}

#[no_mangle]
pub extern "C" fn nx_touch_end(touch: *mut TouchState, id: u64, x: f32, y: f32) {
    if touch.is_null() { return; }
    unsafe { (*touch).process_event(&Event::TouchEnd { id, pos: Point::new(x, y) }); }
}

#[no_mangle]
pub extern "C" fn nx_touch_get_center(touch: *const TouchState, x: *mut f32, y: *mut f32) -> bool {
    if touch.is_null() { return false; }
    unsafe {
        if let Some(center) = (*touch).center() { *x = center.x; *y = center.y; true } else { false }
    }
}

// ============= GESTURE RECOGNITION =============

#[no_mangle]
pub extern "C" fn nx_gesture_create() -> *mut GestureRecognizer { Box::into_raw(Box::new(GestureRecognizer::new())) }

#[no_mangle]
pub extern "C" fn nx_gesture_destroy(gesture: *mut GestureRecognizer) { if !gesture.is_null() { unsafe { let _ = Box::from_raw(gesture); } } }

#[no_mangle]
pub extern "C" fn nx_gesture_detect(gesture: *const GestureRecognizer, touch: *const TouchState) -> NxGestureResult {
    let empty = NxGestureResult { gesture_type: NxGestureType::None, x: 0.0, y: 0.0, scale: 1.0, rotation: 0.0, velocity_x: 0.0, velocity_y: 0.0 };
    if gesture.is_null() || touch.is_null() { return empty; }
    
    unsafe {
        if let Some(g) = (*gesture).detect_from_touch(&*touch) {
            let gesture_type = match g.gesture_type {
                nxrender_input::GestureType::Tap => NxGestureType::Tap,
                nxrender_input::GestureType::DoubleTap => NxGestureType::DoubleTap,
                nxrender_input::GestureType::LongPress => NxGestureType::LongPress,
                nxrender_input::GestureType::Pan => NxGestureType::Pan,
                nxrender_input::GestureType::Swipe(dir) => match dir {
                    nxrender_input::SwipeDirection::Left => NxGestureType::SwipeLeft,
                    nxrender_input::SwipeDirection::Right => NxGestureType::SwipeRight,
                    nxrender_input::SwipeDirection::Up => NxGestureType::SwipeUp,
                    nxrender_input::SwipeDirection::Down => NxGestureType::SwipeDown,
                },
                nxrender_input::GestureType::Pinch => if g.scale < 1.0 { NxGestureType::PinchIn } else { NxGestureType::PinchOut },
                nxrender_input::GestureType::Rotate => NxGestureType::Rotate,
            };
            NxGestureResult { gesture_type, x: g.position.x, y: g.position.y, scale: g.scale, rotation: g.rotation, velocity_x: g.velocity.x, velocity_y: g.velocity.y }
        } else { empty }
    }
}

// ============= GESTURE TRANSLATION =============

#[no_mangle]
pub extern "C" fn nx_translator_create() -> *mut GestureTranslator { Box::into_raw(Box::new(GestureTranslator::new())) }

#[no_mangle]
pub extern "C" fn nx_translator_destroy(tr: *mut GestureTranslator) { if !tr.is_null() { unsafe { let _ = Box::from_raw(tr); } } }

#[no_mangle]
pub extern "C" fn nx_translator_set_screen_width(tr: *mut GestureTranslator, width: f32) {
    if tr.is_null() { return; }
    unsafe { (*tr).set_screen_width(width); }
}

#[no_mangle]
pub extern "C" fn nx_translator_translate(tr: *const GestureTranslator, gesture: NxGestureType, fingers: u8, edge_x: f32) -> NxGestureAction {
    if tr.is_null() { return NxGestureAction::None; }
    
    let pattern = match gesture {
        NxGestureType::SwipeLeft => GesturePattern::SwipeLeft,
        NxGestureType::SwipeRight => GesturePattern::SwipeRight,
        NxGestureType::SwipeUp => GesturePattern::SwipeUp,
        NxGestureType::SwipeDown => GesturePattern::SwipeDown,
        NxGestureType::PinchIn => GesturePattern::PinchIn,
        NxGestureType::PinchOut => GesturePattern::PinchOut,
        NxGestureType::DoubleTap => GesturePattern::DoubleTap,
        _ => return NxGestureAction::None,
    };
    
    let start_pos = if edge_x >= 0.0 { Some(Point::new(edge_x, 0.0)) } else { None };
    
    unsafe {
        match (*tr).translate(pattern, fingers, start_pos) {
            Some(GestureAction::Back) => NxGestureAction::Back,
            Some(GestureAction::Forward) => NxGestureAction::Forward,
            Some(GestureAction::Refresh) => NxGestureAction::Refresh,
            Some(GestureAction::ZoomIn) => NxGestureAction::ZoomIn,
            Some(GestureAction::ZoomOut) => NxGestureAction::ZoomOut,
            Some(GestureAction::ZoomReset) => NxGestureAction::ZoomReset,
            Some(GestureAction::NextTab) => NxGestureAction::NextTab,
            Some(GestureAction::PrevTab) => NxGestureAction::PrevTab,
            Some(GestureAction::CloseTab) => NxGestureAction::CloseTab,
            Some(GestureAction::FullScreen) => NxGestureAction::FullScreen,
            _ => NxGestureAction::None,
        }
    }
}

// ============= KEYBOARD =============

#[no_mangle]
pub extern "C" fn nx_keyboard_create() -> *mut KeyboardState { Box::into_raw(Box::new(KeyboardState::new())) }

#[no_mangle]
pub extern "C" fn nx_keyboard_destroy(kb: *mut KeyboardState) { if !kb.is_null() { unsafe { let _ = Box::from_raw(kb); } } }

#[no_mangle]
pub extern "C" fn nx_keyboard_is_ctrl(kb: *const KeyboardState) -> bool {
    if kb.is_null() { return false; }
    unsafe { (*kb).is_ctrl() }
}

#[no_mangle]
pub extern "C" fn nx_keyboard_is_shift(kb: *const KeyboardState) -> bool {
    if kb.is_null() { return false; }
    unsafe { (*kb).is_shift() }
}

#[no_mangle]
pub extern "C" fn nx_keyboard_is_alt(kb: *const KeyboardState) -> bool {
    if kb.is_null() { return false; }
    unsafe { (*kb).is_alt() }
}

// ============= VERSION =============

#[no_mangle]
pub extern "C" fn nx_version() -> *const c_char {
    static VERSION: &[u8] = b"NXRENDER 1.0.0\0";
    VERSION.as_ptr() as *const c_char
}
