#include "common/firmware.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace {

void Check(bool value, const char* message) {
	if (!value) {
		std::fprintf(stderr, "FirmwareTests: failed: %s\n", message);
		std::abort();
	}
}

using Firmware::Version;

void TestParseAndFormat() {
	Check(Firmware::Parse("10") == Version {10, 0, 0}, "major only");
	Check(Firmware::Parse("10.2") == Version {10, 20, 0}, "one minor digit means tens");
	Check(Firmware::Parse("10.20") == Version {10, 20, 0}, "major.minor");
	Check(Firmware::Parse("1.14.01") == Version {1, 14, 1}, "major.minor.patch");
	Check(!Firmware::Parse(""), "empty");
	Check(!Firmware::Parse("100.00"), "three-digit major");
	Check(!Firmware::Parse("10.200"), "three-digit minor");
	Check(!Firmware::Parse("10.x"), "letters");
	Check(!Firmware::Parse("10.20.1"), "one-digit patch");
	Check(Firmware::ToString({10, 20, 0}) == "10.20", "format without patch");
	Check(Firmware::ToString({1, 5, 2}) == "1.05.02", "format with patch");
}

void TestEncoding() {
	Check(Firmware::Encode({10, 20, 0}) == 0x10200000u, "encode 10.20");
	Check(Firmware::Encode({3, 50, 0}) == 0x03500000u, "encode 3.50 like the SDK checks");
	Check(Firmware::Decode(0x10200000u) == Version {10, 20, 0}, "decode 10.20");
	Check(!Firmware::Decode(0x1a000000u), "non-decimal nibble");
	Check(Firmware::ParseRequiredSystemSoftwareVersion("0x0114000000000000") == Version {1, 14, 0},
	      "param.json 1.14");
	Check(Firmware::ParseRequiredSystemSoftwareVersion("0x1020010000000000") ==
	          Version {10, 20, 1},
	      "param.json 10.20.01");
	Check(!Firmware::ParseRequiredSystemSoftwareVersion("0x0114"), "short param.json value");
	Check(Version {10, 0, 0} < Version {10, 20, 0} && Version {9, 99, 0} < Version {10, 0, 0},
	      "ordering");
}

void TestProfileUpdate() {
	const auto path = std::filesystem::temp_directory_path() / "ppss5_firmware_test.ini";
	std::filesystem::remove(path);

	Check(Firmware::InstalledVersion(path) == Firmware::DEFAULT_VERSION, "default without file");

	Version previous;
	Check(Firmware::Update(path, {11, 0, 0}, false, &previous) == Firmware::UpdateResult::Updated,
	      "upgrade");
	Check(previous == Firmware::DEFAULT_VERSION, "previous is the default");
	Check(Firmware::InstalledVersion(path) == Version {11, 0, 0}, "upgrade persisted");

	Check(Firmware::Update(path, {11, 0, 0}, false, nullptr) ==
	          Firmware::UpdateResult::AlreadyInstalled,
	      "same version");
	Check(Firmware::Update(path, {10, 0, 0}, false, nullptr) ==
	          Firmware::UpdateResult::DowngradeRefused,
	      "downgrade refused");
	Check(Firmware::InstalledVersion(path) == Version {11, 0, 0}, "refused downgrade kept file");
	Check(Firmware::Update(path, {10, 0, 0}, true, nullptr) == Firmware::UpdateResult::Updated,
	      "forced downgrade");
	Check(Firmware::InstalledVersion(path) == Version {10, 0, 0}, "forced downgrade persisted");

	{
		std::ofstream out(path, std::ios::trunc);
		out << "# comment\n  version = 12.40 \r\n";
	}
	Check(Firmware::LoadProfile(path) == Version {12, 40, 0}, "hand-edited profile");

	std::filesystem::remove(path);
}

void TestActive() {
	Firmware::SetActive({12, 0, 0});
	Check(Firmware::GetActive() == Version {12, 0, 0}, "active version");
}

} // namespace

int main() {
	TestParseAndFormat();
	TestEncoding();
	TestProfileUpdate();
	TestActive();
	std::puts("FirmwareTests: passed");
	return 0;
}
