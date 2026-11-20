#ifndef KYTY_COMMON_FIRMWARE_H_
#define KYTY_COMMON_FIRMWARE_H_

// PPSS5 virtual firmware.
//
// The emulator is a high-level emulator: it does not load or need any Sony system software.
// Games still ask the "console" which system software version it runs, and some refuse to boot
// when the answer is lower than the version they were built for. The virtual firmware is the
// version the emulator reports. It is a plain number, so updating it means raising that number
// (`--firmware-update <X.YY>`), not installing a PS5UPDATE.PUP.

#include <compare>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace Firmware {

struct Version {
	uint32_t major = 0; // 0-99
	uint32_t minor = 0; // 0-99
	uint32_t patch = 0; // 0-99

	auto operator<=>(const Version&) const = default;
};

// Firmware reported when no profile file and no --firmware override exist.
inline constexpr Version DEFAULT_VERSION {10, 0, 0};

// Profile file that stores the installed virtual firmware, relative to the working directory.
inline constexpr const char* PROFILE_FILE_NAME = "ppss5_firmware.ini";

// Accepts "10", "10.2", "10.20" and "10.20.01".
std::optional<Version> Parse(std::string_view text);

// "10.20", or "10.20.01" when the patch is not zero.
std::string ToString(const Version& version);

// Sony's packed form: decimal digits as hex nibbles, 0xMMmmpp00 (10.20 -> 0x10200000).
uint32_t                Encode(const Version& version);
std::optional<Version>  Decode(uint32_t packed);

// Decodes the param.json "requiredSystemSoftwareVersion" string ("0x0114000000000000" -> 1.14).
std::optional<Version>  ParseRequiredSystemSoftwareVersion(std::string_view text);

std::filesystem::path   DefaultProfilePath();
std::optional<Version>  LoadProfile(const std::filesystem::path& path);
bool                    SaveProfile(const std::filesystem::path& path, const Version& version);

// Installed version from the profile, or DEFAULT_VERSION when there is none.
Version                 InstalledVersion(const std::filesystem::path& path);

enum class UpdateResult { Updated, AlreadyInstalled, DowngradeRefused, WriteFailed };

// Writes `target` to the profile. Lower versions are refused unless allow_downgrade is set.
UpdateResult Update(const std::filesystem::path& path, const Version& target,
                    bool allow_downgrade, Version* previous);

// Version reported to the running game.
void    SetActive(const Version& version);
Version GetActive();

} // namespace Firmware

#endif /* KYTY_COMMON_FIRMWARE_H_ */
