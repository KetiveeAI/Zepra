// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#pragma once

#include "common/types.h"
#include "../config/dual_config.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>

namespace zepra {

// Sandbox Security Levels
enum class SandboxLevel {
    UNTRUSTED,      // No system access
    LIMITED,        // Basic file access
    TRUSTED,        // Full file access
    SYSTEM,         // System-level access
    ADMIN           // Administrative access
};

// Sandbox Resource Limits
struct SandboxLimits {
    size_t maxMemoryMB = 512;
    size_t maxDiskSpaceMB = 1024;
    size_t maxCPUTimeSeconds = 300;
    size_t maxNetworkConnections = 10;
    size_t maxFileHandles = 100;
    size_t maxThreads = 20;
    size_t maxProcesses = 5;
    bool allowFileSystem = false;
    bool allowNetwork = false;
    bool allowSystemCalls = false;
    bool allowRegistry = false;
    bool allowUI = false;
    bool allowAudio = false;
    bool allowVideo = false;
    bool allowPrinting = false;
    bool allowClipboard = false;
    bool allowSensors = false;
    bool allowCamera = false;
    bool allowMicrophone = false;
    bool allowLocation = false;
    
    SandboxLimits() = default;
};

// Sandbox Process Information
struct SandboxProcess {
    String processId;
    String appName;
    String appVersion;
    String executablePath;
    String workingDirectory;
    SandboxLevel securityLevel;
    SandboxLimits limits;
    std::vector<String> arguments;
    std::unordered_map<String, String> environment;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point lastActivity;
    size_t memoryUsage;
    size_t cpuUsage;
    size_t diskUsage;
    size_t networkUsage;
    bool isRunning;
    bool isSuspended;
    int exitCode;
    String errorMessage;
    
    SandboxProcess() : memoryUsage(0), cpuUsage(0), diskUsage(0), networkUsage(0), 
                       isRunning(false), isSuspended(false), exitCode(0) {}
};

// Sandbox File System
struct SandboxFileSystem {
    String rootPath;
    String tempPath;
    String cachePath;
    String dataPath;
    String configPath;
    std::vector<String> allowedPaths;
    std::vector<String> blockedPaths;
    std::unordered_map<String, String> mountPoints;
    bool readOnly;
    bool encrypted;
    
    SandboxFileSystem() : readOnly(false), encrypted(false) {}
};

// Sandbox Network
struct SandboxNetwork {
    bool enabled;
    String proxyHost;
    int proxyPort;
    std::vector<String> allowedHosts;
    std::vector<String> blockedHosts;
    std::vector<String> allowedPorts;
    std::vector<String> blockedPorts;
    std::vector<String> allowedProtocols;
    bool sslRequired;
    bool certificateValidation;
    
    SandboxNetwork() : enabled(false), proxyPort(0), sslRequired(true), certificateValidation(true) {}
};

// Sandbox Security Policy
struct SandboxSecurityPolicy {
    String policyId;
    String policyName;
    String description;
    SandboxLevel defaultLevel;
    SandboxLimits defaultLimits;
    std::vector<String> allowedAPIs;
    std::vector<String> blockedAPIs;
    std::vector<String> allowedLibraries;
    std::vector<String> blockedLibraries;
    std::unordered_map<String, String> environmentVariables;
    std::vector<String> requiredPermissions;
    bool codeSigningRequired;
    bool integrityChecking;
    bool antiTampering;
    bool processIsolation;
    bool memoryProtection;
    bool stackProtection;
    bool heapProtection;
    
    SandboxSecurityPolicy() : codeSigningRequired(false), integrityChecking(true), 
                              antiTampering(true), processIsolation(true), 
                              memoryProtection(true), stackProtection(true), 
                              heapProtection(true) {}
};

// Sandbox Manager
class SandboxManager {
public:
    SandboxManager();
    ~SandboxManager();
    
    // Initialization
    bool initialize();
    void shutdown();
    bool isInitialized() const;
    
    // Sandbox Creation
    String createSandbox(const String& appName, const String& appVersion, 
                        SandboxLevel level = SandboxLevel::UNTRUSTED);
    bool destroySandbox(const String& sandboxId);
    bool resetSandbox(const String& sandboxId);
    
    // Process Management
    String launchProcess(const String& sandboxId, const String& executablePath, 
                        const std::vector<String>& arguments = {});
    bool terminateProcess(const String& processId);
    bool suspendProcess(const String& processId);
    bool resumeProcess(const String& processId);
    bool killProcess(const String& processId);
    
    // Process Information
    SandboxProcess getProcessInfo(const String& processId) const;
    std::vector<SandboxProcess> getAllProcesses() const;
    std::vector<SandboxProcess> getSandboxProcesses(const String& sandboxId) const;
    bool isProcessRunning(const String& processId) const;
    
