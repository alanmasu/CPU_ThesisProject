# Toolchain
CC      = riscv32-unknown-elf-gcc
OBJDUMP = riscv32-unknown-elf-objdump
READELF = riscv32-unknown-elf-readelf

# Linker script
LINKER  = linker_script.ld



# Compilatore e flag
# CFLAGS = -Iinclude $(addprefix -I, $(wildcard libraries/*/include)) -ffreestanding -nostdlib -Os 
CFLAGS = -Iinclude $(addprefix -I, $(wildcard libraries/*/include)) -ffreestanding -Os --specs=nano.specs -ffunction-sections -fdata-sections -Wl,--gc-sections -DMY_RISCV -Wstack-usage=8000
LDFLAGS = -T $(LINKER) -e main 
# LDFLAGS = -e main

# Directory principali
BIN_DIR = build/bin
SRC_DIR = src
LIB_DIR = libraries
OBJ_DIR = build/obj
TEST_DIR = test
TEST_BIN_DIR = $(BIN_DIR)/test

TEST_FILES = $(wildcard $(TEST_DIR)/*/*.c)

# Nome del file ELF da generare
TARGET = ${BIN_DIR}/main.elf

# Trova tutte le directory src delle librerie
LIBS_SRC_DIRS = $(wildcard $(LIB_DIR)/*/src)

# Trova tutti i file sorgente (principali e librerie)
SRC_FILES = $(wildcard $(SRC_DIR)/*.c) $(foreach dir, $(LIBS_SRC_DIRS), $(wildcard $(dir)/*.c))
LIB_FILES = $(foreach dir, $(LIBS_SRC_DIRS), $(wildcard $(dir)/*.c))


# Mappa i file sorgente ai file oggetto (preserva la struttura della directory)
OBJ_FILES = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRC_FILES))
LIB_OBJS = $(patsubst $(LIB_DIR)/%,$(OBJ_DIR)/libraries/%,$(LIB_FILES:.c=.o))
TEST_OBJS = $(patsubst $(TEST_DIR)/%,$(OBJ_DIR)/test/%,$(TEST_FILES:.c=.o))

# Trova tutte le sottodirectory in test/
TESTS = $(shell find $(TEST_DIR) -mindepth 1 -maxdepth 1 -type d)
TEST_BINS = $(patsubst $(TEST_DIR)/%, $(TEST_BIN_DIR)/%, $(TESTS))

# Regola principale
main: $(TARGET)

# Regola all
all: test $(TARGET)


# Regola per creare il file ELF
$(TARGET): $(OBJ_FILES)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Regola generica per compilare i file sorgente in file oggetto
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Regola per compilare i file di test
$(OBJ_DIR)/test/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


# Regola per compilare per la simulazione
CFLAGS_SIM = "$(CFLAGS) -DSIMULATION"
sim:
	$(MAKE) -s CFLAGS=$(CFLAGS_SIM) all

# Regola per compilare i test
# @echo "TEST_BINS: $(TEST_BINS)"
# @echo "OBJ_FILES: $(OBJ_FILES)"		
# @echo "SRC_FILES: $(SRC_FILES)"
# @echo "LIB_FILES: $(LIB_FILES)"
test : $(TEST_BIN_DIR) $(TEST_BINS)


# Regola per creare la directory dei binari dei test
$(TEST_BIN_DIR):
	mkdir -p $(TEST_BIN_DIR)

# Compilazione dei file oggetto dei test
$(TEST_OBJS): $(TEST_FILES)
	@echo ""
	@echo "Compiling test files..."
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $(filter %/$(patsubst %.o,%.c, $(notdir $@)), $(TEST_FILES)) -o $@

# Link e generazione dei binari dei test
$(TEST_BINS): $(LIB_OBJS) $(TEST_OBJS)
	@echo ""
	@echo "Linking test..."
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(filter %/$(notdir $@).o, $(TEST_OBJS)) $(LIB_OBJS) -o $@ $(LDFLAGS)

# Regola per eseguire objdump
dump: $(TARGET)
	$(OBJDUMP) -d $(TARGET)

# Regola per leggere le sezioni
sections: $(TARGET)
	$(READELF) -S $(TARGET)

# Regola per estrarre i simboli
symbols: $(TARGET)
	$(READELF) -s $(TARGET)

# Pulizia dei file generati
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Pulizia completa
cleanall: clean
	rm -rf $(BIN_DIR)/

# Regole di utilità
.PHONY: all clean cleanall dump sections test