program_EXECUTABLE := elevator

CXX = clang++-3.6

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(notdir $(patsubst %/,%,$(dir $(mkfile_path))))
current_path := $(abspath ../$(current_dir))

program_CXX_SRCS := $(wildcard *.cpp)
program_CXX_OBJS := ${program_CXX_SRCS:.cpp=.o}

program_C_SRCS := $(wildcard *.c)
program_C_OBJS += ${program_C_SRCS:.c=.o}

program_OBJS := $(program_C_OBJS)
program_OBJS += $(program_CXX_OBJS)


program_LIBRARY_DIRS := $(current_dir) $(current_dir)/headers/  
program_LIBRARIES := comedi m pthread

CXXFLAGS += -Wall -g -std=c++11
CFLAGS += -Wall -g -std=c99

CPPFLAGS += $(foreach includedir,$(program_INCLUDE_DIRS),-I$(includedir))
LDFLAGS += $(foreach librarydir,$(program_LIBRARY_DIRS),-L$(librarydir))
LDFLAGS += $(foreach library,$(program_LIBRARIES),-l$(library))

.PHONY: all clean distclean
all: $(program_EXECUTABLE)

$(program_CXX_OBJS):%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(CPPFLAGS) $(LDFLAGS)

$(program_C_OBJS):%.o:%.c
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(CPPFLAGS) $(LDFLAGS)

$(program_EXECUTABLE):$(program_OBJS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -o $(program_EXECUTABLE) $(program_OBJS) $(LDFLAGS)

clean:
	@- $(RM) $(program_EXECUTABLE)
	@- $(RM) $(program_C_OBJS)
	@- $(RM) $(program_CXX_OBJS)

distclean: clean

.PHONY: all clean