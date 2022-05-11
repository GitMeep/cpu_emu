# Alternative GNU Make workspace makefile autogenerated by Premake

ifndef config
  config=debug
endif

ifndef verbose
  SILENT = @
endif

ifeq ($(config),debug)
  cpuemu_config = debug

else ifeq ($(config),release)
  cpuemu_config = release

else
  $(error "invalid configuration $(config)")
endif

PROJECTS := cpuemu

.PHONY: all clean help $(PROJECTS) 

all: $(PROJECTS)

cpuemu:
ifneq (,$(cpuemu_config))
	@echo "==== Building cpuemu ($(cpuemu_config)) ===="
	@${MAKE} --no-print-directory -C . -f cpuemu.make config=$(cpuemu_config)
endif

clean:
	@${MAKE} --no-print-directory -C . -f cpuemu.make clean

help:
	@echo "Usage: make [config=name] [target]"
	@echo ""
	@echo "CONFIGURATIONS:"
	@echo "  debug"
	@echo "  release"
	@echo ""
	@echo "TARGETS:"
	@echo "   all (default)"
	@echo "   clean"
	@echo "   cpuemu"
	@echo ""
	@echo "For more information, see https://github.com/premake/premake-core/wiki"