CPP = g++
C = gcc

CALYPSO_DIR = /home/project/calypso
JSONCPP_DIR = /home/project/jsoncpp-src-0.6.0-rc2
JSONCPP_LIB = $(JSONCPP_DIR)
JSONCPP_INC = $(JSONCPP_DIR)/include

INCLUDE = -I$(CALYPSO_DIR) -I$(JSONCPP_INC)
LIBS = 
C_ARGS = -g -Wall -D_FILE_OFFSET_BITS=64 -D__STDC_FORMAT_MACROS -D_REENTRANT $(INCLUDE) 

BINARY = bq
bq_dep = browserquest.o http_parser.o websocket_codec.o worldserver.o region_tree.o \
        base64.o sha1.o map_config.o game_map.o entity.o player.o string_helper.o
bq_lib = -L$(CALYPSO_DIR) -lcalypso_main -L$(JSONCPP_LIB) -ljsoncpp -llog4cplus -lgflags -lpthread -ldl

ALL_OBJS = $(foreach d,$(BINARY),$($(subst .,_,$(d))_dep))

%.o : %.cpp
	$(CPP) $(C_ARGS) -c  $< -o $(patsubst %.cpp,%.o,$<)
%.o : %.cc
	$(CPP) $(C_ARGS) -c  $< -o $(patsubst %.cc,%.o,$<)
%.o : %.c
	$(C) $(C_ARGS) -c  $< -o $(patsubst %.c,%.o,$<)
	
all : $(BINARY)

$(BINARY) : $(ALL_OBJS)
	@echo "now building:" $@
	@echo "dep:" $($(subst .,_,$@)_dep)
	rm -f $@
	$(CPP) $(C_ARGS) -o $@ $($(subst .,_,$@)_dep) $(LIBS) $($(subst .,_,$@)_lib)

clean:
	rm -f $(ALL_OBJS) $(BINARY)

print:
	@echo "print all vars"
	@echo "all objs:" $(ALL_OBJS)
