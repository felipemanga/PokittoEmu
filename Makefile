ifeq ($(shell echo $$OS),$$OS)
    MAKEDIR = if not exist "$(1)" mkdir "$(1)"
    RM = rmdir /S /Q "$(1)"
else
    MAKEDIR = '$(SHELL)' -c "mkdir -p \"$(1)\""
    RM = '$(SHELL)' -c "rm -rf \"$(1)\""
endif

OBJDIR := BUILD
SRCDIR := $(CURDIR)

.PHONY: clean

.SUFFIXES:
.SUFFIXES: .cpp .o

vpath %.cpp .
vpath %.h .

PROJECT := PokittoEmu
BPROJECT := BUILD/PokittoEmu

OBJECTS += BUILD/sys.o
OBJECTS += BUILD/iocon.o
OBJECTS += BUILD/gpio.o
OBJECTS += BUILD/adc.o
OBJECTS += BUILD/iap.o
OBJECTS += BUILD/timers.o
OBJECTS += BUILD/thumb2.o
OBJECTS += BUILD/screen.o
OBJECTS += BUILD/mmu.o
OBJECTS += BUILD/main.o
OBJECTS += BUILD/cpu.o

INCLUDE_PATHS += -I./.

LIBRARY_PATHS :=
LD_FLAGS :=
LIBRARIES :=  $(shell sdl2-config --libs)
LD_SYS_LIBS := 

CPP = 'g++' '-c' '-g'
LD  = 'g++'

CXX_FLAGS += -std=c++17
CXX_FLAGS += $(shell sdl2-config --cflags)

.PHONY: all lst size


all: $(BPROJECT) size
	+@$(call MAKEDIR,$(OBJDIR))

clean:
	$(call RM,$(OBJDIR))

BUILD/%.o : %.cpp
	+@$(call MAKEDIR,$(dir $@))
	+@echo "Compile: $(notdir $<)"
	@$(CPP) $(CXX_FLAGS) $(INCLUDE_PATHS) -o $@ $<

$(BPROJECT): $(OBJECTS)
	+@echo "link: $(notdir $@)"
	@$(LD) $(LD_FLAGS) $(LIBRARY_PATHS) -o $@ $(filter %.o, $^) $(LIBRARIES) $(LD_SYS_LIBS)
