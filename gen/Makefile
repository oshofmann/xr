include make.conf

MAKE_ARGS = CC=$(GCC_PY) vmlinux

all: build

build:
	$(MAKE) -C $(BUILD_DIR) $(MAKE_ARGS)

clean:
	$(MAKE) -C $(BUILD_DIR) clean
	rm -rf $(OUTPUT_BASE)/*

.PHONY: build
