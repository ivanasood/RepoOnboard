CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
# -Ibackend/vendor lets #include <cjson/cJSON.h> find backend/vendor/cjson/cJSON.h
CPPFLAGS ?= -Ibackend/include -Ibackend/vendor

# Use pkg-config when available (Linux with system libcjson); otherwise the
# vendored cJSON source is compiled in directly so no extra flags are needed.
PKG_CFLAGS := $(shell pkg-config --cflags libcjson 2>/dev/null)
PKG_LIBS   := $(shell pkg-config --libs   libcjson libcurl 2>/dev/null)

# When pkg-config provides nothing for libcurl, fall back to -lcurl which is
# always present on macOS via the system SDK / Xcode command-line tools.
ifeq ($(strip $(PKG_LIBS)),)
  EXTRA_LIBS := -lcurl
else
  EXTRA_LIBS :=
endif

LDFLAGS ?=

# Vendored cJSON is compiled as a regular source file so there is no need for
# a system-installed libcjson on any platform.
VENDOR_SOURCES := backend/vendor/cjson/cJSON.c

COMMON_SOURCES := \
	backend/src/config.c \
	backend/src/github_client.c \
	backend/src/repo_analyzer.c \
	backend/src/response_builder.c \
	backend/src/json_utils.c \
	backend/src/analysis_service.c

HTTP_SOURCES := backend/src/main.c backend/src/http_server.c
MCP_SOURCES  := backend/src/mcp_main.c

COMMON_OBJECTS := $(COMMON_SOURCES:.c=.o)
HTTP_OBJECTS   := $(HTTP_SOURCES:.c=.o)
MCP_OBJECTS    := $(MCP_SOURCES:.c=.o)
VENDOR_OBJECTS := $(VENDOR_SOURCES:.c=.o)

TARGETS := backend/bin/repoonboard backend/bin/repoonboard-mcp

.PHONY: all clean run mcp

all: $(TARGETS)

backend/bin/repoonboard: $(HTTP_OBJECTS) $(COMMON_OBJECTS) $(VENDOR_OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(PKG_LIBS) $(EXTRA_LIBS) -o $@

backend/bin/repoonboard-mcp: $(MCP_OBJECTS) $(COMMON_OBJECTS) $(VENDOR_OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(PKG_LIBS) $(EXTRA_LIBS) -o $@

backend/src/%.o: backend/src/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(PKG_CFLAGS) -c $< -o $@

backend/vendor/cjson/%.o: backend/vendor/cjson/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(PKG_CFLAGS) -c $< -o $@

run: backend/bin/repoonboard
	./backend/bin/repoonboard

mcp: backend/bin/repoonboard-mcp
	./backend/bin/repoonboard-mcp

clean:
	rm -f $(HTTP_OBJECTS) $(MCP_OBJECTS) $(COMMON_OBJECTS) $(VENDOR_OBJECTS) $(TARGETS)