    // Resource Monitoring
    void updateResourceUsage(const String& processId);
    SandboxLimits getCurrentLimits(const String& processId) const;
    bool checkResourceLimits(const String& processId) const;
    void enforceResourceLimits(const String& processId);
    
    // File System Management
    bool mountFileSystem(const String& sandboxId, const String& sourcePath, 
                        const String& targetPath, bool readOnly = false);
    bool unmountFileSystem(const String& sandboxId, const String& targetPath);
    SandboxFileSystem getFileSystem(const String& sandboxId) const;
    bool createDirectory(const String& sandboxId, const String& path);
    bool deleteFile(const String& sandboxId, const String& path);
    bool copyFile(const String& sandboxId, const String& source, const String& destination);
    String readFile(const String& sandboxId, const String& path);
    bool writeFile(const String& sandboxId, const String& path, const String& content);
    
    // Network Management
    bool configureNetwork(const String& sandboxId, const SandboxNetwork& config);
    SandboxNetwork getNetworkConfig(const String& sandboxId) const;
    bool allowNetworkAccess(const String& sandboxId, const String& host, int port);
    bool blockNetworkAccess(const String& sandboxId, const String& host, int port);
    bool isNetworkAllowed(const String& sandboxId, const String& host, int port) const;
    
    // Security Policy Management
    bool setSecurityPolicy(const String& sandboxId, const SandboxSecurityPolicy& policy);
    SandboxSecurityPolicy getSecurityPolicy(const String& sandboxId) const;
    bool validateSecurityPolicy(const SandboxSecurityPolicy& policy) const;
    bool enforceSecurityPolicy(const String& sandboxId);
    
    // API Interception
    void registerAPIInterceptor(const String& apiName, 
                               std::function<bool(const String&, const std::vector<String>&)> interceptor);
    void unregisterAPIInterceptor(const String& apiName);
    bool interceptAPICall(const String& processId, const String& apiName, 
                         const std::vector<String>& arguments);
    
    // Event System
    void setProcessStartCallback(std::function<void(const SandboxProcess&)> callback);
    void setProcessEndCallback(std::function<void(const SandboxProcess&)> callback);
    void setResourceLimitCallback(std::function<void(const String&, const String&)> callback);
    void setSecurityViolationCallback(std::function<void(const String&, const String&)> callback);
    
    // Monitoring and Logging
    void enableMonitoring(const String& sandboxId, bool enabled);
    bool isMonitoringEnabled(const String& sandboxId) const;
    void setLogLevel(const String& sandboxId, int level);
    std::vector<String> getLogs(const String& sandboxId) const;
    void clearLogs(const String& sandboxId);
    
    // Performance Optimization
    void enableCaching(const String& sandboxId, bool enabled);
    bool isCachingEnabled(const String& sandboxId) const;
    void setCacheSize(const String& sandboxId, size_t size);
    size_t getCacheSize(const String& sandboxId) const;
    void clearCache(const String& sandboxId);
    
    // Error Handling
    void setErrorCallback(std::function<void(const String&, const String&)> callback);
    std::vector<String> getErrors(const String& sandboxId) const;
    void clearErrors(const String& sandboxId);
    
    // Cleanup
    void cleanupInactiveSandboxes();
    void cleanupOrphanedProcesses();
    void cleanupTemporaryFiles();
    
private:
    bool initialized;
    std::unordered_map<String, SandboxProcess> processes;
    std::unordered_map<String, SandboxFileSystem> fileSystems;
    std::unordered_map<String, SandboxNetwork> networks;
    std::unordered_map<String, SandboxSecurityPolicy> policies;
    std::unordered_map<String, SandboxLimits> limits;
    std::unordered_map<String, bool> monitoring;
    std::unordered_map<String, bool> caching;
    std::unordered_map<String, size_t> cacheSizes;
    std::unordered_map<String, std::vector<String>> logs;
    std::unordered_map<String, std::vector<String>> errors;
    
    // Callbacks
    std::function<void(const SandboxProcess&)> processStartCallback;
    std::function<void(const SandboxProcess&)> processEndCallback;
    std::function<void(const String&, const String&)> resourceLimitCallback;
    std::function<void(const String&, const String&)> securityViolationCallback;
    std::function<void(const String&, const String&)> errorCallback;
    
    // API Interceptors
    std::unordered_map<String, std::function<bool(const String&, const std::vector<String>&)>> apiInterceptors;
    
    // Threading
    std::mutex processMutex;
    std::mutex fileSystemMutex;
    std::mutex networkMutex;
    std::mutex policyMutex;
    std::atomic<bool> shutdownRequested;
    std::thread monitorThread;
    
