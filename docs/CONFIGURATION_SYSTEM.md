# Ketivee OS Dual Configuration System

## Overview

The Ketivee OS uses a dual configuration system to handle different types of settings:

- **`.ncf`** (Network Configuration Format) - INI-style for low-level system and boot settings
- **`.tie`** (Technology Integration Environment) - JSON-style for rich application configurations

## File Formats

### .ncf (Network Configuration Format)

INI-style configuration files for low-level system settings, boot configuration, and hardware detection.

**Features:**
- Tree-like structure connecting all system branches
- Section-based organization `[section]`
- Key-value pairs `key = value`
- Comments with `#`
- Minimal parser for bootloader compatibility

**Example:**
```ini
[system]
name = "Ketivee OS"
version = "1.0.0"
architecture = "x86_64"

[network]
hostname = "ketivee-system"
dhcp_enabled = "true"
dns_servers = "8.8.8.8,8.8.4.4"

[security]
firewall = "enabled"
encryption = "aes256"
sandbox_level = "strict"
```

### .tie (Technology Integration Environment)

JSON-style configuration files for applications, installation automation, and rich settings.

**Features:**
- JSON-based structure for complex configurations
- Installation automation with recipes
- App permissions and sandbox settings
- Feature flags and dependencies
- Cross-platform compatibility

**Example:**
```json
{
  "app_info": {
    "name": "Zepra Browser",
    "version": "1.0.0",
    "developer": "Ketivee Team"
  },
  "dependencies": {
    "system": ["libcurl", "sdl2", "webkit2gtk"]
  },
  "sandbox": {
    "level": "trusted",
    "memory_limit": "512MB",
    "network_connections": 10
  },
  "install_recipe": "mkdir /opt/zepra\ncopy zepra_browser /opt/zepra/\ninstall libcurl-dev"
}
```

## Usage

### Loading Configurations

```cpp
#include "config_manager.h"

zepra::ConfigManager config;

// Load NCF file
if (config.loadConfig("system.ncf", zepra::ConfigFileType::NCF)) {
    std::string osName = config.getValue("system", "name");
    std::string version = config.getValue("system", "version");
}

// Load TIE file
if (config.loadConfig("app.tie", zepra::ConfigFileType::TIE)) {
    std::string appName = config.getValue("app_info", "name");
    std::string installRecipe = config.getInstallRecipe();
}
```

### Modifying Configurations

```cpp
// Set values
config.setValue("network", "hostname", "new-hostname");
config.setValue("security", "firewall", "enabled");

// Remove values
config.removeValue("network", "old_setting");

// Save changes
config.saveConfig("modified.ncf", zepra::ConfigFileType::NCF);
```

### Installation Automation

```cpp
// Process installation recipe from TIE file
if (config.processInstallRecipe()) {
    std::cout << "Installation completed successfully" << std::endl;
}
```

## Configuration Categories

### System Configuration (.ncf)

**Core Sections:**
- `[system]` - OS name, version, architecture
- `[hardware]` - CPU, memory, GPU detection
- `[boot]` - Bootloader settings, kernel parameters
- `[network]` - Network configuration, DNS, hostname
- `[security]` - Firewall, encryption, sandbox levels
- `[services]` - System services status
- `[storage]` - File system, swap, cache settings
- `[performance]` - CPU governor, memory settings
- `[development]` - Development tools and SDK paths

### Application Configuration (.tie)

**Core Sections:**
- `app_info` - Application metadata
- `dependencies` - System and development dependencies
- `installation` - Installation type and settings
- `permissions` - File system, network, system access
- `sandbox` - Security and resource limits
- `features` - Feature flags and capabilities
- `ui` - User interface settings
- `security` - Security and privacy settings
- `performance` - Performance optimization
- `install_recipe` - Automated installation commands

## Installation Recipes

TIE files can contain installation recipes that automate the installation process:

```bash
# Recipe commands
mkdir /opt/app                    # Create directory
copy app_binary /opt/app/         # Copy files
install dependency_package        # Install dependencies
create_desktop_entry /opt/app/app # Create desktop entry
register_mime_types              # Register file associations
update_desktop_database          # Update system database
```

## Security Features

### Sandbox Integration

Both configuration types integrate with the sandbox system:

```cpp
// NCF - System-wide sandbox settings
[sandbox]
default_level = "strict"
memory_limit = "512MB"
network_access = "restricted"

// TIE - App-specific sandbox settings
"sandbox": {
    "level": "trusted",
    "memory_limit": "1GB",
    "network_connections": 20,
    "file_handles": 100
}
```

### Permission Management

TIE files define app permissions:

```json
"permissions": {
    "file_system": {
        "read": ["/home/*", "/tmp/*"],
        "write": ["/home/*/Downloads"],
        "execute": []
    },
    "network": {
        "allowed_hosts": ["*.ketivee.org"],
        "ports": ["80", "443"]
    }
}
```

## Testing

Run the configuration test to verify functionality:

```bash
# Build the test
cmake --build . --target config_test

# Run the test
./bin/config_test
```

The test will:
1. Load example .ncf and .tie files
2. Read and modify configuration values
3. Process installation recipes
4. Validate configurations
5. Create new configuration files

## Integration with Platform Infrastructure

The configuration system integrates with:

- **SandboxManager** - Security and resource management
- **PlatformInfrastructure** - App store, video, cloud platforms
- **KetiveeSearch** - Search engine configuration
- **WebKit Engine** - Browser engine settings

## Future Extensions

### Planned Features:
- Configuration validation schemas
- Configuration encryption
- Remote configuration management
- Configuration versioning
- Configuration templates
- Multi-language support
- Configuration backup and restore

### Platform Integration:
- App store configuration management
- Video platform settings
- Cloud platform configuration
- CDN and analytics settings
- User preference management

## Best Practices

1. **Use .ncf for system-level settings** that need to be accessible during boot
2. **Use .tie for application configurations** with rich features and automation
3. **Validate configurations** before applying them
4. **Use installation recipes** for automated deployment
5. **Follow security guidelines** for permission settings
6. **Document configuration options** in comments
7. **Version control configurations** for tracking changes
8. **Test configurations** in sandboxed environments

## Examples

See the `configs/` directory for example configuration files:
- `system.ncf` - Complete system configuration
- `zepra_browser.tie` - Browser application configuration
- Generated test files from running `config_test` 