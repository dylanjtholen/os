.SUFFIXES:

override OUTPUT := kernel

# User controllable toolchain and toolchain prefix.
TOOLCHAIN := x86_64-elf
TOOLCHAIN_PREFIX :=
ifneq ($(TOOLCHAIN),)
    ifeq ($(TOOLCHAIN_PREFIX),)
        TOOLCHAIN_PREFIX := $(TOOLCHAIN)-
    endif
endif

# User controllable C compiler command.
ifneq ($(TOOLCHAIN_PREFIX),)
    CC := $(TOOLCHAIN_PREFIX)gcc
else
    CC := cc
endif

# User controllable linker command.
LD := $(TOOLCHAIN_PREFIX)ld

# Defaults overrides for variables if using "llvm" as toolchain.
ifeq ($(TOOLCHAIN),llvm)
    CC := clang
    LD := ld.lld
endif

# User controllable C flags.
CFLAGS := -g -O2 -pipe

# User controllable C preprocessor flags. We set none by default.
CPPFLAGS :=

# User controllable nasm flags.
NASMFLAGS := -g

# User controllable linker flags. We set none by default.
LDFLAGS :=

# Check if CC is Clang.
override CC_IS_CLANG := $(shell ! $(CC) --version 2>/dev/null | grep -q '^Target: '; echo $$?)

# If the C compiler is Clang, set the target as needed.
ifeq ($(CC_IS_CLANG),1)
    override CC += \
        -target x86_64-unknown-none-elf
endif

# Internal C flags that should not be changed by the user.
override CFLAGS += \
    -Wall \
    -Wextra \
    -std=gnu11 \
    -ffreestanding \
    -fno-stack-protector \
    -fno-stack-check \
    -fno-lto \
    -fno-PIC \
    -ffunction-sections \
    -fdata-sections \
    -m64 \
    -march=x86-64 \
    -mabi=sysv \
    -mno-80387 \
    -mno-mmx \
    -mno-sse \
    -mno-sse2 \
    -mno-red-zone \
    -mcmodel=kernel

# Internal C preprocessor flags that should not be changed by the user.
override CPPFLAGS := \
    -I kernel \
    $(CPPFLAGS) \
    -MMD \
    -MP

# Internal nasm flags that should not be changed by the user.
override NASMFLAGS := \
    -f elf64 \
    $(patsubst -g,-g -F dwarf,$(NASMFLAGS)) \
    -Wall

# Internal linker flags that should not be changed by the user.
override LDFLAGS += \
    -m elf_x86_64 \
    -nostdlib \
    -static \
    -z max-page-size=0x1000 \
    --gc-sections \
    -T linker.lds

# Use "find" to glob all *.c, *.S, and *.asm files in the tree and obtain the
# object and header dependency file names.
override SRCFILES := $(shell find -L kernel -type f 2>/dev/null | LC_ALL=C sort)
override CFILES := $(filter %.c,$(SRCFILES))
override ASFILES := $(filter %.S,$(SRCFILES))
override NASMFILES := $(filter %.asm,$(SRCFILES))
override OBJ := $(addprefix obj/,$(CFILES:.c=.c.o) $(ASFILES:.S=.S.o) $(NASMFILES:.asm=.asm.o))
override HEADER_DEPS := $(addprefix obj/,$(CFILES:.c=.c.d) $(ASFILES:.S=.S.d))

# Default target. This must come first, before header dependencies.
.PHONY: all
all: bin/$(OUTPUT)

# Include header dependencies.
-include $(HEADER_DEPS)

# Link rules for the final executable.
bin/$(OUTPUT): GNUmakefile linker.lds $(OBJ)
	mkdir -p "$(dir $@)"
	$(LD) $(LDFLAGS) $(OBJ) -o $@

# Compilation rules for *.c files.
obj/%.c.o: %.c GNUmakefile
	mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Compilation rules for *.S files.
obj/%.S.o: %.S GNUmakefile
	mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Compilation rules for *.asm (nasm) files.
obj/%.asm.o: %.asm GNUmakefile
	mkdir -p "$(dir $@)"
	nasm $(NASMFLAGS) $< -o $@

# Remove object files and the final executable.
.PHONY: clean
clean:
	rm -rf bin obj

# --- ISO/image target ----------------------------------------------------
.PHONY: iso
LIMINE_REPO := https://codeberg.org/Limine/Limine.git
LIMINE_DIR := limine
LIMINE_BRANCH := v11.x-binary
ISO_ROOT := iso_root
ISO_OUT := image.iso

# Build and produce a bootable ISO using Limine. This will only clone Limine
# if the `limine` directory does not exist; subsequent runs will reuse it.
iso: bin/$(OUTPUT)
	# Clone limine iff missing
	if [ ! -d "$(LIMINE_DIR)" ]; then \
		git clone $(LIMINE_REPO) $(LIMINE_DIR) --branch=$(LIMINE_BRANCH) --depth=1; \
	fi
	# Build limine utility
	make -C $(LIMINE_DIR)

	# prepare iso root
	mkdir -p $(ISO_ROOT)/boot/limine
	mkdir -p $(ISO_ROOT)/EFI/BOOT

	# copy kernel/binary
	cp -v bin/$(OUTPUT) $(ISO_ROOT)/boot/

	# copy limine files
	cp -v limine.conf $(ISO_ROOT)/boot/ || true
	cp -v $(LIMINE_DIR)/limine-bios.sys $(ISO_ROOT)/boot/limine/
	cp -v $(LIMINE_DIR)/limine-bios-cd.bin $(ISO_ROOT)/boot/limine/
	cp -v $(LIMINE_DIR)/limine-uefi-cd.bin $(ISO_ROOT)/boot/limine/ || true

	# copy EFI binaries if present
	if [ -f $(LIMINE_DIR)/BOOTX64.EFI ]; then \
		cp -v $(LIMINE_DIR)/BOOTX64.EFI $(ISO_ROOT)/EFI/BOOT/; \
	fi
	if [ -f $(LIMINE_DIR)/BOOTIA32.EFI ]; then \
		cp -v $(LIMINE_DIR)/BOOTIA32.EFI $(ISO_ROOT)/EFI/BOOT/; \
	fi

	# build the ISO
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(ISO_ROOT) -o $(ISO_OUT)

	# Install Limine stage1/2 for legacy BIOS boot
	./$(LIMINE_DIR)/limine bios-install $(ISO_OUT) || true
