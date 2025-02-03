CHAOS_DIR = CHAOS

GEM5_REPO = https://github.com/gem5/gem5
GEM5_DIR = gem5
CONFIG = RISCV/gem5.opt
BUILD_DIR = $(GEM5_DIR)/build/$(CONFIG)

RISC_V_GNU_TOOLCHAIN_REPO = https://github.com/riscv-collab/riscv-gnu-toolchain.git
RISC_V_GNU_TOOLCHAIN_DIR = riscv-gnu-toolchain
RISC_V_GNU_TOOLCHAIN_CONFIG_DIR = /opt/riscv

all: install_requirements clone_gem5 move_chaos install_gem5_requirements build_gem5 clone_riscv_toolchain build_riscv_toolchain copy_riscv_lib

install_requirements:
	@apt-get update
	@apt-get install build-essential git m4 scons zlib1g zlib1g-dev libprotobuf-dev protobuf-compiler libprotoc-dev libgoogle-perftools-dev emacs wget python3-dev libboost-all-dev pkg-config bison python3-venv meson ninja-build libglib2.0-dev libpixman-1-dev gawk texinfo flex libgmp-dev libmpc-dev libmpfr-dev lftp python3-pip python3-six
	@apt-get install --only-upgrade libnccl2

clone_gem5:
	@if [ ! -d "$(GEM5_DIR)" ]; then \
		git clone $(GEM5_REPO) --recursive; \
	else \
		echo "gem5 already found."; \
	fi

move_chaos:
	@if [ -d "$(CHAOS_DIR)" ]; then \
		cp -r $(CHAOS_DIR) $(GEM5_DIR)/src/; \
	else \
		echo "CHAOS folder not found, does it exists?"; \
		exit 1; \
	fi

install_gem5_requirements:
	@echo "Installing Python dependencies..."
	@pip install -r $(GEM5_DIR)/requirements.txt

build_gem5:
	@cd $(GEM5_DIR) && \
		scons $(BUILD_DIR) -j$(shell nproc) && \
			cd ..

clone_riscv_toolchain:
	@if [ ! -d "$(RISC_V_GNU_TOOLCHAIN_DIR)" ]; then \
		git clone --recursive $(RISC_V_GNU_TOOLCHAIN_REPO); \
	else \
		echo "riscv-gnu-toolchain already found."; \
	fi

build_riscv_toolchain:
	@cd $(RISC_V_GNU_TOOLCHAIN_DIR) && \
		./configure --prefix=$(RISC_V_GNU_TOOLCHAIN_CONFIG_DIR) && \
		make linux -j$(shell nproc) && \
		cd ..

copy_riscv_lib:
	@cp -r $(RISC_V_GNU_TOOLCHAIN_CONFIG_DIR)/sysroot/lib/* /lib/

.PHONY: all install_requirements clone_gem5 move_chaos install_gem5_requirements build_gem5 clone_riscv_toolchain build_riscv_toolchain copy_riscv_lib