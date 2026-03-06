OUT_DIR := ./out

$(OUT_DIR)/main: main.c
	@mkdir -p $(@D)
	gcc -o ./out/main main.c -g

