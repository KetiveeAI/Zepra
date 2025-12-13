/**
 * @file ssl_error_overlay.cpp
 * @brief SSL error overlay implementation
 */

#include "webcore/ssl_error_overlay.hpp"
#include <sstream>

namespace Zepra::WebCore {

SSLErrorOverlay::SSLErrorOverlay() = default;
SSLErrorOverlay::~SSLErrorOverlay() = default;

void SSLErrorOverlay::show(const SSLErrorInfo& info, SSLErrorCallback callback) {
    currentError_ = info;
    callback_ = std::move(callback);
    visible_ = true;
}

void SSLErrorOverlay::hide() {
    visible_ = false;
}

std::string SSLErrorOverlay::getOverlayHTML() const {
    return generateSSLWarningHTML(currentError_);
}

void SSLErrorOverlay::handleAction(const std::string& action) {
    if (!callback_) return;
    
    if (action == "back") {
        callback_(SSLErrorDecision::GoBack);
    } else if (action == "proceed" && currentError_.allowProceed) {
        callback_(SSLErrorDecision::Proceed);
    } else if (action == "learn") {
        callback_(SSLErrorDecision::LearnMore);
    }
    
    hide();
}

SSLErrorType SSLErrorOverlay::fromCertResult(Networking::CertVerifyResult result) {
    using namespace Networking;
    
    switch (result) {
        case CertVerifyResult::OK:
            return SSLErrorType::None;
        case CertVerifyResult::EXPIRED:
            return SSLErrorType::Expired;
        case CertVerifyResult::NOT_YET_VALID:
            return SSLErrorType::NotYetValid;
        case CertVerifyResult::HOSTNAME_MISMATCH:
            return SSLErrorType::HostnameMismatch;
        case CertVerifyResult::SELF_SIGNED:
            return SSLErrorType::SelfSigned;
        case CertVerifyResult::UNTRUSTED_ROOT:
            return SSLErrorType::UntrustedRoot;
        case CertVerifyResult::REVOKED:
            return SSLErrorType::Revoked;
        default:
            return SSLErrorType::Unknown;
    }
}

std::string SSLErrorOverlay::getErrorTitle(SSLErrorType type) {
    switch (type) {
        case SSLErrorType::Expired:
            return "Certificate Expired";
        case SSLErrorType::NotYetValid:
            return "Certificate Not Yet Valid";
        case SSLErrorType::HostnameMismatch:
            return "Certificate Name Mismatch";
        case SSLErrorType::SelfSigned:
            return "Self-Signed Certificate";
        case SSLErrorType::UntrustedRoot:
            return "Unknown Certificate Authority";
        case SSLErrorType::Revoked:
            return "Certificate Revoked";
        case SSLErrorType::WeakKey:
            return "Weak Security Configuration";
        case SSLErrorType::MixedContent:
            return "Mixed Content Warning";
        default:
            return "Security Error";
    }
}

std::string SSLErrorOverlay::getErrorDescription(SSLErrorType type) {
    switch (type) {
        case SSLErrorType::Expired:
            return "The site's security certificate has expired. This could mean someone is trying to impersonate the site.";
        case SSLErrorType::NotYetValid:
            return "The site's security certificate is not yet valid. Your device's date might be incorrect.";
        case SSLErrorType::HostnameMismatch:
            return "The site's certificate belongs to a different domain. This could indicate a misconfigured site or an attack.";
        case SSLErrorType::SelfSigned:
            return "This site's certificate was signed by itself, not a trusted certificate authority.";
        case SSLErrorType::UntrustedRoot:
            return "The certificate authority is not recognized or trusted by your system.";
        case SSLErrorType::Revoked:
            return "This site's certificate has been revoked, indicating it should no longer be trusted.";
        case SSLErrorType::WeakKey:
            return "The site uses outdated or weak security that doesn't adequately protect your data.";
        case SSLErrorType::MixedContent:
            return "This secure page includes content from insecure sources.";
        default:
            return "An unknown security error occurred while connecting to this site.";
    }
}

bool SSLErrorOverlay::canBypass(SSLErrorType type) {
    switch (type) {
        case SSLErrorType::Revoked:
            return false;  // Never bypass revoked
        case SSLErrorType::Expired:
        case SSLErrorType::NotYetValid:
        case SSLErrorType::SelfSigned:
        case SSLErrorType::HostnameMismatch:
        case SSLErrorType::UntrustedRoot:
            return true;   // Allow with warning
        default:
            return false;
    }
}

std::string generateSSLWarningHTML(const SSLErrorInfo& info) {
    std::ostringstream html;
    
    std::string title = SSLErrorOverlay::getErrorTitle(info.type);
    std::string description = SSLErrorOverlay::getErrorDescription(info.type);
    bool canProceed = SSLErrorOverlay::canBypass(info.type);
    
    html << R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Security Warning - )" << title << R"(</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            color: #fff;
        }
        .container {
            max-width: 600px;
            padding: 40px;
            text-align: center;
        }
        .icon {
            width: 80px;
            height: 80px;
            background: #e74c3c;
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            margin: 0 auto 20px;
            font-size: 40px;
        }
        h1 {
            font-size: 28px;
            margin-bottom: 16px;
            color: #e74c3c;
        }
        .description {
            font-size: 16px;
            line-height: 1.6;
            color: #b0b0c0;
            margin-bottom: 30px;
        }
        .url {
            background: rgba(255,255,255,0.1);
            padding: 12px 20px;
            border-radius: 8px;
            font-family: monospace;
            margin-bottom: 30px;
            word-break: break-all;
        }
        .details {
            background: rgba(0,0,0,0.3);
            border-radius: 8px;
            padding: 20px;
            text-align: left;
            margin-bottom: 30px;
            font-size: 14px;
        }
        .details dt {
            color: #888;
            margin-bottom: 4px;
        }
        .details dd {
            margin-bottom: 12px;
            color: #ddd;
        }
        .buttons {
            display: flex;
            gap: 16px;
            justify-content: center;
            flex-wrap: wrap;
        }
        button {
            padding: 14px 28px;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            cursor: pointer;
            transition: all 0.2s;
        }
        .btn-primary {
            background: #3498db;
            color: white;
        }
        .btn-primary:hover {
            background: #2980b9;
        }
        .btn-danger {
            background: transparent;
            border: 2px solid #e74c3c;
            color: #e74c3c;
        }
        .btn-danger:hover {
            background: #e74c3c;
            color: white;
        }
        .btn-link {
            background: none;
            color: #888;
            text-decoration: underline;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="icon">⚠️</div>
        <h1>)" << title << R"(</h1>
        <p class="description">)" << description << R"(</p>
        
        <div class="url">)" << info.url << R"(</div>
        
        <div class="details">
            <dl>
                <dt>Hostname</dt>
                <dd>)" << info.hostname << R"(</dd>
                <dt>Issuer</dt>
                <dd>)" << info.issuer << R"(</dd>
                <dt>Valid From</dt>
                <dd>)" << info.validFrom << R"(</dd>
                <dt>Valid To</dt>
                <dd>)" << info.validTo << R"(</dd>
            </dl>
        </div>
        
        <div class="buttons">
            <button class="btn-primary" onclick="zepra.ssl.goBack()">Go Back (Recommended)</button>
)";

    if (canProceed) {
        html << R"(
            <button class="btn-danger" onclick="zepra.ssl.proceed()">Proceed Anyway (Unsafe)</button>
)";
    }

    html << R"(
            <button class="btn-link" onclick="zepra.ssl.learn()">Learn More</button>
        </div>
    </div>
</body>
</html>
)";

    return html.str();
}

} // namespace Zepra::WebCore
