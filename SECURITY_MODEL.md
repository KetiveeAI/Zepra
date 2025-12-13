# Zepra Browser — Strict Mode Security Policy

Zepra operates in **Strict Mode by default**, prioritizing:
- User privacy
- Predictable behavior
- Performance stability
- Reduced attack surface

Strict Mode is designed to prevent abuse while preserving core web functionality.

---

## 🛡️ Strict Web Rules

Strict Mode protects users from:
- Cross-site tracking
- Cookie theft
- Malicious redirects
- Spam prompts and overlays
- UX manipulation

---

## 🚫 Content Policy

### Blocked or Restricted Content

| Content Type | Action | Reason |
|-------------|--------|--------|
| Third-party cookies | **BLOCK** | Tracking prevention |
| Popup windows | **Require user gesture** | Anti-spam |
| Auto-playing media | **Muted by default** | UX protection |
| Audio playback | **Requires user gesture** | Abuse prevention |
| Notification prompts | **1 per session** | Anti-spam |
| Downloads | **Require user gesture** | Malware prevention |
| Redirect chains | **Max 3 hops** | Anti-phishing |

---

## 📢 Ad Behavior Policy

Zepra enforces **behavior-based ad rules**, not ad blocking.

| Ad Behavior | Action |
|------------|--------|
| Overlay ads (cover content) | BLOCK |
| Interstitial ads (pre-content) | BLOCK |
| Auto-expanding ads | BLOCK |
| Fake close buttons | BLOCK |
| Inline static ads | ALLOW |
| Sidebar ads | ALLOW |

---

## 🔒 Cookie Policy

Default: First-party only
Third-party: BLOCKED
SameSite: Strict (default)
HttpOnly: Enforced for sensitive cookies
Partitioning: Per top-level origin

yaml
Copy code

### Cookie Theft Prevention
- No `document.cookie` access in cross-origin contexts
- Cookie access restricted to same-site frames
- Automatic expiry heuristics for tracking behavior

---

## 📜 JavaScript Restrictions

### Blocked or Restricted APIs (Strict Mode)

```javascript
document.cookie                 // Blocked in cross-origin
window.open()                   // Requires user gesture
Notification.requestPermission() // Limited calls
navigator.geolocation           // Coarse accuracy only
Rate-Limited APIs
API	Limit
alert()	3 per page
confirm()	3 per page
prompt()	1 per page
window.open()	1 per gesture

⚙️ Implementation Model
ContentPolicy Interface
cpp
Copy code
class ContentPolicy {
public:
    enum class Mode { Strict, Compatible };

    bool allowThirdPartyCookies() const;
    bool allowPopups() const;
    bool allowAutoplayMuted() const;
    bool allowAudioPlayback() const;
    int maxRedirects() const;
};
