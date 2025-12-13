# Zepra Browser - Strict Mode Security Policy

## 🛡️ Strict Web Rules

Zepra operates in **Strict Mode** by default to protect users from:
- Cookie stealing and tracking
- Excessive ads and popups
- User experience exploitation

---

## 🚫 Content Policy

### Blocked Content
| Content Type | Action | Reason |
|--------------|--------|--------|
| **Third-party cookies** | BLOCK | Tracking prevention |
| **Popup windows** | REQUIRE gesture | Anti-spam |
| **Auto-playing video** | MUTE by default | UX protection |
| **Notification prompts** | LIMIT 1/session | Anti-spam |
| **Download prompts** | REQUIRE gesture | Security |
| **Redirect chains** | LIMIT 3 max | Anti-phishing |

### Ad Policy
| Behavior | Action |
|----------|--------|
| Overlay ads (cover content) | BLOCK |
| Interstitial ads (before content) | BLOCK |
| Auto-expand ads | BLOCK |
| Ads with fake close buttons | BLOCK |
| Inline static ads | ALLOW |
| Sidebar ads | ALLOW |

---

## 🔒 Cookie Policy

```
Default: First-party only
Third-party: BLOCKED
SameSite: Strict by default
HttpOnly: Enforced for sensitive cookies
```

### Cookie Theft Prevention
- No `document.cookie` access for cross-origin frames
- Cookie partitioning per top-level origin
- Automatic expiry for tracking cookies

---

## 📜 JavaScript Restrictions

### Blocked APIs (Strict Mode)
```javascript
// These throw SecurityError in Strict Mode
document.cookie        // Read-only in cross-origin
window.open()          // Requires user gesture
Notification.requestPermission()  // Limited calls
navigator.geolocation  // Coarse only
```

### Rate-Limited APIs
| API | Limit |
|-----|-------|
| `alert()` | 3 per page |
| `confirm()` | 3 per page |
| `prompt()` | 1 per page |
| `window.open()` | 1 per gesture |

---

## 🎯 Implementation

### ContentPolicy class
```cpp
class ContentPolicy {
public:
    enum class Mode { Strict, Compatible };
    
    bool allowThirdPartyCookies() const;
    bool allowPopups() const;
    bool allowAutoplay() const;
    int maxRedirects() const;
};
```

### Per-Site Overrides
Users can grant specific sites additional permissions:
- Allow cookies
- Allow popups
- Allow notifications

---

## ✅ User Benefits

1. **Privacy**: No cross-site tracking
2. **Performance**: Faster page loads (blocked ads)
3. **Security**: No cookie theft
4. **UX**: No annoying popups/overlays
