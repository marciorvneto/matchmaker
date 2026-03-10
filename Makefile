OUT_DIR := ./out

WASM_DIR := ./docs

all: $(OUT_DIR)/main $(WASM_DIR)/matchmaker.js

$(OUT_DIR)/main: main.c
	@mkdir -p $(@D)
	gcc -o ./out/main main.c -g

$(WASM_DIR)/matchmaker.js: main.c | $(WASM_DIR)
	emcc -o $@ main.c -s EXPORTED_FUNCTIONS="['_analyze_system']" -s EXPORTED_RUNTIME_METHODS="['cwrap']" -O3

$(WASM_DIR): 
	@mkdir  -p $@

clean: 
	@rm -rf $(OUT_DIR)
	@rm -rf $(WASM_DIR)


.PHONY: all
