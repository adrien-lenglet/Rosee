###### CONF ######

WINDOWS = true
#LINUX = true

RELEASE = true
#DEBUG = true
#SANITIZE = true

##################

CXX = clang++
CXXFLAGS_BASE = -std=c++20
CXXFLAGS = -Wall -Wextra $(CXXFLAGS_BASE)

ifdef SANITIZE
DEBUG = true
CXXFLAGS_BASE += -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
#LD_LIBS += -shared-libsan
endif
ifdef DEBUG
CXXFLAGS_BASE += -g
CXXFLAGS += -DDEBUG
endif
ifdef RELEASE
CXXFLAGS_BASE += -O3
endif

ifdef WINDOWS
LD_LIBS += "$(shell cygpath --unix $(VULKAN_SDK))/Lib/vulkan-1.lib" -lglfw3 -lpthread
endif
ifdef LINUX
LD_LIBS += -lvulkan -lglfw -lpthread
endif

SRCD = src
ROSEED = $(SRCD)/Rosee
ROSEE_SRC = $(ROSEED)/Brush.cpp $(ROSEED)/Cmp.cpp $(ROSEED)/Map.cpp $(ROSEED)/Renderer.cpp $(ROSEED)/Vk.cpp
OBJ_DEP = $(ROSEED)/Vma.o
SRC = $(SRCD)/main.cpp $(ROSEE_SRC)
OBJ = $(SRC:.cpp=.o)

%.vert.spv: %.vert
	glslangValidator $< -V -o $@

%.frag.spv: %.frag
	glslangValidator $< -V -o $@

SHAD = sha
SHA = $(SHAD)/particle.frag $(SHAD)/particle.vert
SHA_VERT = $(SHA:.vert=.vert.spv)
SHA_FRAG = $(SHA:.frag=.frag.spv)
SHAS = $(filter-out $(SHA), $(SHA_VERT) $(SHA_FRAG))

TARGET = rosee

all: $(TARGET) $(SHAS)

$(TARGET): $(OBJ) $(OBJ_DEP)
	$(CXX) $(CXXFLAGS) $(OBJ) $(OBJ_DEP) -o $(TARGET) $(LD_LIBS)

$(ROSEED)/Vma.o:
	$(CXX) $(CXXFLAGS_BASE) -Wno-nullability-completeness $(ROSEED)/Vma.cpp -c -o $(ROSEED)/Vma.o

clean:
	rm -f $(OBJ) $(TARGET)

clean_sha:
	rm -f $(SHAS)

clean_dep:
	rm -f $(OBJ_DEP)

clean_all: clean clean_sha clean_dep
