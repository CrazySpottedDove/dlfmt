VCPKG = vcpkg
VCPKG_TOOLCHAIN = $(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake
BUILD_DIR_RELEASE = build-release
BUILD_DIR_DEBUG = build-debug
BUILD_DIR_RELWITHDBGINFO = build-relwithdbginfo
BUILD_DIR_ASAN = build-asan

.PHONY: all vcpkg clean debug release help build release-win asan relwithdebinfo bin-extension-dlfmt

all: vcpkg
	cmake -DCMAKE_TOOLCHAIN_FILE=$(VCPKG_TOOLCHAIN) -B $(BUILD_DIR_RELEASE) -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -S .
	cmake --build $(BUILD_DIR_RELEASE) --config Release

vcpkg:
# 	$(VCPKG) install $(VCPKG_PKGS)
	$(VCPKG) install

build:
	cmake --build $(BUILD_DIR_RELEASE) --config Release

release:
	cmake -DCMAKE_TOOLCHAIN_FILE=$(VCPKG_TOOLCHAIN) -B $(BUILD_DIR_RELEASE) -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -S .
	cmake --build $(BUILD_DIR_RELEASE)

asan:
	cmake -DCMAKE_TOOLCHAIN_FILE=$(VCPKG_TOOLCHAIN) -B $(BUILD_DIR_ASAN) -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -S . -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g"
	cmake --build $(BUILD_DIR_ASAN)

relwithdebinfo:
	cmake -DCMAKE_TOOLCHAIN_FILE=$(VCPKG_TOOLCHAIN) -B $(BUILD_DIR_RELWITHDBGINFO) -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -S .
	cmake --build $(BUILD_DIR_RELWITHDBGINFO)

debug:
	cmake -DCMAKE_TOOLCHAIN_FILE=$(VCPKG_TOOLCHAIN) -B $(BUILD_DIR_DEBUG) -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -S .
	cmake --build $(BUILD_DIR_DEBUG)

clean:
	rm -rf $(BUILD_DIR_RELEASE)
	rm -rf $(BUILD_DIR_DEBUG)
	rm -rf $(BUILD_DIR_RELWITHDBGINFO)
	rm -rf $(BUILD_DIR_ASAN)

help:
	@echo "Makefile commands:"
	@echo "  all      - Build the project in Release mode (default)"
	@echo "  vcpkg    - Install required packages using vcpkg"
	@echo "  release  - Build the project in Release mode"
	@echo "  debug    - Build the project in Debug mode"
	@echo "  clean    - Remove the build directory"
	@echo "  help     - Show this help message"

# release-win:
# 	cmake -DCMAKE_TOOLCHAIN_FILE="./toolchains/mingw-windows-x64.cmake" -DCMAKE_BUILD_TYPE=Release -S . -B build-win
# 	cmake --build build-win --config Release

# bin-extension-dlfmt: release release-win
# 	cp $(BUILD_DIR_RELEASE)/dlfmt extension/dlfmt/bin/linux/
# 	cp build-win/dlfmt.exe extension/dlfmt/bin/win32/
