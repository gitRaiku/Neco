all: build

.PHONY: build run debug clean remove tar

CC      := gcc

TARGET  := neco
SRC_DIR := src
PRT_DIR := protocol
LIB_DIR := lib
BIN_DIR := bin
OBJ_DIR := .obj

C_SRC := $(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/*/*.c)
C_OBJ := $(C_SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

EX_HEADERS := $(SRC_DIR)/vecs.h $(SRC_DIR)/config.h
VALGRIND = valgrind --leak-check=summary --log-file=valgrind.log

DATE          := $(shell date "+%Y-%m-%d")
COMPILE_FLAGS := -ggdb3 -Og -Wall -march=native -mtune=native -fmodulo-sched \
					       -fstack-clash-protection -pthread -pipe \
					       -fkeep-inline-functions -D_FORTIFY_SOURCE=2 -D_GNU_SOURCE

INCLUDE_FLAGS  := 
LIBRARY_FLAGS  := -lwayland-client -lwayland-cursor -lm
LD_FLAGS       := -O2

xdgsh_XML      := $(PRT_DIR)/xml/xdg-shell.xml 
xout_XML       := $(PRT_DIR)/xml/xdg-output.xml
zwlr_XML       := $(PRT_DIR)/xml/wlr-layer-shell-unstable-v1.xml
PROT_NAMES     := xdgsh zwlr xout
PROT_OBJ_NAMES := $(addprefix $(OBJ_DIR)/$(PRT_DIR)/,$(addsuffix -protocol.o,$(PROT_NAMES)))

.SECONDEXPANSION:
$(OBJ_DIR)/$(PRT_DIR):
	mkdir -p $@
$($(addsuffix _XML,$$(PROT_NAMES)): ; # This is retarded as shit
$(PROT_NAMES:%=$(PRT_DIR)/%-protocol.h): $$($$(patsubst $(PRT_DIR)/%-protocol.h,%_XML,$$@))
	wayland-scanner client-header < $($(@:$(PRT_DIR)/%-protocol.h=%)_XML) > $@
$(PROT_NAMES:%=$(PRT_DIR)/%-protocol.c): $$($$(patsubst $(PRT_DIR)/%-protocol.c,%_XML,$$@))
	wayland-scanner private-code  < $($(@:$(PRT_DIR)/%-protocol.c=%)_XML) > $@
$(PROT_OBJ_NAMES): $(OBJ_DIR)/$(PRT_DIR) $$(patsubst $(OBJ_DIR)/%.o,%.c,$$@) $$(patsubst $(OBJ_DIR)/%.o,%.h,$$@)
	$(CC) $(COMPILE_FLAGS) $(INCLUDE_FLAGS) $(LIBRARY_FLAGS) -c -o $@ $(@:$(OBJ_DIR)/%.o=%.c)

$(C_OBJ): $(OBJ_DIR)/%.o : $(SRC_DIR)/%.c $(SRC_DIR)/%.h $(EX_HEADERS)
	@mkdir -p $(@D)
	$(CC) -c $(COMPILE_FLAGS) $(INCLUDE_FLAGS) $< -o $@

$(BIN_DIR)/$(TARGET): $(C_OBJ) $(PROT_OBJ_NAMES)
	$(CC) $(C_OBJ) $(PROT_OBJ_NAMES) $(LD_FLAGS) $(LIBRARY_FLAGS) -o $@

$(BIN_DIR):
	mkdir -p $@

build: $(BIN_DIR) $(PROT_OBJ_NAMES) $(BIN_DIR)/$(TARGET)

install: build
	mkdir -p /usr/share/wl-neco
	cp neco.gif /usr/share/wl-neco/neco.gif
	 cp $(BIN_DIR)/$(TARGET) /usr/local/bin/$(TARGET)

uninstall:
	rm -rf /usr/share/wl-neco
	rm -f /usr/local/bin/$(TARGET)

run: build
	# $(BIN_DIR)/$(TARGET) neco.gif
	$(BIN_DIR)/$(TARGET) -s 0.01 neco.gif
	# $(BIN_DIR)/$(TARGET) -s 9.0 ~/Misc/Downloads/neco/neco-bed.gif
	# $(BIN_DIR)/$(TARGET) ~/Misc/Downloads/neco/neco-bed.gif

valgrind: build
	$(VALGRIND) $(BIN_DIR)/$(TARGET) -s 0.01

debug: build
	gdb -q --args $(BIN_DIR)/$(TARGET) neco.gif

clean:
	sudo rm -f  $(C_OBJ)
	sudo rm -f  $(PROT_NAMES:%=$(PRT_DIR)/%-protocol.h)
	sudo rm -f  $(PROT_NAMES:%=$(PRT_DIR)/%-protocol.c)
	sudo rm -rf $(OBJ_DIR)/$(PRT_DIR)
	sudo rm -f $(BIN_DIR)/$(TARGET)

