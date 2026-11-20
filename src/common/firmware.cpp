#include "common/firmware.h"

#include <atomic>
#include <charconv>
#include <cstdio>
#include <fstream>
#include <system_error>

namespace Firmware {

static std::atomic<uint32_t> g_active {Encode(DEFAULT_VERSION)};

static bool ParseField(std::string_view text, uint32_t* out, bool two_digits_are_tens) {
	if (text.empty() || text.size() > 2) {
		return false;
	}
	uint32_t value    = 0;
	auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
	if (error != std::errc {} || end != text.data() + text.size()) {
		return false;
	}
	// "10.2" means 10.20, the way the console shows it.
	if (two_digits_are_tens && text.size() == 1) {
		value *= 10;
	}
	*out = value;
	return true;
}

std::optional<Version> Parse(std::string_view text) {
	Version v;

	const auto first = text.find('.');
	if (!ParseField(text.substr(0, first), &v.major, false)) {
		return std::nullopt;
	}
	if (first == std::string_view::npos) {
		return v;
	}

	text.remove_prefix(first + 1);
	const auto second = text.find('.');
	if (!ParseField(text.substr(0, second), &v.minor, true)) {
		return std::nullopt;
	}
	if (second == std::string_view::npos) {
		return v;
	}

	text.remove_prefix(second + 1);
	if (text.size() != 2 || !ParseField(text, &v.patch, false)) {
		return std::nullopt;
	}
	return v;
}

std::string ToString(const Version& version) {
	char buf[16] {};
	if (version.patch != 0) {
		std::snprintf(buf, sizeof(buf), "%u.%02u.%02u", version.major, version.minor, version.patch);
	} else {
		std::snprintf(buf, sizeof(buf), "%u.%02u", version.major, version.minor);
	}
	return buf;
}

static uint32_t ToBcd(uint32_t value) {
	return ((value / 10u) << 4u) | (value % 10u);
}

static std::optional<uint32_t> FromBcd(uint32_t nibbles) {
	const uint32_t hi = (nibbles >> 4u) & 0xfu;
	const uint32_t lo = nibbles & 0xfu;
	if (hi > 9 || lo > 9) {
		return std::nullopt;
	}
	return hi * 10u + lo;
}

uint32_t Encode(const Version& version) {
	return (ToBcd(version.major % 100u) << 24u) | (ToBcd(version.minor % 100u) << 16u) |
	       (ToBcd(version.patch % 100u) << 8u);
}

std::optional<Version> Decode(uint32_t packed) {
	auto major = FromBcd(packed >> 24u);
	auto minor = FromBcd(packed >> 16u);
	auto patch = FromBcd(packed >> 8u);
	if (!major || !minor || !patch) {
		return std::nullopt;
	}
	return Version {*major, *minor, *patch};
}

std::optional<Version> ParseRequiredSystemSoftwareVersion(std::string_view text) {
	if (text.size() != 18 || text[0] != '0' || (text[1] != 'x' && text[1] != 'X')) {
		return std::nullopt;
	}
	uint32_t packed   = 0;
	const char* begin = text.data() + 2;
	auto [end, error] = std::from_chars(begin, begin + 8, packed, 16);
	if (error != std::errc {} || end != begin + 8) {
		return std::nullopt;
	}
	return Decode(packed);
}

std::filesystem::path DefaultProfilePath() {
	return std::filesystem::path(PROFILE_FILE_NAME);
}

static std::string_view Trim(std::string_view s) {
	while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) {
		s.remove_prefix(1);
	}
	while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) {
		s.remove_suffix(1);
	}
	return s;
}

std::optional<Version> LoadProfile(const std::filesystem::path& path) {
	std::ifstream in(path);
	if (!in) {
		return std::nullopt;
	}
	std::string line;
	while (std::getline(in, line)) {
		std::string_view view = Trim(line);
		if (view.empty() || view.front() == '#' || view.front() == ';') {
			continue;
		}
		const auto eq = view.find('=');
		if (eq == std::string_view::npos || Trim(view.substr(0, eq)) != "version") {
			continue;
		}
		return Parse(Trim(view.substr(eq + 1)));
	}
	return std::nullopt;
}

bool SaveProfile(const std::filesystem::path& path, const Version& version) {
	std::ofstream out(path, std::ios::trunc);
	if (!out) {
		return false;
	}
	out << "# PPSS5 virtual firmware. This is the system software version reported to games.\n"
	       "# Raise it with: ppss5 --firmware-update <X.YY>\n"
	    << "version=" << ToString(version) << "\n";
	return static_cast<bool>(out);
}

Version InstalledVersion(const std::filesystem::path& path) {
	return LoadProfile(path).value_or(DEFAULT_VERSION);
}

UpdateResult Update(const std::filesystem::path& path, const Version& target, bool allow_downgrade,
                    Version* previous) {
	const Version current = InstalledVersion(path);
	if (previous != nullptr) {
		*previous = current;
	}
	if (target == current && LoadProfile(path).has_value()) {
		return UpdateResult::AlreadyInstalled;
	}
	if (target < current && !allow_downgrade) {
		return UpdateResult::DowngradeRefused;
	}
	return SaveProfile(path, target) ? UpdateResult::Updated : UpdateResult::WriteFailed;
}

void SetActive(const Version& version) {
	g_active.store(Encode(version));
}

Version GetActive() {
	return Decode(g_active.load()).value_or(DEFAULT_VERSION);
}

} // namespace Firmware
