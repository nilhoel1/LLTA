# Modular MSP430 Makefile
# Usage: make TEST=cnt [target]

# Toolchain Paths
CROSS_COMPILER_DIR = ../../externalDeps/MSP430/msp430-gcc/bin
CROSS_COMPILER = $(CROSS_COMPILER_DIR)/msp430-elf
IR_TOOLCHAIN = ../../build/bin
MSP_SUPPORT_FILES = ../../externalDeps/MSP430/msp430-gcc-support-files/include
MSP_DRIVERLIB = ../../externalDeps/MSP430/msp430-driverlib

# Compilation Flags
MSP_DEVICE = msp430fr5994
CCFLAGS = -D__MSP430F5529__ -O0 -gdwarf-4 -gstrict-dwarf -fno-dwarf2-cfi-asm \
          --target=msp430 -Wall -I$(MSP_SUPPORT_FILES) -I. -Xclang -disable-O0-optnone \
          -I$(MSP_DRIVERLIB)

# Optimization Passes
OPTFLAGS = -passes='mem2reg,instcombine,loop-simplify,loop-rotate,indvars' -S #,loop-unroll

# Backend Flags
LLCFLAGS = --dwarf-version=4 --strict-dwarf -mtriple=msp430 -mcpu=msp430x \
           -filetype=asm

# Linker Flags
LDFLAGS = -T $(MSP_SUPPORT_FILES)/$(MSP_DEVICE).ld \
          -L $(MSP_SUPPORT_FILES) \
          -Og -g -gdwarf-4 -gstrict-dwarf -Wall -mlarge \
          -Wl,--gc-sections -Wl,--start-group -lgcc -lc -Wl,--end-group

# Directories and Inputs
ifndef TEST
$(error TEST argument is required. Usage: make TEST=cnt)
endif

MODULES = ../srcMaelardalen
SOURCE = $(MODULES)/$(TEST).c
BUILD_DIR = build_$(TEST)

# Targets
.PHONY: all clean download analyze

all: $(BUILD_DIR)/$(TEST).dump

# Full WCET analysis target
analyze: $(BUILD_DIR)/$(TEST).wcet

# 0. Create Build Dir
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 1. Compile C to LLVM IR (Unoptimized)
$(BUILD_DIR)/$(TEST).ll: $(SOURCE) | $(BUILD_DIR)
	@echo "CLANG    $< -> $@"
	@$(IR_TOOLCHAIN)/clang $(CCFLAGS) -emit-llvm -S $< -o $@

# 2. Optimized IR
$(BUILD_DIR)/$(TEST).opt.ll: $(BUILD_DIR)/$(TEST).ll
	@echo "OPT      $< -> $@"
	@$(IR_TOOLCHAIN)/opt $(OPTFLAGS) $< -o $@

# 3. LLTA Transformation (LLC Mode - Call Splitter) -> Assembly
$(BUILD_DIR)/$(TEST).S: $(BUILD_DIR)/$(TEST).opt.ll
	@echo "LLTA-LLC $< -> $@"
	@$(IR_TOOLCHAIN)/llta -llc $(LLCFLAGS) $< -o $@
	@echo "FIXUP    $@"
	@sed -i '' 's/call[ 	]*#/call /g' $@
	@grep -v '^[ 	]*\.cfi' $@ > $@.tmp && mv $@.tmp $@
	@grep -v '^[ 	]*\.file' $@ > $@.tmp && mv $@.tmp $@
	@grep -v '^[ 	]*\.loc' $@ > $@.tmp && mv $@.tmp $@

# 4. Assemble to Object (using GCC)
$(BUILD_DIR)/$(TEST).o: $(BUILD_DIR)/$(TEST).S
	@echo "GCC-ASM  $< -> $@"
	@$(CROSS_COMPILER)-gcc -mlarge -c $< -o $@

# 6. Link to ELF
$(BUILD_DIR)/$(TEST).elf: $(BUILD_DIR)/$(TEST).o
	@echo "LINK     $< -> $@"
	@$(CROSS_COMPILER)-gcc $(LDFLAGS) -o $@ $<

# 7. Object Dump
$(BUILD_DIR)/$(TEST).dump: $(BUILD_DIR)/$(TEST).elf
	@echo "DUMP     $< -> $@"
	@$(CROSS_COMPILER)-objdump -D $< > $@

clean:
	rm -rf build_*

download:
	../../scripts/download_MSP430.sh

# 8. Generate Loop Bounds JSON using clang plugin
# The plugin outputs to $filename.loop_bounds.json in cwd
$(BUILD_DIR)/$(TEST).loop_bounds.json: $(SOURCE) | $(BUILD_DIR)
	@echo "LOOPBND  $< -> $@"
	@echo "LOOPBND  $< -> $@"
	@$(abspath $(IR_TOOLCHAIN))/clang -cc1 -triple msp430 \
		-I$(abspath $(MSP_SUPPORT_FILES)) -I$(abspath $(MSP_DRIVERLIB)) \
		-load $(abspath $(IR_TOOLCHAIN))/../lib/LoopBoundPlugin.dylib \
		-plugin loop-bound \
		-plugin-arg-loop-bound verbose \
		$(abspath $<) > /dev/null 2>&1 || true
	@if [ -f "$(abspath $(SOURCE)).loop_bounds.json" ]; then \
		mv "$(abspath $(SOURCE)).loop_bounds.json" $@; \
	else \
		echo '{"loop_bounds":[]}' > $@; \
	fi

# 9. LLTA WCET Analysis (full analysis with loop bounds and dump file)
$(BUILD_DIR)/$(TEST).wcet: $(BUILD_DIR)/$(TEST).opt.ll $(BUILD_DIR)/$(TEST).dump $(BUILD_DIR)/$(TEST).loop_bounds.json
	@echo "LLTA-WCET $< -> $@"
	@cd $(BUILD_DIR) && \
		$(abspath $(IR_TOOLCHAIN))/llta \
		-loop-bounds-json=$(abspath $(BUILD_DIR)/$(TEST).loop_bounds.json) \
		-dump-file=$(abspath $(BUILD_DIR)/$(TEST).dump) \
		$(LLCFLAGS) $(abspath $<) -o /dev/null 2>&1 | tee $(abspath $@)