    // Internal methods
    String generateSandboxId() const;
    String generateProcessId() const;
    bool validateSandboxId(const String& sandboxId) const;
    bool validateProcessId(const String& processId) const;
    void monitorThreadFunction();
    void logEvent(const String& sandboxId, const String& event);
    void addError(const String& sandboxId, const String& error);
    bool checkSecurityViolation(const String& processId, const String& operation);
    void enforceProcessIsolation(const String& processId);
    void enforceMemoryProtection(const String& processId);
    void enforceStackProtection(const String& processId);
    void enforceHeapProtection(const String& processId);
    bool verifyCodeIntegrity(const String& executablePath);
    bool verifyCodeSignature(const String& executablePath);
    String encryptData(const String& data) const;
    String decryptData(const String& encryptedData) const;
};

// Platform Infrastructure
class PlatformInfrastructure {
public:
    PlatformInfrastructure();
    ~PlatformInfrastructure();
    
    // Initialization
    bool initialize();
    void shutdown();
    bool isInitialized() const;
    
    // App Store Management
    struct AppInfo {
        String appId;
        String appName;
        String appVersion;
        String description;
        String category;
        String developer;
        String iconPath;
        String executablePath;
        std::vector<String> screenshots;
        std::vector<String> tags;
        double rating;
        int downloads;
        String price;
        bool isFree;
        bool isVerified;
        bool isFeatured;
        std::chrono::system_clock::time_point releaseDate;
        std::chrono::system_clock::time_point lastUpdate;
        
        AppInfo() : rating(0.0), downloads(0), isFree(true), isVerified(false), isFeatured(false) {}
    };
    
    String registerApp(const AppInfo& appInfo);
    bool unregisterApp(const String& appId);
    bool updateApp(const String& appId, const AppInfo& appInfo);
    AppInfo getAppInfo(const String& appId) const;
    std::vector<AppInfo> searchApps(const String& query) const;
    std::vector<AppInfo> getAppsByCategory(const String& category) const;
    std::vector<AppInfo> getFeaturedApps() const;
    std::vector<AppInfo> getTopApps() const;
    std::vector<AppInfo> getRecentApps() const;
    
    // Video Platform Management
    struct VideoInfo {
        String videoId;
        String title;
        String description;
        String category;
        String creator;
        String thumbnailPath;
        String videoPath;
        String subtitlePath;
        std::vector<String> tags;
        int duration;
        String quality;
        String format;
        double rating;
        int views;
        int likes;
        int dislikes;
        bool isPublic;
        bool isPremium;
        std::chrono::system_clock::time_point uploadDate;
        std::chrono::system_clock::time_point lastModified;
        
        VideoInfo() : duration(0), rating(0.0), views(0), likes(0), dislikes(0), 
                      isPublic(true), isPremium(false) {}
    };
    
    String uploadVideo(const VideoInfo& videoInfo);
    bool deleteVideo(const String& videoId);
    bool updateVideo(const String& videoId, const VideoInfo& videoInfo);
    VideoInfo getVideoInfo(const String& videoId) const;
    std::vector<VideoInfo> searchVideos(const String& query) const;
    std::vector<VideoInfo> getVideosByCategory(const String& category) const;
    std::vector<VideoInfo> getTrendingVideos() const;
    std::vector<VideoInfo> getRecommendedVideos(const String& userId) const;
    std::vector<VideoInfo> getSubscribedVideos(const String& userId) const;
    
    // Cloud Platform Management
    struct CloudStorage {
        String userId;
        String storageId;
        String name;
        String path;
        size_t totalSpace;
        size_t usedSpace;
        size_t freeSpace;
        bool isEncrypted;
        bool isShared;
        std::vector<String> sharedWith;
        std::chrono::system_clock::time_point lastSync;
        
        CloudStorage() : totalSpace(0), usedSpace(0), freeSpace(0), 
                         isEncrypted(true), isShared(false) {}
    };
    
    String createCloudStorage(const CloudStorage& storage);
    bool deleteCloudStorage(const String& storageId);
    bool updateCloudStorage(const String& storageId, const CloudStorage& storage);
    CloudStorage getCloudStorage(const String& storageId) const;
    std::vector<CloudStorage> getUserCloudStorages(const String& userId) const;
    bool uploadFile(const String& storageId, const String& localPath, const String& remotePath);
    bool downloadFile(const String& storageId, const String& remotePath, const String& localPath);
    bool deleteFile(const String& storageId, const String& remotePath);
    std::vector<String> listFiles(const String& storageId, const String& path) const;
    bool shareStorage(const String& storageId, const String& userId);
    bool unshareStorage(const String& storageId, const String& userId);
    
