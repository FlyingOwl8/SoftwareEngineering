SERVICES     = user-service folder-file-service folder-consumer
BUILD_DIR    = build_debug
BUILD_RELEASE = build_release

.PHONY: all build-debug build-release dist-clean clean help

all: build-debug

# ─── BUILD ───────────────────────────────────────────────────────────────────

build-debug:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo \
		-DUSERVER_SANITIZE="" \
		-DUSERVER_FEATURE_POSTGRESQL=OFF \
		-DUSERVER_FEATURE_REDIS=OFF \
		-DUSERVER_FEATURE_GRPC=OFF \
		-DUSERVER_FEATURE_MONGODB=OFF \
		-DUSERVER_FEATURE_TESTSUITE=OFF
	@cd $(BUILD_DIR) && make -j$$(nproc) $(SERVICES)

build-release:
	@mkdir -p $(BUILD_RELEASE)
	@cd $(BUILD_RELEASE) && cmake .. \
		-DCMAKE_BUILD_TYPE=Release \
		-DUSERVER_FEATURE_POSTGRESQL=OFF \
		-DUSERVER_FEATURE_REDIS=OFF \
		-DUSERVER_FEATURE_GRPC=OFF \
		-DUSERVER_FEATURE_MONGODB=OFF \
		-DUSERVER_FEATURE_TESTSUITE=OFF
	@cd $(BUILD_RELEASE) && make -j$$(nproc) $(SERVICES)

# ─── CLEAN ───────────────────────────────────────────────────────────────────

clean:
	@rm -rf $(BUILD_DIR) $(BUILD_RELEASE)

dist-clean: clean
	@echo "Cleaned build directories"

# ─── HELP ────────────────────────────────────────────────────────────────────

help:
	@echo "Targets:"
	@echo "  build-debug    Build both services (debug)"
	@echo "  build-release  Build both services (release)"
	@echo "  dist-clean     Remove all build artifacts"
