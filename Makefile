EXE ?= montecraplo

CXX := g++
CXXFLAGS := -std=c++17 -march=native
RELEASEFLAGS = -O3
DEBUGFLAGS = -g -fsanitize=address,undefined

SRCS := $(wildcard engine/*.cpp engine/nn/*.cpp)
HDRS := $(wildcard engine/*.hpp engine/pzstl/*.hpp)
OBJS := $(SRCS:.cpp=.o)

.PHONY: release debug clean

release: CXXFLAGS += $(RELEASEFLAGS)
release: $(EXE)

debug: CXXFLAGS += $(DEBUGFLAGS)
debug: $(EXE)

$(EXE): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "Build complete. Run with './$(EXE)'"

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "Cleaning up..."
	rm -f $(EXE)
	rm -f $(OBJS)