    // User Management
    struct UserInfo {
        String userId;
        String username;
        String email;
        String displayName;
        String avatarPath;
        String subscriptionType;
        std::vector<String> permissions;
        std::vector<String> favorites;
        std::vector<String> history;
        std::chrono::system_clock::time_point registrationDate;
        std::chrono::system_clock::time_point lastLogin;
        bool isActive;
        bool isVerified;
        bool isPremium;
        
        UserInfo() : isActive(true), isVerified(false), isPremium(false) {}
    };
    
    String registerUser(const UserInfo& userInfo);
    bool unregisterUser(const String& userId);
    bool updateUser(const String& userId, const UserInfo& userInfo);
    UserInfo getUserInfo(const String& userId) const;
    bool authenticateUser(const String& username, const String& password);
    bool changePassword(const String& userId, const String& oldPassword, const String& newPassword);
    bool resetPassword(const String& email);
    bool verifyEmail(const String& userId, const String& verificationCode);
    
    // Content Delivery Network (CDN)
    struct CDNNode {
        String nodeId;
        String location;
        String ipAddress;
        int port;
        String protocol;
        double bandwidth;
        double latency;
        bool isActive;
        std::vector<String> supportedFormats;
        
        CDNNode() : port(80), bandwidth(0.0), latency(0.0), isActive(true) {}
    };
    
    String addCDNNode(const CDNNode& node);
    bool removeCDNNode(const String& nodeId);
    bool updateCDNNode(const String& nodeId, const CDNNode& node);
    CDNNode getCDNNode(const String& nodeId) const;
    std::vector<CDNNode> getAllCDNNodes() const;
    String getOptimalCDNNode(const String& userLocation) const;
    bool distributeContent(const String& contentId, const std::vector<String>& nodeIds);
    bool removeContent(const String& contentId, const std::vector<String>& nodeIds);
    
    // Analytics and Monitoring
    struct AnalyticsData {
        String metricId;
        String metricName;
        String metricType;
        double value;
        String unit;
        std::chrono::system_clock::time_point timestamp;
        std::unordered_map<String, String> tags;
        
        AnalyticsData() : value(0.0) {}
    };
    
    void recordMetric(const AnalyticsData& metric);
    std::vector<AnalyticsData> getMetrics(const String& metricName, 
                                         std::chrono::system_clock::time_point start,
                                         std::chrono::system_clock::time_point end) const;
    double getAverageMetric(const String& metricName, 
                           std::chrono::system_clock::time_point start,
                           std::chrono::system_clock::time_point end) const;
    std::vector<String> getTopMetrics(const String& metricType, int limit = 10) const;
    
    // Event System
    void setAppInstallCallback(std::function<void(const String&, const AppInfo&)> callback);
    void setVideoUploadCallback(std::function<void(const String&, const VideoInfo&)> callback);
    void setUserRegistrationCallback(std::function<void(const String&, const UserInfo&)> callback);
    void setCDNUpdateCallback(std::function<void(const String&, const CDNNode&)> callback);
    
    // Error Handling
    void setErrorCallback(std::function<void(const String&, const String&)> callback);
    std::vector<String> getErrors() const;
    void clearErrors();
    
private:
    bool initialized;
    std::unique_ptr<SandboxManager> sandboxManager;
    std::unordered_map<String, AppInfo> apps;
    std::unordered_map<String, VideoInfo> videos;
    std::unordered_map<String, CloudStorage> cloudStorages;
    std::unordered_map<String, UserInfo> users;
    std::unordered_map<String, CDNNode> cdnNodes;
    std::vector<AnalyticsData> analytics;
    std::vector<String> errors;
    
    // Callbacks
    std::function<void(const String&, const AppInfo&)> appInstallCallback;
    std::function<void(const String&, const VideoInfo&)> videoUploadCallback;
    std::function<void(const String&, const UserInfo&)> userRegistrationCallback;
    std::function<void(const String&, const CDNNode&)> cdnUpdateCallback;
    std::function<void(const String&, const String&)> errorCallback;
    
    // Internal methods
    void addError(const String& error);
    bool validateAppInfo(const AppInfo& appInfo) const;
    bool validateVideoInfo(const VideoInfo& videoInfo) const;
    bool validateUserInfo(const UserInfo& userInfo) const;
    bool validateCloudStorage(const CloudStorage& storage) const;
    bool validateCDNNode(const CDNNode& node) const;
    String generateUniqueId() const;
    bool checkUserPermissions(const String& userId, const String& permission) const;
    double calculateLatency(const String& userLocation, const String& nodeLocation) const;
    String encryptSensitiveData(const String& data) const;
    String decryptSensitiveData(const String& encryptedData) const;
};

} // namespace zepra 